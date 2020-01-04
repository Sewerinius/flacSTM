#ifndef FLACSTM_AUDIOBUFFER_H
#define FLACSTM_AUDIOBUFFER_H

#include <FLAC/all.h>

typedef enum ABState {
    NEW = 0,
    PLAY_WHEN_FILLED,
    FILLED,
    PLAY_IMMEDIATELY,
    PLAYING,
    FREE
} ABState_t;

typedef struct AudioBuffer {
    size_t size;
    uint16_t *buffer;
    ABState_t state;
} AudioBuffer_t;

#define NEW_AUDIO_BUFFER (AudioBuffer_t) {.size = 0, .buffer = NULL, .state = NEW}
#define AB_BUFFER_ELEM_BYTES sizeof(uint16_t)

AudioBuffer_t *newAudioBuffer();
void freeAudioBuffer(AudioBuffer_t *this);

void fillAudioBuffer(AudioBuffer_t *this, const FLAC__Frame *frame, const FLAC__int32 *const buffer[]);

#endif //FLACSTM_AUDIOBUFFER_H
