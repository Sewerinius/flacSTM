//
// Created by sewerin on 05.12.2019.
//

#include <ILI9341_STM32_Driver.h>
#include <stdbool.h>
#include <string.h>
#include <ili9341_touch.h>
#include <ff.h>
#include <ILI9341_GFX.h>
#include <5x5_font.h>
#include "playerApp.h"

static playerState_t state;
static bool redraw;
static char currentFolder[512];
static int scrollOffset;

void drawFileExplorer();

void playerInit() {
    ILI9341_Init();
    ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
    ILI9341_Fill_Screen(WHITE);
    scrollOffset = CHAR_HEIGHT;

    state = PLAYER_FILEEXPLORER;
    redraw = true;
    strcpy(currentFolder, "/");
}

static int lastx = -1;

void playerProcess() {
    if (ILI9341_TouchPressed() == true) {
        uint16_t x, y;
        ILI9341_TouchGetCoordinates(&x, &y);
        printf("%d, %d\n", x, y);

        if (lastx != -1) {
            scrollOffset += x - lastx;
        }
        lastx = x;

        redraw = true;
    } else {
        lastx = -1;
    }

    if (redraw == true) {
        redraw = false;
        ILI9341_Fill_Screen(WHITE);
        switch (state) {

            case PLAYER_FILEEXPLORER:
                drawFileExplorer();
                break;
            case PLAYER_PLAYER:
                break;
        }
    }

    //    FLACdecode("/weWillRockYou.flac");
}

void drawFileExplorer() {
    DIR dir;
    FRESULT fr;
    FILINFO fno;
    fr = f_findfirst(&dir, &fno, "", "*.flac");

    int i = scrollOffset;
    while (fr == FR_OK && fno.fname[0]) {
        char c[22];
        strncpy(c, fno.fname, 21);
        c[21] = '\0';
        ILI9341_Draw_Text(c, 10, i, BLACK, 2, WHITE);
        i += CHAR_HEIGHT * 4;
        f_findnext(&dir, &fno);
    }

    f_closedir(&dir);
}


