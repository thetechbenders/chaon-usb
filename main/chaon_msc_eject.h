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
} chaon_msc_eject_t;

/* Zero-initialized state means ready for LUN 0 if the backend is USB-owned. */
bool chaon_msc_eject_ready(const chaon_msc_eject_t *state,
                            uint8_t lun, bool usb_owned);

/* Valid START STOP UNIT requests are acknowledged only if safe.
 * LOAD/EJECT + STOP latches ejected. WRITE10 is synchronous in the USB task.
 * LOAD/EJECT + START does not rearm media within a boot. Cold reboot rearms.
 */
bool chaon_msc_eject_start_stop(chaon_msc_eject_t *state,
                                 uint8_t lun, bool start, bool load_eject);
/* Validate whole-sector transfers and in-bounds ranges before SDMMC access.
 * Never allow zero-byte, sub-sector, unaligned, or overflowing writes. */
bool chaon_msc_transfer_valid(uint32_t lba, uint32_t offset,
                              uint32_t bytes, uint32_t sector_count,
                              uint32_t sector_size);

/* USB callback adapter, only call from the TinyUSB task. */
bool chaon_msc_device_ready(uint8_t lun, bool usb_owned);
bool chaon_msc_device_start_stop(uint8_t lun, bool start, bool load_eject);
#endif
