/* SPDX-License-Identifier: AGPL-3.0-only */
#ifndef CHAON_SD_PROBE_H
#define CHAON_SD_PROBE_H
#include "chaon_sd_diag.h"
#include "chaon_fat_ro.h"
#include <stdbool.h>

/* Starts an optional, low-priority SDMMC observation task.
 * NO filesystem mount, file reads, disk writes or script execution.
 * Failure to start is advisory and must not prevent USB HID operation.
 */
void chaon_sd_probe_start(void);

/* Coherent, bounded task-context snapshot; no SD I/O or LCD calls. */
chaon_sd_diag_t chaon_sd_probe_snapshot(void);
/* Copy at most 8 directory entries under a short SMP lock. The snapshot is
 * advisory and cannot affect the HID state machine. No FS writes exist. */
bool chaon_sd_probe_catalog_snapshot(chaon_fat_catalog_t *result);

#endif
