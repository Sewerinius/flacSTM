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
#include <uda1380.h>
#include "app.h"
#include "player.h"

static AppState_t state;
static bool redraw;
//static char currentFolder[512];
static int scrollOffset;
#define ROW_HEIGHT CHAR_HEIGHT * 3

typedef struct FileList {
    char *fname;
    BYTE attribs;
    struct FileList *next;
} FileList_t;

FileList_t *files = NULL;
FileList_t *currentlyPlaying;

static void freeFileList(FileList_t *list) {
    while (list != NULL) {
        FileList_t *next = list->next;
        free(list->fname);
        free(list);
        list = next;
    }
}

static FileList_t *newFileList(char *name, BYTE attribs) {
    FileList_t *newfl = malloc(sizeof(FileList_t));
    size_t len = strlen(name);
    newfl->fname = malloc(sizeof(char) * (len + 1));
    strcpy(newfl->fname, name);
    newfl->attribs = attribs;
    newfl->next = NULL;
    return newfl;
}

static void openFolder() {
    freeFileList(files);

    files = newFileList("..", AM_DIR);
    FileList_t *head = files;

    FRESULT res;
    DIR dir;
    FILINFO fno;

    res = f_opendir(&dir, "");                       /* Open the directory */
    if (res == FR_OK) {
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
            if (fno.fattrib & AM_DIR) {                    /* It is a directory */
                head->next = newFileList(fno.fname, fno.fattrib);
                head = head->next;
            }
        }
        f_closedir(&dir);
    }

    res = f_findfirst(&dir, &fno, "", "*.flac");

    int i = scrollOffset;
    while (res == FR_OK && fno.fname[0]) {
        head->next = newFileList(fno.fname, fno.fattrib);
        head = head->next;
        f_findnext(&dir, &fno);
    }

    f_closedir(&dir);
}

static void drawFileExplorer();

void appInit() {
    playerInit();

    ILI9341_Init();
    ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
    ILI9341_Fill_Screen(WHITE);
    scrollOffset = CHAR_HEIGHT;

    state = FILE_EXPLORER;
    redraw = true;
    openFolder();
//    strcpy(currentFolder, "/");
}

static int startx = -1;
static int lastx = -1;
static int starty = -1;
static int lasty = -1;
#define CLICK_TRESHOLD 5

static void startPlaying(FileList_t* file) {
    currentlyPlaying = file;
    state = PLAYER;
    playerInitFile(file->fname);
}

FileList_t* getClickedFile(int x) {
    FileList_t* head = files;
    if (scrollOffset > x)
        return NULL;
    int i = scrollOffset;
    while (head != NULL) {
        if (x > i && x < i + ROW_HEIGHT) {
            return head;
        }
        i += ROW_HEIGHT;
        head = head->next;
    }
    return NULL;
}

static void handleClick(int x, int y) {
//    printf("Click!\n");
    switch (state) {
        case FILE_EXPLORER: {
            FileList_t *clicked = getClickedFile(x);
            if (clicked != NULL) {
                if ((clicked->attribs & AM_DIR) != 0) {
                    f_chdir(clicked->fname);
                    openFolder();
                    redraw = true;
                } else {
                    startPlaying(clicked);
                    redraw = true;
                }
            }
            break;
        }
        case PLAYER:
            playerHandleClick(x, y);
            break;
    }
}

static void handleScroll(int dx, int dy) {
    switch (state) {
        case FILE_EXPLORER:
            if (dx != 0) {
                scrollOffset += dx;
                redraw = true;
            }
            break;
        case PLAYER:
            break;
    }
}

static void handleTouch() {
    if (ILI9341_TouchPressed() == true) {
        uint16_t x, y;
        ILI9341_TouchGetCoordinates(&x, &y);
//        printf("%d, %d\n", x, y);

        if (startx == -1) {
            startx = x;
            starty = y;
        }

        if (lastx != -1) {
            handleScroll(x - lastx, y - lasty);
        }
        lastx = x;
        lasty = y;
    } else {
        if (startx != -1) {
            if (abs(startx - lastx) < CLICK_TRESHOLD && abs(starty - lasty) < CLICK_TRESHOLD) {
                handleClick((startx + lastx) >> 1, (starty + lasty) >> 1);
            }

            startx = -1;
            lastx = -1;
            starty = -1;
            lasty = -1;
        }
    }
}

void appProcess() {
    handleTouch();

    switch (state) {
        case FILE_EXPLORER:
            break;
        case PLAYER:
            playerProcess();
            break;
    }

    if (redraw == true) {
        redraw = false;
        ILI9341_Fill_Screen(WHITE);
        switch (state) {
            case FILE_EXPLORER:
                drawFileExplorer();
                break;
            case PLAYER:
                playerDraw();
                break;
        }
    }

    //    FLACdecode("/weWillRockYou.flac");
}

static void drawFileExplorer() {
    FileList_t *head = files;

    int i = scrollOffset;
    while (head != NULL) {
        char c[22];
        strncpy(c, head->fname, 21);
        c[21] = '\0';
        uint16_t color = (head->attribs & AM_DIR) ? BLACK : BLUE;
        ILI9341_Draw_Text(c, 10, i, color, 2, WHITE);
        i += ROW_HEIGHT;
        head = head->next;
    }

}


