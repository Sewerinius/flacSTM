//
// Created by sewerin on 05.11.2019.
//


#include <stdio.h>
#include <FLAC/stream_decoder.h>
#include <FLAC/format.h>
#include <stdbool.h>
#include <uda1380.h>
#include <ILI9341_STM32_Driver.h>
#include <ILI9341_GFX_Ex.h>
#include <ILI9341_GFX.h>
#include <app_internal.h>
#include "bitreader.h"
#include "audioBuffer.h"
#include "i2s.h"
#include "player.h"

#define ZEROES_SIZE 256
uint16_t zeroes[ZEROES_SIZE] = {0};

static bool playingZeroes = true;
static bool paused = false;
static bool fullRedraw = true;

static int currentlyPlayingBufferIdx = 1;
static int lastFreeBufferIdx = 0;

#define TOTAL_BUFFERS 2
static AudioBuffer_t buffers[TOTAL_BUFFERS] = {NEW_AUDIO_BUFFER, NEW_AUDIO_BUFFER};
static int freeCount = 2;

static void i2sPlay(int idx) {
    HAL_StatusTypeDef res = HAL_I2S_Transmit_DMA(&hi2s3, buffers[idx].buffer, buffers[idx].size);
    if (res != HAL_OK) {
        Error_Handler();
    }
    buffers[idx].state = PLAYING;
    currentlyPlayingBufferIdx = idx;
}

static void tryI2sPlayNext() {
    if (buffers[currentlyPlayingBufferIdx].state == PLAYING) {
        Error_Handler();
    }
    int idx = 1 - currentlyPlayingBufferIdx;
    if (buffers[idx].state == FILLED && !paused) {
        playingZeroes = false;
        i2sPlay(idx);
    } else if (buffers[idx].state == PLAYING) {
        Error_Handler();
    } else {
        playingZeroes = true;
        HAL_StatusTypeDef res = HAL_I2S_Transmit_DMA(&hi2s3, zeroes, ZEROES_SIZE);
        if (res != HAL_OK) {
            Error_Handler();
        }
    }
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s) {
    if (!playingZeroes) {
        buffers[currentlyPlayingBufferIdx].state = FREE;
        freeCount++;
    }
    tryI2sPlayNext();
}

void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s) {
    Error_Handler();
}

typedef struct {
    FIL file;
    FLAC__StreamMetadata_StreamInfo *streamInfo;
} DecoderData_t;

static FLAC__StreamDecoderReadStatus
readCallback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data) {
    if (*bytes <= 0)
        return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
    DecoderData_t *decoderData = client_data;
    FIL *fil = &decoderData->file;
    size_t bytesRead = 0;
    FRESULT fresult = f_read(fil, buffer, *bytes, &bytesRead);
    if (fresult != FR_OK) {
        Error_Handler();
        return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
    }
    *bytes = bytesRead;
    if (bytesRead == 0) {
        return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
    }
    return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

static FLAC__StreamDecoderSeekStatus
seekCallback(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data) {
    DecoderData_t *decoderData = client_data;
    FRESULT res = f_lseek(&decoderData->file, absolute_byte_offset);
    if (res != FR_OK) {
        printf("SEEK_ERROR %d\n", res);
        return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
    }
    return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

static FLAC__StreamDecoderTellStatus
tellCallback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data) {
    DecoderData_t *decoderData = client_data;
    *absolute_byte_offset = (FLAC__uint64) f_tell(&decoderData->file);
    return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

static FLAC__StreamDecoderLengthStatus
lengthCallback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data) {
    DecoderData_t *decoderData = client_data;
    *stream_length = (FLAC__uint64) f_size(&decoderData->file);
    return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

static FLAC__bool eofCallback(const FLAC__StreamDecoder *decoder, void *client_data) {
    DecoderData_t *decoderData = client_data;
    return f_eof(&decoderData->file);
}

static FLAC__StreamDecoderWriteStatus
writeCallback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[],
              void *client_data) {
//    printf("BlockSize: %lu\n", frame->header.blocksize);
//    printf("SampleRate: %lu\n", frame->header.sample_rate);
//    printf("channels: %lu\n", frame->header.channels);
//    printf("channel_assignment: %d\n", frame->header.channel_assignment);
//    printf("bits_per_sample: %lu\n", frame->header.bits_per_sample);
//    printf("number_type: %d\n", frame->header.number_type);
//    printf("%lu, %lu, %lu, %lu\n", frame->header.blocksize, frame->header.channels, frame->header.sample_rate, frame->header.bits_per_sample);
    fillAudioBuffer(&buffers[lastFreeBufferIdx], frame, buffer);
    lastFreeBufferIdx = 1 - lastFreeBufferIdx;
    freeCount--;
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void
metadataCallback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data) {
    DecoderData_t *decoderData = client_data;
    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
        printf("MinBlocksize = %lu\n", metadata->data.stream_info.min_blocksize);
        printf("MaxBlocksize = %lu\n", metadata->data.stream_info.max_blocksize);
        printf("MinFramesize = %lu\n", metadata->data.stream_info.min_framesize);
        printf("MaxFramesize = %lu\n", metadata->data.stream_info.max_framesize);
        printf("SampleRate = %lu\n", metadata->data.stream_info.sample_rate);
        printf("Channels = %lu\n", metadata->data.stream_info.channels);
        printf("BitsPerSample = %lu\n", metadata->data.stream_info.bits_per_sample);
        printf("TotalSamples = %lu\n", (uint32_t) metadata->data.stream_info.total_samples);

        decoderData->streamInfo = malloc(sizeof(FLAC__StreamMetadata_StreamInfo));
        *decoderData->streamInfo = metadata->data.stream_info;
    }
}

static void
errorCallback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data) {
    //TODO
    printf("\n\nError: %s\n\n", FLAC__StreamDecoderErrorStatusString[status]);
}

static FLAC__StreamDecoder *decoder;
static DecoderData_t *decoderData;

void playerInit() {
//    for (int i = 0; i < ZEROES_SIZE; i++) {
//        if (i < ZEROES_SIZE / 2) {
//            zeroes[i] = 0xFFFF * i / (ZEROES_SIZE / 2);
//        } else {
//            zeroes[i] = 0xFFFF * (ZEROES_SIZE - i) / (ZEROES_SIZE / 2);
//        }
//    }

    tryI2sPlayNext();
    HAL_Delay(100);

    decoder = FLAC__stream_decoder_new();
    decoderData = malloc(sizeof(DecoderData_t));
    decoderData->streamInfo = NULL;

    UDA1380_Configuration();
}

void playerInitFile(char *fileName) {
    FRESULT fresult = f_open(&decoderData->file, fileName, FA_READ);
    if (fresult != FR_OK) {
        Error_Handler();
    }

    FLAC__stream_decoder_init_stream(decoder, readCallback, seekCallback, tellCallback, lengthCallback, eofCallback,
                                     writeCallback, metadataCallback, errorCallback, decoderData);
    FLAC__stream_decoder_process_until_end_of_metadata(decoder);
    FLAC__stream_decoder_process_single(decoder);

    changeAppState(PLAYER);
    fullRedraw = true;
}

void playerStop() {
    FLAC__stream_decoder_finish(decoder);
    f_close(&decoderData->file);
    free(decoderData->streamInfo);
    decoderData->streamInfo = NULL;
}

uint32_t playerProcess() {
    uint32_t ret = 0;
    if (freeCount > 0) {
        if(FLAC__stream_decoder_process_single(decoder)) {
            ret |= APP_EVENT_REDRAW;
        } //TODO: EOF
    } else {
        HAL_Delay(10); //MAYBE calc delay
    }
    return ret;
}

#define PLAY_BUTTON_X 160
#define PLAY_BUTTON_Y 150
#define PLAY_BUTTON_RADIUS 30

#define BACK_BUTTON_SIZE 20
#define BACK_BUTTON_LEFT 10
#define BACK_BUTTON_TOP 10
#define BACK_BUTTON_MIDDLE_X BACK_BUTTON_LEFT + BACK_BUTTON_SIZE / 2
#define BACK_BUTTON_RIGHT BACK_BUTTON_LEFT + BACK_BUTTON_SIZE
#define BACK_BUTTON_MIDDLE_Y BACK_BUTTON_TOP + BACK_BUTTON_SIZE / 2
#define BACK_BUTTON_BOTTOM BACK_BUTTON_TOP + BACK_BUTTON_SIZE

#define PROGRESS_BAR_TOP 200
#define PROGRESS_BAR_BOTTOM 220
#define PROGRESS_BAR_HEIGHT PROGRESS_BAR_BOTTOM - PROGRESS_BAR_TOP
#define PROGRESS_BAR_LEFT 10
#define PROGRESS_BAR_RIGHT 310
#define PROGRESS_BAR_WIDTH PROGRESS_BAR_RIGHT - PROGRESS_BAR_LEFT

#define VOLUME_UP_X 285
#define VOLUME_UP_Y 25
#define VOLUME_DOWN_X 285
#define VOLUME_DOWN_Y 75
#define VOLUME_BUTTONS_RADIUS 15
#define VOLUME_BUTTONS_SIZE 22
#define VOLUME_BUTTONS_WIDTH 7

static bool lastPaused = false;

void playerDraw() {
    if (fullRedraw) {
        fullRedraw = false;
        ILI9341_Fill_Screen(WHITE);

        ILI9341_Draw_Overlapping_Triangle(BACK_BUTTON_LEFT, BACK_BUTTON_MIDDLE_Y, BACK_BUTTON_RIGHT, BACK_BUTTON_TOP, BACK_BUTTON_RIGHT, BACK_BUTTON_BOTTOM,
                                          BACK_BUTTON_MIDDLE_X, BACK_BUTTON_MIDDLE_Y, BACK_BUTTON_RIGHT, BACK_BUTTON_TOP + BACK_BUTTON_SIZE / 4, BACK_BUTTON_RIGHT, BACK_BUTTON_BOTTOM - BACK_BUTTON_SIZE / 4, BLACK);

        ILI9341_Draw_Hollow_Circle(VOLUME_UP_X, VOLUME_UP_Y, VOLUME_BUTTONS_RADIUS, BLACK);
        ILI9341_Draw_Filled_Rectangle_Coord(VOLUME_UP_X - VOLUME_BUTTONS_SIZE/2, VOLUME_UP_Y - VOLUME_BUTTONS_WIDTH/2, VOLUME_UP_X + VOLUME_BUTTONS_SIZE/2, VOLUME_UP_Y + VOLUME_BUTTONS_WIDTH/2, BLACK);
        ILI9341_Draw_Filled_Rectangle_Coord(VOLUME_UP_X - VOLUME_BUTTONS_WIDTH/2, VOLUME_UP_Y - VOLUME_BUTTONS_SIZE/2, VOLUME_UP_X +  VOLUME_BUTTONS_WIDTH/2, VOLUME_UP_Y + VOLUME_BUTTONS_SIZE/2, BLACK);

        ILI9341_Draw_Hollow_Circle(VOLUME_DOWN_X, VOLUME_DOWN_Y, VOLUME_BUTTONS_RADIUS, BLACK);
        ILI9341_Draw_Filled_Rectangle_Coord(VOLUME_DOWN_X - VOLUME_BUTTONS_SIZE/2, VOLUME_DOWN_Y - VOLUME_BUTTONS_WIDTH/2, VOLUME_DOWN_X + VOLUME_BUTTONS_SIZE/2, VOLUME_DOWN_Y + VOLUME_BUTTONS_WIDTH/2, BLACK);
    }

//    if(lastPaused)
//    ILI9341_Draw_Filled_Circle(PLAY_BUTTON_X, PLAY_BUTTON_Y, PLAY_BUTTON_RADIUS, WHITE);
    ILI9341_Draw_Filled_Rectangle_Coord(PLAY_BUTTON_X - PLAY_BUTTON_RADIUS, PLAY_BUTTON_Y - PLAY_BUTTON_RADIUS, PLAY_BUTTON_X + PLAY_BUTTON_RADIUS, PLAY_BUTTON_Y + PLAY_BUTTON_RADIUS, WHITE);
    ILI9341_Draw_Hollow_Circle(PLAY_BUTTON_X, PLAY_BUTTON_Y, PLAY_BUTTON_RADIUS, BLACK);
    if (paused) {
        ILI9341_Draw_Filled_Triangle(145, 135, 175, 150, 145, 165, BLACK);
    } else {
        ILI9341_Draw_Filled_Rectangle_Coord(145, 135, 155, 165, BLACK);
        ILI9341_Draw_Filled_Rectangle_Coord(165, 135, 175, 165, BLACK);
    }

    ILI9341_Draw_Filled_Rectangle_Coord(PROGRESS_BAR_LEFT, PROGRESS_BAR_TOP, PROGRESS_BAR_RIGHT, PROGRESS_BAR_BOTTOM, WHITE);
    ILI9341_Draw_Hollow_Rectangle_Coord(PROGRESS_BAR_LEFT, PROGRESS_BAR_TOP, PROGRESS_BAR_RIGHT, PROGRESS_BAR_BOTTOM, BLACK);

    FLAC__uint64 pos;
    FLAC__stream_decoder_get_decode_position(decoder, &pos);
    double prog = (double) pos / f_size(&decoderData->file);
    ILI9341_Draw_Rectangle(PROGRESS_BAR_LEFT, PROGRESS_BAR_TOP, prog * PROGRESS_BAR_WIDTH, PROGRESS_BAR_HEIGHT, BLACK);
}

uint32_t playerHandleClick(int y, int x) {
    uint32_t ret = 0;

    { //Check for play button
        int dx = abs(PLAY_BUTTON_X - x);
        int dy = abs(PLAY_BUTTON_Y - y);

        if (dx * dx + dy * dy < PLAY_BUTTON_RADIUS * PLAY_BUTTON_RADIUS) {
            paused = !paused;
            ret |= APP_EVENT_REDRAW;
        }
    }

    { //Check for back button
        if (x >= BACK_BUTTON_LEFT && x <= BACK_BUTTON_RIGHT && y >= BACK_BUTTON_TOP && y <= BACK_BUTTON_BOTTOM) {
            playerStop();
            changeAppState(FILE_EXPLORER);
            ret |= APP_EVENT_REDRAW;
        }
    }

    { //Check for progress bar
        if (x >= PROGRESS_BAR_LEFT && x <= PROGRESS_BAR_RIGHT && y >= PROGRESS_BAR_TOP && y <= PROGRESS_BAR_BOTTOM) {
//            printf("%d, %d\n", currentlyPlayingBufferIdx, lastFreeBufferIdx);
            FLAC__uint64 prog = FLAC__stream_decoder_get_total_samples(decoder) * (double) (x-PROGRESS_BAR_LEFT) / PROGRESS_BAR_WIDTH;
            for (int i = 0; i < TOTAL_BUFFERS; i++) {
                while(buffers[i].state == PLAYING);
                if(buffers[i].state == FILLED) {
                    buffers[i].state = FREE;
                    freeCount++;
                    lastFreeBufferIdx = i; //TODO figure out why assertion breaks without this line
                }
            }
            FLAC__bool res = FLAC__stream_decoder_seek_absolute(decoder, prog);
            if (!res) {
                printf("SEEK_ERROR\n");
                Error_Handler();
            }
            ret |= APP_EVENT_REDRAW;
        }
    }

    { //Check for volume up button
        int dx = abs(VOLUME_UP_X - x);
        int dy = abs(VOLUME_UP_Y - y);

        if (dx * dx + dy * dy < VOLUME_BUTTONS_RADIUS * VOLUME_BUTTONS_RADIUS) {
            // increase volume level
            volumeUp();
        }
    }

    { //Check for volume down button
        int dx = abs(VOLUME_DOWN_X - x);
        int dy = abs(VOLUME_DOWN_Y - y);

        if (dx * dx + dy * dy < VOLUME_BUTTONS_RADIUS * VOLUME_BUTTONS_RADIUS) {
            // decrease volume level
            volumeDown();
        }
    }

    return ret;
}
