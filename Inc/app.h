//
// Created by sewerin on 05.12.2019.
//

#ifndef CLIONTEST_PLAYERAPP_H
#define CLIONTEST_PLAYERAPP_H

typedef enum AppState {
    FILE_EXPLORER = 0,
    PLAYER
} AppState_t;

void appInit();

void appProcess();

#endif //CLIONTEST_PLAYERAPP_H
