/* SPDX-License-Identifier: AGPL-3.0-only */
#ifndef CHAON_MSC_EJECT_H
#define CHAON_MSC_EJECT_H

#include <stdbool.h>
#include <stdint.h>

/* Pure, host-testable MSC media-availability policy.
 * Accesses must be serialized by the TinyUSB task. Production integration
 * uses synchronous physical SD writes, not asynchronous acknowledgments.
 * No application FAT mount, GPIO access, or USB operations belong here.
 */
typedef struct {
    bool ejected;
    uint32_t pending_writes;
} chaon_msc_eject_t;

/* Zero-initialized state means ready for LUN 0 if the backend is USB-owned. */
bool chaon_msc_eject_ready(const chaon_msc_eject_t *state,
                            uint8_t lun, bool usb_owned);

/* Native-model barrier: useful for testing pending-I/O rejection; the
 * production write callback is synchronous and does not enter this barrier. */
bool chaon_msc_eject_write_begin(chaon_msc_eject_t *state,
                                  uint8_t lun, bool usb_owned);
bool chaon_msc_eject_write_end(chaon_msc_eject_t *state);

/* Valid START STOP UNIT requests are acknowledged only if safe.
 * LOAD/EJECT + STOP latches ejected after outstanding writes finish.
 * LOAD/EJECT + START does not rearm media within a boot. Cold reboot rearms.
 */
bool chaon_msc_eject_start_stop(chaon_msc_eject_t *state,
                                 uint8_t lun, bool start, bool load_eject);
/* USB callback adapter, only call from the TinyUSB task. */
bool chaon_msc_device_ready(uint8_t lun, bool usb_owned);
bool chaon_msc_device_start_stop(uint8_t lun, bool start, bool load_eject);
#endif
