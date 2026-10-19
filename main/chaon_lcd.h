#ifndef CHAON_LCD_H
#define CHAON_LCD_H
#include <stdbool.h>
#include <stdint.h>
#include "chaon_status.h"

/* ST7735 on original T-Dongle-S3. Experimental until physically inspected.
 * Failure never blocks USB HID, and backlight remains off on init errors.
 */
bool chaon_lcd_open(void);
bool chaon_lcd_show(chaon_status_t status, uint32_t remaining_s);
#endif
