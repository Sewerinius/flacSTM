//
// Created by sewerin on 09.01.2020.
//

#include "ILI9341_STM32_Driver.h"
#include "ILI9341_GFX_Ex.h"

static int32_t orient2d(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3) {
    return (int32_t) (x2-x1)*(y3-y1) - (y2-y1)*(x3-x1);
}

#define min(a, b) (a < b ? a : b)
#define max(a, b) (a > b ? a : b)

extern uint16_t LCD_WIDTH;
extern uint16_t LCD_HEIGHT;

void ILI9341_Draw_Filled_Triangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint16_t Colour) {
    // Compute triangle bounding box
    int minX = min(min(x1, x2), x3);
    int minY = min(min(y1, y2), y3);
    int maxX = max(max(x1, x2), x3);
    int maxY = max(max(y1, y2), y3);

    // Clip against screen bounds
    minX = max(minX, 0);
    minY = max(minY, 0);
    maxX = min(maxX, LCD_WIDTH - 1);
    maxY = min(maxY, LCD_HEIGHT - 1);

    // Rasterize
    for (int16_t y = minY; y <= maxY; y++) {
        for (int16_t x = minX; x <= maxX; x++) {
            // Determine barycentric coordinates
            int w0 = orient2d(x2, y2, x3, y3, x, y);
            int w1 = orient2d(x3, y3, x1, y1, x, y);
            int w2 = orient2d(x1, y1, x2, y2, x, y);

            // Ifx, y is on or inside all edges, render pixel.
            if (w0 >= 0 && w1 >= 0 && w2 >= 0)
                ILI9341_Draw_Pixel((uint16_t) x, (uint16_t) y, Colour);
        }
    }
}

void ILI9341_Draw_Overlapping_Triangle(int16_t x11, int16_t y11, int16_t x12, int16_t y12, int16_t x13, int16_t y13,
                                       int16_t x21, int16_t y21, int16_t x22, int16_t y22, int16_t x23, int16_t y23, uint16_t Colour) {
    // Compute triangle bounding box
    int minX = min(min(x11, x12), x13);
    int minY = min(min(y11, y12), y13);
    int maxX = max(max(x11, x12), x13);
    int maxY = max(max(y11, y12), y13);

    // Clip against screen bounds
    minX = max(minX, 0);
    minY = max(minY, 0);
    maxX = min(maxX, LCD_WIDTH - 1);
    maxY = min(maxY, LCD_HEIGHT - 1);

    // Rasterize
    for (int16_t y = minY; y <= maxY; y++) {
        for (int16_t x = minX; x <= maxX; x++) {
            // Determine barycentric coordinates
            int w10 = orient2d(x12, y12, x13, y13, x, y);
            int w11 = orient2d(x13, y13, x11, y11, x, y);
            int w12 = orient2d(x11, y11, x12, y12, x, y);

            int w20 = orient2d(x22, y22, x23, y23, x, y);
            int w21 = orient2d(x23, y23, x21, y21, x, y);
            int w22 = orient2d(x21, y21, x22, y22, x, y);

            // Ifx, y is on or inside all edges, render pixel.
            if ((w10 | w11 | w12) >= 0 && (w20 | w21 | w22) < 0)
                ILI9341_Draw_Pixel((uint16_t) x, (uint16_t) y, Colour);
        }
    }
}
