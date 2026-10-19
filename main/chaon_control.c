#include "chaon_control.h"

chaon_control_event_t chaon_control_step(chaon_control_t *c,
                                         bool usb_mounted,
                                         bool pressed,
                                         int64_t now_ms)
{
    if (!c) return CHAON_CONTROL_NONE;

    chaon_control_event_t event = CHAON_CONTROL_NONE;
    if (!usb_mounted) {
        c->state = CHAON_STATUS_WAIT_USB;
        c->saw_press = false;
        c->suppress_release = pressed;
    } else if (c->state == CHAON_STATUS_WAIT_USB) {
        c->state = CHAON_STATUS_READY;
        c->saw_press = false;
        c->suppress_release = pressed; /* held on reconnect is not a gesture */
    }

    if (c->state == CHAON_STATUS_ARMED &&
        now_ms - c->armed_at_ms >= CHAON_CONTROL_ARM_TTL_MS) {
        c->state = CHAON_STATUS_READY;
        c->saw_press = false;
        event = CHAON_CONTROL_EXPIRE;
    }
    if (c->state == CHAON_STATUS_CANCELLED &&
        now_ms >= c->cancelled_until_ms) {
        c->state = CHAON_STATUS_READY;
    }

    /* Keep the original 20 ms polling / 40 ms debounce semantics. */
    if (pressed != c->candidate_pressed) {
        c->candidate_pressed = pressed;
        c->debounce_ms = 0;
    } else if (c->debounce_ms < CHAON_CONTROL_DEBOUNCE_MS) {
        c->debounce_ms += CHAON_CONTROL_POLL_MS;
    }
    if (c->debounce_ms >= CHAON_CONTROL_DEBOUNCE_MS &&
        c->stable_pressed != c->candidate_pressed) {
        c->stable_pressed = c->candidate_pressed;
        if (c->stable_pressed) {
            c->saw_press = usb_mounted && !c->suppress_release &&
                           (c->state == CHAON_STATUS_READY ||
                            c->state == CHAON_STATUS_ARMED);
        } else {
            if (c->suppress_release) {
                c->suppress_release = false;
            } else if (c->saw_press && usb_mounted) {
                if (c->state == CHAON_STATUS_READY) {
                    c->state = CHAON_STATUS_ARMED;
                    c->armed_at_ms = now_ms;
                    event = CHAON_CONTROL_ARM;
                } else if (c->state == CHAON_STATUS_ARMED) {
                    c->state = CHAON_STATUS_RUNNING;
                    event = CHAON_CONTROL_EXECUTE;
                }
            }
            c->saw_press = false;
        }
    }
    return event;
}

void chaon_control_complete(chaon_control_t *c,
                             chaon_run_result_t result,
                             bool usb_mounted,
                             bool pressed,
                             int64_t completed_ms)
{
    if (!c || c->state != CHAON_STATUS_RUNNING) return;

    if (!usb_mounted) {
        c->state = CHAON_STATUS_WAIT_USB;
    } else if (result == CHAON_RUN_CANCELLED) {
        c->state = CHAON_STATUS_CANCELLED;
        c->cancelled_until_ms = completed_ms + CHAON_CONTROL_CANCEL_HOLD_MS;
    } else if (result != CHAON_RUN_OK) {
        /* Failed final key release MUST be reported as FAULT, not
         * a successful cancellation. The host may still hold a key. */
        c->state = CHAON_STATUS_FAULT;
    } else {
        c->state = CHAON_STATUS_READY;
    }

    /* Preserve the original physical cancellation/release suppression.
     * Never reinterpret the held cancel button's eventual release as ARM. */
    c->stable_pressed = pressed;
    c->candidate_pressed = pressed;
    c->suppress_release = pressed;
    c->debounce_ms = 0;
}
