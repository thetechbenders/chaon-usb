#include "chaon_control.h"

static void close_browser(chaon_control_t *c, bool pressed)
{
    c->confirm_active = false;
    c->confirm_release_ready = false;
    c->confirm_press_seen = false;
    c->browser_active = false;
    c->browser_selected = false;
    c->browser_index = 0;
    c->browser_count = 0;
    c->saw_press = false;
    /* A release after an invalidation or timeout must not arm HID. */
    c->suppress_release = pressed || c->stable_pressed ||
                          c->candidate_pressed;
}

chaon_control_event_t chaon_control_step_confirmation(chaon_control_t *c,
    bool usb_mounted, bool pressed, int64_t now_ms,
    bool catalogue_available, uint8_t catalogue_count,
    bool eligible)
{
    if (!c) return CHAON_CONTROL_NONE;
    if (catalogue_count > CHAON_CONTROL_BROWSER_MAX_ENTRIES)
        catalogue_available = false;

    chaon_control_event_t event = CHAON_CONTROL_NONE;
    if (c->confirm_active && (!usb_mounted || !eligible ||
        !catalogue_available || !catalogue_count ||
        catalogue_count != c->browser_count ||
        now_ms < c->confirm_entered_ms ||
        now_ms - c->confirm_entered_ms >= CHAON_CONTROL_CONFIRM_TTL_MS)) {
        close_browser(c, pressed);
        return CHAON_CONTROL_CONFIRM_CANCEL;
    }
    if (!usb_mounted) {
        c->state = CHAON_STATUS_WAIT_USB;
        c->saw_press = false;
        c->suppress_release = pressed;
        c->browser_active = false;
        c->browser_selected = false;
        c->confirm_active = false;
        c->confirm_release_ready = false;
        c->confirm_press_seen = false;
        c->browser_index = 0;
        c->browser_count = 0;
    } else if (c->state == CHAON_STATUS_WAIT_USB) {
        c->state = CHAON_STATUS_READY;
        c->saw_press = false;
        c->suppress_release = pressed; /* held on reconnect is not a gesture */
    }

    if (c->browser_active && (!catalogue_available ||
        !catalogue_count || catalogue_count != c->browser_count ||
        c->state != CHAON_STATUS_READY ||
        now_ms - c->browser_activity_ms >= CHAON_CONTROL_BROWSER_IDLE_MS)) {
        close_browser(c, pressed);
        event = CHAON_CONTROL_BROWSER_CLOSE;
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

    /* Existing 20 ms polling and 40 ms stable-edge debounce semantics. */
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
            if (c->confirm_active) {
                if (now_ms - c->confirm_entered_ms < 500 ||
                    !c->confirm_release_ready) {
                    close_browser(c, true);
                    return CHAON_CONTROL_CONFIRM_CANCEL;
                }
                c->confirm_press_seen = true;
                c->confirm_pressed_ms = now_ms;
            }
            c->saw_press = usb_mounted && !c->suppress_release &&
                (c->browser_active || c->state == CHAON_STATUS_READY ||
                 c->state == CHAON_STATUS_ARMED);
            if (c->saw_press) c->pressed_at_ms = now_ms;
        } else {
            if (c->suppress_release) {
                c->suppress_release = false;
            } else if (c->saw_press && usb_mounted) {
                const int64_t duration = now_ms - c->pressed_at_ms;
                if (duration >= 0 && c->confirm_active) {
                    /* Confirm is deliberately inert until source freshness and
                     * atomic execution ownership exist in a later milestone. */
                    const bool valid_press = c->confirm_release_ready &&
                        c->confirm_press_seen &&
                        duration >= 1500 && duration <= 3000;
                    close_browser(c, false);
                    event = valid_press ? CHAON_CONTROL_CONFIRM_ACK :
                        CHAON_CONTROL_CONFIRM_CANCEL;
                } else if (duration >= 0 && c->browser_active) {
                    c->browser_activity_ms = now_ms;
                    if (duration >= CHAON_CONTROL_CONFIRM_ENTER_MS &&
                        c->browser_selected && eligible) {
                        c->confirm_active = true;
                        c->confirm_release_ready = false;
                        c->confirm_press_seen = false;
                        c->confirm_entered_ms = now_ms;
                        c->browser_activity_ms = now_ms;
                        event = CHAON_CONTROL_CONFIRM_ENTER;
                    } else if (duration >= CHAON_CONTROL_BROWSER_EXIT_MS) {
                        close_browser(c, false);
                        event = CHAON_CONTROL_BROWSER_CLOSE;
                    } else if (duration >= CHAON_CONTROL_BROWSER_HOLD_MS) {
                        c->browser_selected = !c->browser_selected;
                        event = CHAON_CONTROL_BROWSER_SELECT;
                    } else if (c->browser_count) {
                        c->browser_index = (uint8_t)
                            ((c->browser_index + 1U) % c->browser_count);
                        c->browser_selected = false;
                        event = CHAON_CONTROL_BROWSER_NEXT;
                    }
                } else if (duration >= 0 &&
                           duration >= CHAON_CONTROL_BROWSER_HOLD_MS &&
                           c->state == CHAON_STATUS_READY) {
                    /* Consume long holds: they can NEVER become ARM/EXECUTE.
                     * A missing SD catalogue does not enable another action. */
                    if (catalogue_available && catalogue_count) {
                        c->browser_active = true;
                        c->browser_selected = false;
                        c->browser_index = 0;
                        c->browser_count = catalogue_count;
                        c->browser_activity_ms = now_ms;
                        event = CHAON_CONTROL_BROWSER_ENTER;
                    }
                } else if (duration >= 0 &&
                           c->state == CHAON_STATUS_READY) {
                    c->state = CHAON_STATUS_ARMED;
                    c->armed_at_ms = now_ms;
                    event = CHAON_CONTROL_ARM;
                } else if (c->state == CHAON_STATUS_ARMED) {
                    if (duration >= 0 &&
                        duration < CHAON_CONTROL_BROWSER_HOLD_MS) {
                        c->state = CHAON_STATUS_RUNNING;
                        event = CHAON_CONTROL_EXECUTE;
                    } else {
                        c->state = CHAON_STATUS_READY;
                        event = CHAON_CONTROL_DISARM;
                    }
                }
            }
            c->saw_press = false;
        }
    }
    if (c->confirm_active && !c->stable_pressed &&
        !c->candidate_pressed &&
        now_ms - c->confirm_entered_ms >= 500)
        c->confirm_release_ready = true;
    return event;
}

chaon_control_event_t chaon_control_step_browser(chaon_control_t *c,
    bool usb_mounted, bool pressed, int64_t now_ms,
    bool catalogue_available, uint8_t catalogue_count)
{
    return chaon_control_step_confirmation(c, usb_mounted, pressed, now_ms,
        catalogue_available, catalogue_count, false);
}

chaon_control_event_t chaon_control_step(chaon_control_t *c,
                                         bool usb_mounted,
                                         bool pressed,
                                         int64_t now_ms)
{
    return chaon_control_step_browser(c, usb_mounted, pressed, now_ms,
                                      false, 0);
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
        /* Failed all-keys release is FAULT, never success. */
        c->state = CHAON_STATUS_FAULT;
    } else {
        c->state = CHAON_STATUS_READY;
    }

    c->browser_active = false;
    c->browser_selected = false;
    c->confirm_active = false;
    c->confirm_release_ready = false;
    c->confirm_press_seen = false;
    c->browser_count = 0;
    c->browser_index = 0;
    c->saw_press = false;
    /* Never reinterpret the held cancellation button's release as ARM. */
    c->stable_pressed = pressed;
    c->candidate_pressed = pressed;
    c->suppress_release = pressed;
    c->debounce_ms = 0;
}
