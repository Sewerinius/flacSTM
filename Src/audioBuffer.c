#include <stdlib.h>
#include "audioBuffer.h"

AudioBuffer_t *newAudioBuffer() {
    AudioBuffer_t *ret = malloc(sizeof(AudioBuffer_t));
    *ret = NEW_AUDIO_BUFFER;
    return ret;
}

static void resize(AudioBuffer_t *this, size_t newSize) {
    if (this->size < newSize) {
        free(this->buffer);
        this->buffer = malloc(newSize * sizeof(uint16_t));
    }
    this->size = newSize;
}

void fillAudioBuffer(AudioBuffer_t *this, const FLAC__Frame *frame, const FLAC__int32 *const *buffer) {
    assert(this->state != FILLED && this->state != PLAYING && this->state != PLAY_IMMEDIATELY);
//    static int count = 0;
//    printf("%d\n", ++count);
//    printf("%d\n", this->state);

    int bytesPerSample = frame->header.bits_per_sample / 8;
    size_t size = frame->header.blocksize * frame->header.channels * bytesPerSample / AB_BUFFER_ELEM_BYTES;
    if (size != this->size)
        resize(this, size);

    uint16_t tmp = 0;
    int i = 0;
    int k = 0;
    for (int sample = 0; sample < frame->header.blocksize; sample++) {
        for (int channel = 0; channel < frame->header.channels; channel++) {
            for (int byte = bytesPerSample - 1; byte >= 0; byte--) {
                uint8_t tmp2 = ((uint8_t)((buffer[channel][sample] >> (byte * 8)) & 0xFF));
                tmp = tmp << 8 | tmp2;
                if (i == AB_BUFFER_ELEM_BYTES - 1) {
                    this->buffer[k++] = tmp;
                    i = 0;
                } else {
                    i++;
                }
//                this->buffer[(sample * frame->header.channels + channel) * bytesPerSample + byte] =
//                        (uint8_t)((buffer[channel][sample] >> (byte * 8)) & 0xFF);
            }
        }
    }
    assert(k < size + 1);
    if (this->state == PLAY_WHEN_FILLED) {
        this->state = PLAY_IMMEDIATELY;
    } else {
        this->state = FILLED;
    }
}

void freeAudioBuffer(AudioBuffer_t *this) {
    free(this->buffer);
    free(this);
}