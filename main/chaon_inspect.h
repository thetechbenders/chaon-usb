/* SPDX-License-Identifier: AGPL-3.0-only */
#ifndef CHAON_INSPECT_H
#define CHAON_INSPECT_H
#include <stddef.h>
#include <stdint.h>
#include "chaon_script.h"

/* Diagnostic-only output. No instruction, source bytes or executable
 * program ever crosses this interface into the main/HID task. */
typedef enum {
    CHAON_INSPECT_NONE, CHAON_INSPECT_PENDING, CHAON_INSPECT_VALID,
    CHAON_INSPECT_BAD_SOURCE, CHAON_INSPECT_BAD_SYNTAX,
    CHAON_INSPECT_NOT_SCRIPT, CHAON_INSPECT_TOO_LARGE,
    CHAON_INSPECT_STALE, CHAON_INSPECT_IO_ERROR,
    CHAON_INSPECT_BAD_MEDIA
} chaon_inspect_state_t;
typedef enum {
    CHAON_INSPECT_OP_NONE, CHAON_INSPECT_OP_TYPE,
    CHAON_INSPECT_OP_DELAY, CHAON_INSPECT_OP_ENTER,
    CHAON_INSPECT_OP_TAB
} chaon_inspect_op_t;
typedef struct {
    chaon_inspect_state_t state;
    uint8_t step_count;
    uint16_t error_line;
    chaon_inspect_op_t first_op;
} chaon_inspection_t;

/* Caller-owned parser workspace. The SD task keeps one static instance so
 * the 2048-byte source copy never lands on a FreeRTOS task stack or crosses
 * a task boundary. The compile function wipes it on every return path. */
typedef struct {
    char source[CHAON_MAX_SOURCE];
} chaon_inspect_workspace_t;

/* Validate and compile into caller-owned fixed storage. `out` is cleared on
 * entry and remains empty on every failure. A valid result is the only case
 * in which `out` contains a program. No VM or HID operation is performed. */
chaon_inspection_t chaon_inspect_compile_bytes(const uint8_t *source,
    size_t length, chaon_inspect_workspace_t *workspace,
    chaon_program_t *out);

/* Text must be ASCII-safe, have no embedded NUL, and be terminated at
 * source[length]. This summary-only compatibility entry point uses private
 * fixed scratch which is erased before return; firmware staging uses the
 * explicit caller-owned workspace above.
 */
chaon_inspection_t chaon_inspect_bytes(const uint8_t *source, size_t length);
#endif
