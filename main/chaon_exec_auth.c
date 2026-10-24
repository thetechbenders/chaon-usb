/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_exec_auth.h"
#include <string.h>
void chaon_exec_auth_cancel(chaon_exec_auth_t *a) {
    if (a) memset(a, 0, sizeof *a);
}
bool chaon_exec_auth_begin(chaon_exec_auth_t *a, bool staged_valid,
    bool source_fresh, bool display_ok, bool usb_connected,
    bool boot_released, int64_t now_ms,
    uint32_t generation, uint32_t usb_epoch) {
    if (!a) return false;
    chaon_exec_auth_cancel(a);
    if (!staged_valid || !source_fresh || !display_ok || !usb_connected ||
        !boot_released || generation == 0 || usb_epoch == 0 || now_ms < 0)
        return false;
    a->state = CHAON_AUTH_RELEASE_WAIT;
    a->generation = generation;
    a->usb_epoch = usb_epoch;
    a->entered_ms = now_ms;
    return true;
}
bool chaon_exec_auth_confirm(chaon_exec_auth_t *a, bool usb_connected,
    bool selected, bool staged_valid, bool source_fresh,
    bool display_ok, uint32_t generation, uint32_t usb_epoch,
    bool boot_released, int64_t now_ms, int64_t press_duration_ms) {
    if (!a || a->state == CHAON_AUTH_IDLE) return false;
    if (!usb_connected || !selected || !staged_valid || !source_fresh ||
        !display_ok || !boot_released || generation != a->generation ||
        usb_epoch != a->usb_epoch || now_ms < a->entered_ms ||
        now_ms - a->entered_ms > CHAON_AUTH_WINDOW_MS) {
        chaon_exec_auth_cancel(a);
        return false;
    }
    if (a->state == CHAON_AUTH_RELEASE_WAIT) {
        /* Distinct release-acknowledgment event, not the confirmation press. */
        if (press_duration_ms != 0) { chaon_exec_auth_cancel(a); return false; }
        if (now_ms - a->entered_ms < CHAON_AUTH_RELEASE_MS) return false;
        a->state = CHAON_AUTH_CONFIRM_WAIT;
        a->released_ms = now_ms;
        return false;
    }
    const bool allowed = a->state == CHAON_AUTH_CONFIRM_WAIT &&
        now_ms >= a->released_ms &&
        press_duration_ms >= CHAON_AUTH_CONFIRM_MIN_MS &&
        press_duration_ms <= CHAON_AUTH_CONFIRM_MAX_MS;
    chaon_exec_auth_cancel(a);
    return allowed;
}
