//
// Created by sewerin on 05.11.2019.
//

#ifndef CLIONTEST_BITREADER_H
#define CLIONTEST_BITREADER_H

typedef struct Bitreader {
    FIL file;
    uint16_t buffer;
    uint8_t bitsInBuffer;
} Bitreader_t;

int initBitreader(Bitreader_t *reader, const char *fileName);

int read(Bitreader_t *reader, uint16_t nBits, void *d, uint16_t dBytes);

#endif //CLIONTEST_BITREADER_H
