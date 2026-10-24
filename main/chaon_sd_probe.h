/* SPDX-License-Identifier: AGPL-3.0-only */
#ifndef CHAON_SD_PROBE_H
#define CHAON_SD_PROBE_H
#include "chaon_sd_diag.h"
#include "chaon_fat_ro.h"
#include "chaon_inspect.h"
#include "chaon_script.h"
#include <stdbool.h>

/* Starts an optional, low-priority SDMMC observation task.
 * No filesystem mount or disk writes. This task never calls the VM or HID;
 * it publishes only bounded staged data and fresh-source verification.
 * Optional bounded read-only .CHN validation runs in this task only.
 * Failure to start is advisory and must not prevent USB HID operation.
 */
void chaon_sd_probe_start(void);

/* Coherent, bounded task-context snapshot; no SD I/O or LCD calls. */
chaon_sd_diag_t chaon_sd_probe_snapshot(void);
/* Copy at most 8 directory entries under a short SMP lock. The snapshot is
 * advisory and cannot affect the HID state machine. No FS writes exist. */
bool chaon_sd_probe_catalog_snapshot(chaon_fat_catalog_t *result);

/* Advisory summary-only file inspection. Never exposes source data or
 * a compiled program. Selection identity must match the current snapshot. */
bool chaon_sd_probe_inspect_request(uint8_t index,
                                    const chaon_fat_entry_t *expected);
bool chaon_sd_probe_inspect_snapshot(uint8_t index,
    const chaon_fat_entry_t *expected, chaon_inspection_t *out);
void chaon_sd_probe_inspect_clear(void);

/* Coherent digest-bearing stage copy; returns validation evidence only. */
bool chaon_sd_probe_staged_program_copy_hashed(uint8_t index,
    const chaon_fat_entry_t *expected, uint32_t *generation,
    chaon_program_t *out, uint8_t digest[32]);

/* Nonblocking last-moment source verification, exclusively performed in the
 * SD task. Result is one-use and scoped to a strictly increasing request ID.
 * A true result certifies the exact source bytes at the completed read,
 * not indefinite card presence. No API here performs HID/VM operations. */
typedef enum {
    CHAON_SD_VERIFY_WAIT,
    CHAON_SD_VERIFY_PASS,
    CHAON_SD_VERIFY_FAIL
} chaon_sd_verify_result_t;
uint32_t chaon_sd_probe_verify_request(uint8_t index,
    const chaon_fat_entry_t *expected, uint32_t generation,
    const uint8_t digest[32]);
chaon_sd_verify_result_t chaon_sd_probe_verify_poll(uint32_t request_id);
void chaon_sd_probe_verify_cancel(void);

/* Bounded copy-out interface for a future, separately authorized control
 * feature. A successful copy is validation evidence, NOT execution authority.
 * Current app_main intentionally has no caller and still runs only its
 * embedded fixed demo. Callers must provide non-stack fixed storage. */
bool chaon_sd_probe_staged_program_copy(uint8_t index,
    const chaon_fat_entry_t *expected, uint32_t *generation,
    chaon_program_t *out);

#endif
