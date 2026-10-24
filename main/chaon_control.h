#ifndef CHAON_CONTROL_H
#define CHAON_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

#include "chaon_status.h"
#include "chaon_vm.h"

/* All BOOT gestures are classified in this controller, never in the UI.
 * The normal short-tap/short-tap fixed HID demo remains unchanged.
 */
enum {
    CHAON_CONTROL_POLL_MS = 20,
    CHAON_CONTROL_DEBOUNCE_MS = 40,
    CHAON_CONTROL_ARM_TTL_MS = 30000,
    CHAON_CONTROL_CANCEL_HOLD_MS = 2000,
    CHAON_CONTROL_BROWSER_HOLD_MS = 900,
    CHAON_CONTROL_BROWSER_EXIT_MS = 3000,
    CHAON_CONTROL_BROWSER_IDLE_MS = 60000,
    CHAON_CONTROL_BROWSER_MAX_ENTRIES = 8,
    CHAON_CONTROL_CONFIRM_ENTER_MS = 5000,
    CHAON_CONTROL_CONFIRM_TTL_MS = 10000
};

typedef enum {
    CHAON_CONTROL_NONE,
    CHAON_CONTROL_ARM,
    CHAON_CONTROL_EXPIRE,
    CHAON_CONTROL_EXECUTE,
    CHAON_CONTROL_DISARM,
    CHAON_CONTROL_BROWSER_ENTER,
    CHAON_CONTROL_BROWSER_NEXT,
    CHAON_CONTROL_BROWSER_SELECT,
    CHAON_CONTROL_PICK_READY,
    CHAON_CONTROL_BROWSER_CLOSE,
    CHAON_CONTROL_CONFIRM_ENTER,
    CHAON_CONTROL_CONFIRM_CANCEL,
    CHAON_CONTROL_CONFIRM_ACK /* Inert confirmation only, NEVER VM execution */
} chaon_control_event_t;

typedef struct {
    chaon_status_t state;
    bool stable_pressed;
    bool candidate_pressed;
    bool saw_press;
    bool suppress_release;
    int debounce_ms;
    int64_t armed_at_ms;
    int64_t cancelled_until_ms;
    int64_t pressed_at_ms;
    /* Advisory browsing ONLY. Neither selection nor SD metadata is ever
     * used to authorize a VM or USB HID report. */
    bool confirm_active;
    bool confirm_release_ready;
    bool confirm_press_seen;
    int64_t confirm_entered_ms;
    int64_t confirm_pressed_ms;
    bool browser_active;
    bool browser_selected;
    uint8_t browser_index;
    uint8_t browser_count;
    int64_t browser_activity_ms;
} chaon_control_t;

/* Backward-compatible fixed HID demo entry point. No filesystem browsing. */
chaon_control_event_t chaon_control_step(chaon_control_t *control,
                                         bool usb_mounted,
                                         bool boot_pressed,
                                         int64_t now_ms);

/* Application entry point: catalogue presence/count control only the passive
 * browser. The only EXECUTE event is still the two-short-tap fixed demo.
 * Never accept SD file bytes or path strings in this controller.
 */
chaon_control_event_t chaon_control_step_browser(chaon_control_t *control,
    bool usb_mounted, bool boot_pressed, int64_t now_ms,
    bool catalogue_available, uint8_t catalogue_count);

/* The caller sets eligibility from the current VALID inspection and LCD state.
 * No confirmation event authorizes or triggers VM/HID. */
chaon_control_event_t chaon_control_step_confirmation(chaon_control_t *control,
    bool usb_mounted, bool boot_pressed, int64_t now_ms,
    bool catalogue_available, uint8_t catalogue_count,
    bool valid_inspection_and_display);

void chaon_control_complete(chaon_control_t *control,
                             chaon_run_result_t result,
                             bool usb_mounted,
                             bool boot_pressed,
                             int64_t completed_ms);
#endif
