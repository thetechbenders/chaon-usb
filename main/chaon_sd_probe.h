/* SPDX-License-Identifier: AGPL-3.0-only */
#ifndef CHAON_SD_PROBE_H
#define CHAON_SD_PROBE_H
#include "chaon_sd_diag.h"

/* Starts an optional, low-priority SDMMC observation task.
 * NO filesystem mount, file reads, disk writes or script execution.
 * Failure to start is advisory and must not prevent USB HID operation.
 */
void chaon_sd_probe_start(void);

/* Coherent, bounded task-context snapshot; no SD I/O or LCD calls. */
chaon_sd_diag_t chaon_sd_probe_snapshot(void);

#endif
