//
// Created by sewerin on 09.01.2020.
//

#ifndef FLACSTM_APP_INTERNAL_H
#define FLACSTM_APP_INTERNAL_H

typedef enum AppState {
    FILE_EXPLORER = 1,
    PLAYER
} AppState_t;

#define APP_EVENT_REDRAW (uint32_t) (1U << 0)

void changeAppState(AppState_t newState);

#endif //FLACSTM_APP_INTERNAL_H
