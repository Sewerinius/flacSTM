#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <ff.h>

#include "bitreader.h"

int initBitreader(Bitreader_t *reader, const char *fileName) {

    FRESULT res = f_open(&reader->file, fileName, FA_READ);
    if (res != FR_OK) {
        return -1;
    }
    reader->buffer = 0;
    reader->bitsInBuffer = 0;
    return 0;
}

int read(Bitreader_t *reader, uint16_t nBits, void *d, uint16_t dBytes) {
    uint8_t *dest = (uint8_t *) d;
    uint16_t dBits = dBytes * 8;
    int readAlign = nBits % 8;
    // printf("ReadAlign: %X\n", readAlign);
    assert(dBits >= nBits);

    while (dBits >= nBits + 8) {
        if (BYTE_ORDER != LITTLE_ENDIAN) {
            *dest = 0;
            dest++;
        } else {
            dest[dBytes - 1] = 0;
        }
        dBits -= 8;
        dBytes--;
    }
    if (readAlign > 0) {
        if (readAlign <= reader->bitsInBuffer) {
            reader->buffer <<= readAlign;
            uint8_t out = reader->buffer >> 8;
            if (BYTE_ORDER != LITTLE_ENDIAN) {
                *dest = out;
                dest++;
            } else {
                dest[dBytes - 1] = out;
            }
            reader->buffer &= 0xFF;
            reader->bitsInBuffer -= readAlign;
        } else {
            reader->buffer <<= reader->bitsInBuffer;
            int nextByte = fgetc(reader->file);
            // printf("NextByte: %02X\n", nextByte);
            if (nextByte == EOF) {
                return EOF;
            }
            reader->buffer |= nextByte;
            reader->buffer <<= (readAlign - reader->bitsInBuffer);
            uint8_t out = reader->buffer >> 8;
            // printf("Out: %02X\n", out);
            if (BYTE_ORDER != LITTLE_ENDIAN) {
                *dest = out;
                dest++;
            } else {
                dest[dBytes - 1] = out;
            }
            reader->buffer &= 0xFF;
            reader->bitsInBuffer += 8 - readAlign;
        }
        nBits -= readAlign;
        dBytes--;
    }
    while (nBits > 0) {
        reader->buffer <<= reader->bitsInBuffer;
        int nextByte = fgetc(reader->file);
        // printf("NextByte: %02X\n", nextByte);
        if (nextByte == EOF) {
            return EOF;
        }
        reader->buffer |= nextByte;
        reader->buffer <<= (8 - reader->bitsInBuffer);
        uint8_t out = reader->buffer >> 8;
        // printf("Out: %02X\n", out);
        if (BYTE_ORDER != LITTLE_ENDIAN) {
            *dest = out;
            dest++;
        } else {
            dest[dBytes - 1] = out;
        }
        reader->buffer &= 0xFF;
        nBits -= 8;
        dBits -= 8;
        dBytes--;
    }
    return 0;
}

//int main(int argc, char* argv[]) {
//    Bitreader_t reader;
//    if(initBitreader(&reader, argv[1]) == -1) {
//        perror("Couldn't open file");
//        return -1;
//    }
//    uint16_t buffer;
//    while (read(&reader, 12, &buffer, sizeof(buffer)) != EOF)
//    {
//        printf("%X", buffer);
//        getchar();
//    }
//}