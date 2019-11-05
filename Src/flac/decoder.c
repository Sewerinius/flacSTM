//#include <stdio.h>
//#include <stdlib.h>
//#include <stdint.h>
//#include <assert.h>
//
//#include "constants.h"
//
//typedef struct frameBlock {
//    uint16_t syncCodeAndBlockingStrategy;
//    uint8_t blockSizeAndSampleRate;
//    uint8_t channelsAndSampleSize;
//    struct frameBlock* next;
//} frameBlock_t;
//
//typedef struct metadataBlock {
//    uint8_t type;
//    uint32_t length;
//    uint8_t* data;
//    struct metadataBlock* next;
//} metadataBlock_t;
//
//typedef struct flacData{
//    char flacString[5];
//    metadataBlock_t* metadataBlockChain;
//    frameBlock_t* frameBlockChain;
//} flacData_t;
//
//void freeFlacData(flacData_t* data) {
//    metadataBlock_t* metadataBlock = data->metadataBlockChain;
//    while (metadataBlock != NULL) {
//        metadataBlock_t* tmp = metadataBlock;
//        metadataBlock = metadataBlock->next;
//        free(tmp->data);
//        free(tmp);
//    }
//    frameBlock_t* frameBlock = data->frameBlockChain;
//    while (frameBlock != NULL) {
//        frameBlock_t* tmp = frameBlock;
//        frameBlock = frameBlock->next;
//        free(tmp);
//    }
//}
//
//// void readBytes(FILE* file, int n, uint32_t* dest) {
////     *dest = 0;
////     while (n--) {
////         *dest <<= 8;
////         *dest |= fgetc(file);
////     }
//// }
//
//void readBytes(FILE* file, int n, void* dest, int destSize, int endianess) { //WRONG, endianess problems
//    assert(destSize >= n);
//    uint8_t* d = (uint8_t*) dest;
//
//    if (endianess == BYTE_ORDER) {
//        fread(d, 1, n, file);
//        d += n;
//        destSize -= n;
//        while (destSize--) {
//            *d = 0;
//            d++;
//        }
//    } else {
//        while (destSize > n) {
//            d[destSize] = 0;
//            destSize--;
//        }
//        while (n--) {
//            d[n] = fgetc(file);
//        }
//    }
//
//}
//
//metadataBlock_t* newMetadataBlock(FILE* file) {
//    metadataBlock_t* block = malloc(sizeof(metadataBlock_t));
//    block->next = NULL;
//    block->type = fgetc(file);
//    readBytes(file, 3, &(block->length), 4, BIG_ENDIAN);
//    block->data = malloc(block->length * sizeof(uint8_t));
//    fread(block->data, 1, block->length, file);
//    return block;
//}
//
//frameBlock_t* newFrameBlock(FILE* file) {
//    frameBlock_t* block = malloc(sizeof(frameBlock_t));
//    block->next = NULL;
//    // fread(&(block->syncCodeAndBlockingStrategy), 2, 1, file);
//    readBytes(file, 2, &(block->syncCodeAndBlockingStrategy), 2, BIG_ENDIAN);
//    block->blockSizeAndSampleRate = fgetc(file);
//    block->channelsAndSampleSize = fgetc(file);
//    return block;
//}
//
//void readFlac(FILE* file, flacData_t* data) {
//    fgets(data->flacString, 5, file);
//    metadataBlock_t* lastBlock = newMetadataBlock(file);
//    data->metadataBlockChain = lastBlock;
//    while ((lastBlock->type & 0x80) == 0) {
//        lastBlock->next = newMetadataBlock(file);
//        lastBlock = lastBlock->next;
//    }
//    data->frameBlockChain = newFrameBlock(file);
//}
//
//const char *bit_rep[16] = {
//    [ 0] = "0000", [ 1] = "0001", [ 2] = "0010", [ 3] = "0011",
//    [ 4] = "0100", [ 5] = "0101", [ 6] = "0110", [ 7] = "0111",
//    [ 8] = "1000", [ 9] = "1001", [10] = "1010", [11] = "1011",
//    [12] = "1100", [13] = "1101", [14] = "1110", [15] = "1111",
//};
//
//void printFlac(flacData_t* data) {
//    printf("%s - must be equal to fLaC\n", data->flacString);
//    // printf("%d - STREAMINFO type\n", data->metadataBlockChain->type);
//    // printf("%d - STREAMINFO length\n", data->metadataBlockChain->length);
//    metadataBlock_t* metadataBlock = data->metadataBlockChain;
//    while (metadataBlock != NULL) {
//        printf("Metadata Block with type 0x%02X and length %d\n", metadataBlock->type, metadataBlock->length);
//        metadataBlock = metadataBlock->next;
//    }
//    frameBlock_t* frameBlock = data->frameBlockChain;
//    while (frameBlock != NULL) {
//        printf("Frame Block: \nBlocking startegy: 0b%d\n", frameBlock->syncCodeAndBlockingStrategy & 0x1);
//        printf("BlockSize: 0b%s, SamplingRate: 0b%s\n", bit_rep[frameBlock->blockSizeAndSampleRate >> 4], bit_rep[frameBlock->blockSizeAndSampleRate & 0x0F]);
//        printf("Channels: 0b%s, SampleSize: 0b%s\n", bit_rep[frameBlock->blockSizeAndSampleRate >> 4], bit_rep[(frameBlock->blockSizeAndSampleRate & 0x0F) >> 1]);
//        frameBlock = frameBlock->next;
//    }
//}
//
//void print_byte(uint8_t byte)
//{
//    printf("%s%s", bit_rep[byte >> 4], bit_rep[byte & 0x0F]);
//}
//
//int main(int argc, char* argv[]) {
//    if (argc < 2) {
//        fprintf(stderr, "Usage: %s filename\n", argv[0]);
//        return -1;
//    }
//    FILE * f = fopen(argv[1], "rb");
//    if (f == NULL) {
//        perror("Error while opening file");
//        return -1;
//    }
//
//    // register_printf_function();
//
//    flacData_t data;
//
//    readFlac(f, &data);
//    printFlac(&data);
//
//    getc(stdin);
//
//    int c;
//    while ((c = fgetc(f)) != EOF) {
//        printf("0b%s%s 0x%02X", bit_rep[c >> 4], bit_rep[c & 0x0F], c);
//        getc(stdin);
//    }
//
//    freeFlacData(&data);
//    fclose(f);
//}