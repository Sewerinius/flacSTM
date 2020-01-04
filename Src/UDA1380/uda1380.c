//
// Created by sewerin on 19.12.2019.
//

#include <stm32f7xx_hal.h>
#include <i2c.h>
#include <stdio.h>
#include "stm32f767xx.h"
#include "uda1380.h"

#define UDA1380_WRITE_ADDRESS     0x30
#define UDA1380_READ_ADDRESS      0x31

#define UDA1380_REG_EVALCLK       0x00
#define UDA1380_REG_I2S           0x01
#define UDA1380_REG_PWRCTRL       0x02
#define UDA1380_REG_ANAMIX        0x03
#define UDA1380_REG_HEADAMP       0x04
#define UDA1380_REG_MSTRVOL       0x10
#define UDA1380_REG_MIXVOL        0x11
#define UDA1380_REG_MODEBBT       0x12
#define UDA1380_REG_MSTRMUTE      0x13
#define UDA1380_REG_MIXSDO        0x14
#define UDA1380_REG_DECVOL        0x20
#define UDA1380_REG_PGA           0x21
#define UDA1380_REG_ADC           0x22
#define UDA1380_REG_AGC           0x23

#define UDA1380_REG_L3            0x7f
#define UDA1380_REG_HEADPHONE     0x18
#define UDA1380_REG_DEC           0x28

uint8_t UDA1380InitData[][3] =
        {
                /*
                 *Enable all power for now
                 */
                {UDA1380_REG_PWRCTRL,  0xA5, 0xDF},
                /*
                 *CODEC ADC and DAC clock from WSPLL, all clocks enabled
                 */
                {UDA1380_REG_EVALCLK,  0x0F, 0x39},
                /*
                 *I2S bus data I/O formats, use digital mixer for output
                 *BCKO is slave
                 */
                {UDA1380_REG_I2S,      0x00, 0x00},
                /*
                 *Full mixer analog input gain
                 */
                {UDA1380_REG_ANAMIX,   0x00, 0x00},
                /*
                 *Enable headphone short circuit protection
                 */
                {UDA1380_REG_HEADAMP,  0x02, 0x02},
                /*
                 *Full master volume
                 */
                {UDA1380_REG_MSTRVOL,  0x77, 0x77},
                /*
                 *Enable full mixer volume on both channels
                 */
                {UDA1380_REG_MIXVOL,   0x00, 0x00},
                /*
                 *Bass and treble boost set to flat
                 */
                {UDA1380_REG_MODEBBT,  0x55, 0x15},
                /*
                 *Disable mute and de-emphasis
                 */
                {UDA1380_REG_MSTRMUTE, 0x00, 0x00},
                /*
                 *Mixer off, other settings off
                 */
                {UDA1380_REG_MIXSDO,   0x00, 0x00},
                /*
                 *ADC decimator volume to max
                 */
                {UDA1380_REG_DECVOL,   0x00, 0x00},
                /*
                 *No PGA mute, full gain
                 */
                {UDA1380_REG_PGA,      0x00, 0x00},
                /*
                 *Select line in and MIC, max MIC gain
                 */
                {UDA1380_REG_ADC,      0x0F, 0x02},
                /*
                 *AGC
                 */
                {UDA1380_REG_AGC,      0x00, 0x00},
                /*
                 *Disable clocks to save power
                 *{UDA1380_REG_EVALCLK,     0x00, 0x32},
                 *disable power to input to save power
                 *{UDA1380_REG_PWRCTRL,     0xA5, 0xC0},
                 */

                /*
                 *End of list
                 */
                {0xFF,                 0xFF, 0xFF}
        };

uint8_t UDA1380_Configuration(void) {
    uint8_t dev_addr = UDA1380_WRITE_ADDRESS;
    uint8_t i = 0;
    HAL_StatusTypeDef errorcode;
//    CODEC_I2C_Configuration();

    errorcode = HAL_I2C_IsDeviceReady(&hi2c1, dev_addr, 3, HAL_MAX_DELAY);
    if (errorcode != HAL_OK) {
        printf("UDA1380 Ready result: %d\n", errorcode);
    }

    {
        uint8_t res[] = {0, 0};
        errorcode = HAL_I2C_Mem_Read(&hi2c1, dev_addr, UDA1380_REG_PWRCTRL, 1, res, 2, HAL_MAX_DELAY);
        printf("UDA1380_REG_PWRCTRL Before: 0x%X%X, errorcode=%d\n", res[0], res[1], errorcode);
    }

    while (UDA1380InitData[i][0] != 0xff) {
        errorcode = HAL_I2C_Master_Transmit(&hi2c1, dev_addr, UDA1380InitData[i], 3, HAL_MAX_DELAY); //TODO: Check if data isn't sent in wrong endianess
        if (errorcode == HAL_OK) {
            i++;
        } else {
            printf("I2c ERROR : 0x%x\r\n", errorcode);
            return ERROR;
        }
    }

    {
        uint8_t res[] = {0, 0};
        errorcode = HAL_I2C_Mem_Read(&hi2c1, dev_addr, UDA1380_REG_PWRCTRL, 1, res, 2, HAL_MAX_DELAY);
        printf("UDA1380_REG_PWRCTRL After: 0x%X%X, errorcode=%d\n", res[0], res[1], errorcode);
    }

    printf("UDA1380 Init OK!\r\n");
    return SUCCESS;
}
