#ifndef CHAON_LCD_H
#define CHAON_LCD_H
#include <stdbool.h>
#include <stdint.h>
#include "chaon_status.h"
#include "chaon_sd_diag.h"
#include "chaon_fat_ro.h"
#include "chaon_inspect.h"

/* ST7735 on original T-Dongle-S3. Experimental until physically inspected.
 * Failure never blocks the embedded demo; selected SD execution fails closed.
 * The backlight remains off on initialization errors.
 */
bool chaon_lcd_open(void);
/* Safe before USB enumeration; independent of SD and FAT. */
bool chaon_lcd_show_usb_fault(const char *reason);
/* Single main-task draw before MSC enumeration. Does not touch SD media. */
bool chaon_lcd_show_msc(const char *volume_label, bool usage_valid,
                        uint64_t used_bytes, uint64_t total_bytes);
bool chaon_lcd_show(chaon_status_t status, uint32_t remaining_s,
                    chaon_sd_diag_t sd);
/* Main-task-only; transfers 160x8 pixels instead of a full screen. */
bool chaon_lcd_show_sd(chaon_sd_diag_t sd);
/* Main UI task only: passive read-only root entry preview. */
bool chaon_lcd_show_catalog(const chaon_fat_catalog_t *cat, uint8_t index);
/* Main-loop-only full-screen browser; no file reads or HID. */
bool chaon_lcd_show_browser(const chaon_fat_catalog_t *cat,
                            uint8_t index, bool picked);
bool chaon_lcd_show_browser_inspect(const chaon_fat_catalog_t *cat,
    uint8_t index, bool picked, const chaon_inspection_t *info);
#endif
