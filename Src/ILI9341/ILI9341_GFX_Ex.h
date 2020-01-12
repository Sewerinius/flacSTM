//
// Created by sewerin on 09.01.2020.
//

#ifndef FLACSTM_ILI9341_GFX_EX_H
#define FLACSTM_ILI9341_GFX_EX_H

#include <stdint.h>

void ILI9341_Draw_Filled_Triangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint16_t Colour);
void ILI9341_Draw_Overlapping_Triangle(int16_t x11, int16_t y11, int16_t x12, int16_t y12, int16_t x13, int16_t y13,
                                       int16_t x21, int16_t y21, int16_t x22, int16_t y22, int16_t x23, int16_t y23, uint16_t Colour);

#endif //FLACSTM_ILI9341_GFX_EX_H
