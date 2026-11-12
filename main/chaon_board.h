/* Board GPIO profiles. Plus is inert until physically validated. */
#pragma once
#include "driver/gpio.h"
#if defined(CHAON_BOARD_TDONGLE_S3_PLUS)
#define CHAON_BOARD_NAME "T-Dongle-S3-Plus"
#define CHAON_BOARD_PLUS 1
#define CHAON_BOARD_INERT_BRINGUP 1
#else
/* Native host tests don't use CMake; default to the original board. */
#define CHAON_BOARD_NAME "T-Dongle-S3"
#define CHAON_BOARD_PLUS 0
#define CHAON_BOARD_INERT_BRINGUP 0
#endif
/* Common pins from LILYGO published board pinout. */
#define CHAON_PIN_BOOT GPIO_NUM_0
#define CHAON_PIN_APA102_DATA GPIO_NUM_40
#define CHAON_PIN_APA102_CLOCK GPIO_NUM_39
#define CHAON_PIN_SD_CLK GPIO_NUM_12
#define CHAON_PIN_SD_CMD GPIO_NUM_16
#define CHAON_PIN_SD_D0 GPIO_NUM_14
/* LCD pins from vendor LCD example. Confirm on Plus board revision. */
#define CHAON_PIN_LCD_MOSI GPIO_NUM_3
#define CHAON_PIN_LCD_CLK GPIO_NUM_5
#define CHAON_PIN_LCD_CS GPIO_NUM_4
#define CHAON_PIN_LCD_DC GPIO_NUM_2
#define CHAON_PIN_LCD_RST GPIO_NUM_1
#define CHAON_PIN_LCD_BL GPIO_NUM_38
