/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_msc_eject.h"
#include <stddef.h>

bool chaon_msc_eject_ready(const chaon_msc_eject_t *s,
                            uint8_t lun, bool usb_owned)
{
    return s != NULL && lun == 0 && usb_owned && !s->ejected;
}

bool chaon_msc_eject_start_stop(chaon_msc_eject_t *s,
                                 uint8_t lun, bool start, bool load_eject)
{
    if (!s || lun != 0) return false;
    if (!load_eject) return !s->ejected;
    if (start) return !s->ejected; /* Allow a no-op START before eject. */
    s->ejected = true;
    return true;
}

bool chaon_msc_transfer_valid(uint32_t lba, uint32_t offset,
                              uint32_t bytes, uint32_t sectors,
                              uint32_t sector_size)
{
    if (sector_size != 512u || offset != 0 || bytes == 0 ||
        bytes % sector_size != 0 || lba >= sectors) return false;
    return bytes / sector_size <= sectors - lba;
}

/* The patched pinned MSC callbacks execute these operations in the TinyUSB
 * task. No ISR or other task is permitted to mutate this state. */
static chaon_msc_eject_t device_state;

bool chaon_msc_device_ready(uint8_t lun, bool usb_owned)
{
    return chaon_msc_eject_ready(&device_state, lun, usb_owned);
}

bool chaon_msc_device_start_stop(uint8_t lun, bool start, bool load_eject)
{
    return chaon_msc_eject_start_stop(&device_state, lun, start, load_eject);
}
