#ifndef CHAON_VM_H
#define CHAON_VM_H
#include "chaon_script.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    /* Keyboard report; (modifier, usage)=(0,0) releases all keys. */
    bool (*report)(uint8_t modifier, uint8_t usage, void *context);
    /* Delay requested in short slices so cancellation remains responsive. */
    void (*sleep_ms)(uint32_t ms, void *context);
    /* Non-null return requests cancellation. */
    bool (*cancelled)(void *context);
    void *context;
} chaon_transport_t;

typedef enum {
    CHAON_RUN_OK, CHAON_RUN_CANCELLED, CHAON_RUN_IO_ERROR, CHAON_RUN_INVALID
} chaon_run_result_t;

/** Execute a validated program; always attempt final all-keys release.
 * Never retries a partially executed program automatically.
 */
chaon_run_result_t chaon_run(const chaon_program_t *program,
                            const chaon_transport_t *transport);
#endif
