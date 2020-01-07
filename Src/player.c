//
// Created by sewerin on 05.11.2019.
//


#include <stdio.h>
#include <FLAC/stream_decoder.h>
#include <FLAC/format.h>
#include <stdbool.h>
#include <uda1380.h>
#include "bitreader.h"
#include "audioBuffer.h"
#include "i2s.h"
#include "player.h"

#define ZEROES_SIZE 256
uint16_t zeroes[ZEROES_SIZE] = {0};
static bool playingZeroes = true;

static int currentlyPlayingBufferIdx = 1;
static int lastFreeBufferIdx = 0;
static AudioBuffer_t buffers[] = {NEW_AUDIO_BUFFER, NEW_AUDIO_BUFFER};
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
    switch (buffers[idx].state) {
        case FILLED:
            playingZeroes = false;
            i2sPlay(idx);
            break;
        case NEW:
        case FREE:
            playingZeroes = true;
            HAL_StatusTypeDef res = HAL_I2S_Transmit_DMA(&hi2s3, zeroes, ZEROES_SIZE);
            if (res != HAL_OK) {
                Error_Handler();
            }
            break;
        case PLAYING:
            Error_Handler();
            break;
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
    FLAC__StreamMetadata_StreamInfo* streamInfo;
} DecoderData_t;

static FLAC__StreamDecoderReadStatus readCallback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data) {
    if (*bytes <= 0)
        return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
    DecoderData_t* decoderData = client_data;
    FIL* fil = &decoderData->file;
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

static FLAC__StreamDecoderSeekStatus seekCallback(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data) {
    printf("S\n");
    return FLAC__STREAM_DECODER_SEEK_STATUS_UNSUPPORTED; //TODO
}

static FLAC__StreamDecoderTellStatus tellCallback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data) {
    DecoderData_t* decoderData = client_data;
    *absolute_byte_offset = (FLAC__uint64) f_tell(&decoderData->file);
    return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

static FLAC__StreamDecoderLengthStatus lengthCallback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data) {
    printf("L\n");
    return FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED; //TODO
}

static FLAC__bool eofCallback(const FLAC__StreamDecoder *decoder, void *client_data) {
    DecoderData_t* decoderData = client_data;
    return f_eof(&decoderData->file);
}

static FLAC__StreamDecoderWriteStatus writeCallback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data) {
//    printf("%lu, %lu, %lu, %lu\n", frame->header.blocksize, frame->header.channels, frame->header.sample_rate, frame->header.bits_per_sample);
    fillAudioBuffer(&buffers[lastFreeBufferIdx], frame, buffer);
    lastFreeBufferIdx = 1-lastFreeBufferIdx;
    freeCount--;
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void metadataCallback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data) {
    DecoderData_t* decoderData = client_data;
    if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
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

static void errorCallback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data) {
    //TODO
    printf("\n\nError: %s\n\n", FLAC__StreamDecoderErrorStatusString[status]);
}

static FLAC__StreamDecoder* decoder;
static DecoderData_t* decoderData;

void playerInit() {
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

    FLAC__stream_decoder_init_stream(decoder, readCallback, seekCallback, tellCallback, lengthCallback, eofCallback, writeCallback, metadataCallback, errorCallback, decoderData);
    FLAC__stream_decoder_process_until_end_of_metadata(decoder);
    FLAC__stream_decoder_process_single(decoder);
//    tryI2sPlayNext();
}

void playerStop() {
    FLAC__stream_decoder_finish(decoder);
    f_close(&decoderData->file);
    free(decoderData->streamInfo);
    decoderData->streamInfo = NULL;
}

void playerProcess() {
    if (freeCount > 0) {
        FLAC__stream_decoder_process_single(decoder); //TODO: EOF
    }
//    } else {
//        HAL_Delay(10); //MAYBE calc delay
//    }
}

void playerDraw() {

}

void playerHandleClick(int x, int y) {

}
