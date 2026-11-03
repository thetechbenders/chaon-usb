/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_msc_eject.h"
#include <limits.h>
#include <stddef.h>

bool chaon_msc_eject_ready(const chaon_msc_eject_t *s,
                            uint8_t lun, bool usb_owned)
{
    return s != NULL && lun == 0 && usb_owned && !s->ejected;
}

bool chaon_msc_eject_write_begin(chaon_msc_eject_t *s,
                                  uint8_t lun, bool usb_owned)
{
    if (!chaon_msc_eject_ready(s, lun, usb_owned) ||
        s->pending_writes == UINT32_MAX) return false;
    ++s->pending_writes;
    return true;
}

bool chaon_msc_eject_write_end(chaon_msc_eject_t *s)
{
    if (!s || s->pending_writes == 0) return false;
    --s->pending_writes;
    return true;
}

bool chaon_msc_eject_start_stop(chaon_msc_eject_t *s,
                                 uint8_t lun, bool start, bool load_eject)
{
    if (!s || lun != 0) return false;
    if (!load_eject) return !s->ejected;
    if (start) return !s->ejected; /* Allow a no-op START before eject. */
    if (s->pending_writes != 0) return false; /* No false-safe eject. */
    s->ejected = true;
    return true;
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
