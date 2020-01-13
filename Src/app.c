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
#include <app_internal.h>
#include "player.h"

static AppState_t state;
//static bool redraw;
static uint32_t events;
//static char currentFolder[512];
static int scrollOffset = 0;
#define ROW_HEIGHT (CHAR_HEIGHT * 3)

void changeAppState(AppState_t newState) {
    state = newState;
}

typedef struct FileList {
    char *fname;
    BYTE attribs;
    struct FileList *next;
} FileList_t;

static int filesLength = 0;
static FileList_t *files = NULL;
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
    filesLength = 1;
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
                filesLength++;
            }
        }
        f_closedir(&dir);
    }

    res = f_findfirst(&dir, &fno, "", "*.flac");

    int i = scrollOffset;
    while (res == FR_OK && fno.fname[0]) {
        head->next = newFileList(fno.fname, fno.fattrib);
        head = head->next;
        filesLength++;
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

    changeAppState(FILE_EXPLORER);
    events = APP_EVENT_REDRAW;
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

static uint32_t handleClick(int x, int y) {
//    printf("Click!\n");
    uint32_t ret = 0;
    switch (state) {
        case FILE_EXPLORER: {
            FileList_t *clicked = getClickedFile(x);
            if (clicked != NULL) {
                if ((clicked->attribs & AM_DIR) != 0) {
                    f_chdir(clicked->fname);
                    openFolder();
                    scrollOffset = CHAR_HEIGHT;
                    ret |= APP_EVENT_REDRAW;
                } else {
                    startPlaying(clicked);
                    ret |= APP_EVENT_REDRAW;
                }
            }
            break;
        }
        case PLAYER:
            ret |= playerHandleClick(x, y);
            break;
    }
    return ret;
}

static uint32_t handleScroll(int dx, int dy) {
    uint32_t ret = 0;
    switch (state) {
        case FILE_EXPLORER:
            if (dx != 0) {
                scrollOffset += dx;
                ret |= APP_EVENT_REDRAW;
            }
            scrollOffset = scrollOffset > -(filesLength - 10) * ROW_HEIGHT ? scrollOffset : -(filesLength - 10) * ROW_HEIGHT;
            scrollOffset = scrollOffset < CHAR_HEIGHT ? scrollOffset : CHAR_HEIGHT;
            break;
        case PLAYER:
            break;
    }
    return ret;
}

static uint32_t handleTouch() {
    uint32_t ret = 0;
    if (ILI9341_TouchPressed() == true) {
        uint16_t x, y;
        ILI9341_TouchGetCoordinates(&x, &y);
//        printf("%d, %d\n", x, y);

        if (startx == -1) {
            startx = x;
            starty = y;
        }

        if (lastx != -1) {
            ret |= handleScroll(x - lastx, y - lasty);
        }
        lastx = x;
        lasty = y;
    } else {
        if (startx != -1) {
            if (abs(startx - lastx) < CLICK_TRESHOLD && abs(starty - lasty) < CLICK_TRESHOLD) {
                ret |= handleClick((startx + lastx) >> 1, (starty + lasty) >> 1);
            }

            startx = -1;
            lastx = -1;
            starty = -1;
            lasty = -1;
        }
    }
    return ret;
}

void appProcess() {
    events |= handleTouch();

    switch (state) {
        case FILE_EXPLORER:
            break;
        case PLAYER:
            events |= playerProcess();
            break;
    }

    if ((events & APP_EVENT_REDRAW) != 0) {
        switch (state) {
            case FILE_EXPLORER:
                drawFileExplorer();
                break;
            case PLAYER:
                playerDraw();
                break;
        }
    }

    events = 0;
}

static void drawFileExplorer() {
    ILI9341_Fill_Screen(WHITE);
    FileList_t *head = files;

    int i = scrollOffset;
    while (head != NULL) {
        if (i >= -CHAR_HEIGHT && i <= 10 * ROW_HEIGHT) {
            char c[22];
            strncpy(c, head->fname, 21);
            c[21] = '\0';
            uint16_t color = (head->attribs & AM_DIR) ? BLACK : BLUE;
            ILI9341_Draw_Text(c, 10, i, color, 2, WHITE);
        }
        i += ROW_HEIGHT;
        head = head->next;
    }

    int y = (scrollOffset - CHAR_HEIGHT) * 240 / ROW_HEIGHT / filesLength;
    printf("%d, %d\n", scrollOffset, y);
    ILI9341_Draw_Rectangle(315, abs(y), 5, (double) 10 / filesLength * 240, LIGHTGREY);
}


