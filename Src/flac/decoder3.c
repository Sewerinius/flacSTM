//
// Created by sewerin on 05.11.2019.
//


#include <stdio.h>
#include <FLAC/stream_decoder.h>
#include <FLAC/format.h>
#include "bitreader.h"
#include "decoder.h"

typedef struct {
    FIL file;
    FIL out;
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
    return false; //TODO
}

//static FLAC__StreamDecoderWriteStatus writeCallback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data) {
//    printf(".");
//    fflush(stdout);
//    DecoderData_t* data = client_data;
//    for (int sample = 0; sample < frame->header.blocksize; ++sample) {
//        for (int channel = 0; channel < frame->header.channels; ++channel) {
//            UINT bytesWritten;
//            FRESULT fresult = f_write(&data->out, buffer[channel] + sample, frame->header.bits_per_sample/8, &bytesWritten);
//            if (fresult != FR_OK)
//                return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
//        }
//    }
//    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
//}

static FLAC__StreamDecoderWriteStatus writeCallback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data) {
    printf("%lu, %lu, %lu, %lu\n", frame->header.blocksize, frame->header.channels, frame->header.sample_rate, frame->header.bits_per_sample);
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void metadataCallback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data) {
    if(metadata->type == 0) {
        printf("MinBlocksize = %lu\n", metadata->data.stream_info.min_blocksize);
        printf("MaxBlocksize = %lu\n", metadata->data.stream_info.max_blocksize);
        printf("MinFramesize = %lu\n", metadata->data.stream_info.min_framesize);
        printf("MaxFramesize = %lu\n", metadata->data.stream_info.max_framesize);
        printf("SampleRate = %lu\n", metadata->data.stream_info.sample_rate);
        printf("Channels = %lu\n", metadata->data.stream_info.channels);
        printf("BitsPerSample = %lu\n", metadata->data.stream_info.bits_per_sample);
        printf("TotalSamples = %llu\n", metadata->data.stream_info.total_samples);
    }
}

static void errorCallback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data) {
    //TODO
}


int FLACdecode(char *path) {
    DecoderData_t decoderData;
    FRESULT fresult = f_open(&decoderData.file, path, FA_READ);
    if (fresult != FR_OK) {
        Error_Handler();
        return -1;
    }
    fresult = f_open(&decoderData.out, "out2.wav", FA_WRITE | FA_CREATE_ALWAYS);
    if (fresult != FR_OK) {
        Error_Handler();
        return -1;
    }

    FLAC__StreamDecoder *decoder = FLAC__stream_decoder_new();
    FLAC__stream_decoder_init_stream(decoder, readCallback, seekCallback, tellCallback, lengthCallback, eofCallback, writeCallback, metadataCallback, errorCallback, &decoderData);
    printf("Decoding\n");
    FLAC__stream_decoder_process_until_end_of_stream(decoder);

    f_close(&decoderData.file);
    f_close(&decoderData.out);
    printf("Done\n\r");

    return 0;

}
