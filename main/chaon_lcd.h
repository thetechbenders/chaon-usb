#ifndef CHAON_LCD_H
#define CHAON_LCD_H
#include <stdbool.h>
#include <stdint.h>
#include "chaon_status.h"
#include "chaon_sd_diag.h"
#include "chaon_fat_ro.h"

/* ST7735 on original T-Dongle-S3. Experimental until physically inspected.
 * Failure never blocks USB HID, and backlight remains off on init errors.
 */
bool chaon_lcd_open(void);
bool chaon_lcd_show(chaon_status_t status, uint32_t remaining_s,
                    chaon_sd_diag_t sd);
/* Main-task-only; transfers 160x8 pixels instead of a full screen. */
bool chaon_lcd_show_sd(chaon_sd_diag_t sd);
/* Main UI task only: passive read-only root entry preview. */
bool chaon_lcd_show_catalog(const chaon_fat_catalog_t *cat, uint8_t index);
/* Main-loop-only full-screen browser; no file reads or HID. */
bool chaon_lcd_show_browser(const chaon_fat_catalog_t *cat,
                            uint8_t index, bool picked);
#endif
