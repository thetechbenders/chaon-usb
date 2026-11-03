/* SPDX-License-Identifier: AGPL-3.0-only */
#ifndef CHAON_MSC_EJECT_H
#define CHAON_MSC_EJECT_H

#include <stdbool.h>
#include <stdint.h>

/* Pure, host-testable MSC media-availability policy.
 * Integration must serialize updates with TinyUSB callbacks and deferred
 * write completion (or provide a separately reviewed synchronization scheme).
 * No application FAT mount, GPIO access, or USB operations belong here.
 */
typedef struct {
    bool ejected;
    uint32_t pending_writes;
} chaon_msc_eject_t;

/* Zero-initialized state means ready for LUN 0 if the backend is USB-owned. */
bool chaon_msc_eject_ready(const chaon_msc_eject_t *state,
                            uint8_t lun, bool usb_owned);

/* Reserve before deferring a write; release exactly once on completion. */
bool chaon_msc_eject_write_begin(chaon_msc_eject_t *state,
                                  uint8_t lun, bool usb_owned);
bool chaon_msc_eject_write_end(chaon_msc_eject_t *state);

/* Valid START STOP UNIT requests are acknowledged only if safe.
 * LOAD/EJECT + STOP latches ejected after all deferred writes finish.
 * LOAD/EJECT + START does not rearm media within a boot. Reconnect/reset
 * policies must be reviewed explicitly before integration.
 */
bool chaon_msc_eject_start_stop(chaon_msc_eject_t *state,
                                 uint8_t lun, bool start, bool load_eject);
#endif
