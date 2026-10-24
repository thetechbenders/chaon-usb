/* SPDX-License-Identifier: AGPL-3.0-only */
#ifndef CHAON_EXEC_AUTH_H
#define CHAON_EXEC_AUTH_H
#include <stdbool.h>
#include <stdint.h>
/* Portable, event-level authorization gate. This module never reads SD or sends HID. */
enum { CHAON_AUTH_ENTER_HOLD_MS = 5000, CHAON_AUTH_RELEASE_MS = 500,
       CHAON_AUTH_WINDOW_MS = 10000, CHAON_AUTH_CONFIRM_MIN_MS = 1500,
       CHAON_AUTH_CONFIRM_MAX_MS = 3000 };
typedef enum {
    CHAON_AUTH_IDLE, CHAON_AUTH_RELEASE_WAIT, CHAON_AUTH_CONFIRM_WAIT
} chaon_auth_state_t;
typedef struct {
    chaon_auth_state_t state;
    uint32_t generation;
    uint32_t usb_epoch;
    int64_t entered_ms;
    int64_t released_ms;
} chaon_exec_auth_t;
void chaon_exec_auth_cancel(chaon_exec_auth_t *a);
bool chaon_exec_auth_begin(chaon_exec_auth_t *a, bool staged_valid,
    bool source_fresh, bool display_ok, bool usb_connected,
    bool boot_released, int64_t now_ms,
    uint32_t generation, uint32_t usb_epoch);
/* Returns true exactly once after an independently classified physical confirmation.
 * The caller must atomically take/revalidate the execution-owned program before use. */
bool chaon_exec_auth_confirm(chaon_exec_auth_t *a, bool usb_connected,
    bool selected, bool staged_valid, bool source_fresh,
    bool display_ok, uint32_t generation, uint32_t usb_epoch,
    bool boot_released, int64_t now_ms, int64_t press_duration_ms);
#endif
