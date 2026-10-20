#ifndef CHAON_SCREEN_H
#define CHAON_SCREEN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "chaon_status.h"
#include "chaon_sd_diag.h"

enum { CHAON_SCREEN_WIDTH = 160, CHAON_SCREEN_HEIGHT = 80,
       CHAON_SCREEN_SD_Y = 72, CHAON_SCREEN_SD_HEIGHT = 8,
       CHAON_SCREEN_BYTES = CHAON_SCREEN_WIDTH * CHAON_SCREEN_HEIGHT * 2 };

/* Portable RGB565 big-endian framebuffer renderer; no allocation or SDK calls.
 * Returns false without touching memory for a short/NULL output buffer.
 */
bool chaon_screen_render(chaon_status_t status, uint32_t seconds_remaining,
                         chaon_sd_diag_t sd, uint8_t *buffer, size_t size);

/* Repaint only the reserved bottom strip; primary UI pixels are untouched. */
bool chaon_screen_render_sd(chaon_sd_diag_t sd, uint8_t *buffer, size_t size);

typedef struct {
    chaon_status_t status;
    uint32_t seconds;
    chaon_sd_diag_t sd;
} chaon_screen_key_t;
typedef enum {
    CHAON_SCREEN_NO_UPDATE, CHAON_SCREEN_FULL_UPDATE, CHAON_SCREEN_SD_UPDATE
} chaon_screen_update_t;
/* NULL previous means first frame. Countdown matters only when ARMED. */
chaon_screen_update_t chaon_screen_update(const chaon_screen_key_t *previous,
                                         const chaon_screen_key_t *current);

#endif
