#ifndef CHAON_SCREEN_H
#define CHAON_SCREEN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "chaon_status.h"

enum { CHAON_SCREEN_WIDTH = 160, CHAON_SCREEN_HEIGHT = 80,
       CHAON_SCREEN_BYTES = CHAON_SCREEN_WIDTH * CHAON_SCREEN_HEIGHT * 2 };

/* Portable RGB565 big-endian framebuffer renderer; no allocation or SDK calls.
 * Returns false without touching memory for a short/NULL output buffer.
 */
bool chaon_screen_render(chaon_status_t status, uint32_t seconds_remaining,
                         uint8_t *buffer, size_t size);

#endif
