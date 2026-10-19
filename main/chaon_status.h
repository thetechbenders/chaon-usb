#ifndef CHAON_STATUS_H
#define CHAON_STATUS_H

#include <stdint.h>

typedef enum {
    CHAON_STATUS_WAIT_USB,
    CHAON_STATUS_READY,
    CHAON_STATUS_ARMED,
    CHAON_STATUS_RUNNING,
    CHAON_STATUS_CANCELLED,
    CHAON_STATUS_FAULT,
} chaon_status_t;

typedef struct {
    uint8_t red, green, blue;
} chaon_rgb_t;

/* Pure, clock-driven indicator policy. ARMED blinks at 2 Hz (250 ms on/off).
 * Cancellation is solid red, while faults blink red more rapidly.
 */
chaon_rgb_t chaon_status_led(chaon_status_t status, uint32_t now_ms);
const char *chaon_status_name(chaon_status_t status);
const char *chaon_status_hint(chaon_status_t status);

#endif
