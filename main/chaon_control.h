#ifndef CHAON_CONTROL_H
#define CHAON_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

#include "chaon_status.h"
#include "chaon_vm.h"

/* Pure BOOT-button controller. The I/O shell must sample button/USB inputs,
 * call step() once per poll, and run the VM only on EXECUTE.
 */
enum {
    CHAON_CONTROL_POLL_MS = 20,
    CHAON_CONTROL_DEBOUNCE_MS = 40,
    CHAON_CONTROL_ARM_TTL_MS = 30000,
    CHAON_CONTROL_CANCEL_HOLD_MS = 2000
};

typedef enum {
    CHAON_CONTROL_NONE,
    CHAON_CONTROL_ARM,
    CHAON_CONTROL_EXPIRE,
    CHAON_CONTROL_EXECUTE
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
} chaon_control_t;

/* A zero-initialized controller starts in WAIT_USB. */
chaon_control_event_t chaon_control_step(chaon_control_t *control,
                                         bool usb_mounted,
                                         bool boot_pressed,
                                         int64_t now_ms);

/* Called once by the I/O shell after an EXECUTE event and blocking VM run.
 * Resynchronizes the physical button so the cancel press cannot re-arm.
 */
void chaon_control_complete(chaon_control_t *control,
                             chaon_run_result_t result,
                             bool usb_mounted,
                             bool boot_pressed,
                             int64_t completed_ms);

#endif
