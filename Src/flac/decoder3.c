//
// Created by sewerin on 05.11.2019.
//


#include <stdio.h>
#include "bitreader.h"
#include "decoder.h"

int FLACdecode(char *path) {
    Bitreader_t reader;
    if (initBitreader(&reader, path) == -1) {
        perror("Couldn't open file");
        return -1;
    }
    uint16_t buffer;
    while (read(&reader, 12, &buffer, sizeof(buffer)) != EOF) {
        printf("%X", buffer);
    }
}
