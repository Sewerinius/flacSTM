//
// Created by sewerin on 05.12.2019.
//

#ifndef CLIONTEST_PLAYERAPP_H
#define CLIONTEST_PLAYERAPP_H

typedef enum playerState {
    PLAYER_FILEEXPLORER = 0,
    PLAYER_PLAYER
} playerState_t;

void playerInit();

void playerProcess();

#endif //CLIONTEST_PLAYERAPP_H
