/* SPDX-License-Identifier: AGPL-3.0-only */
#ifndef CHAON_USB_STORAGE_H
#define CHAON_USB_STORAGE_H
#include "chaon_usb_mode.h"
#include "tinyusb_msc.h"
#include <stdint.h>
#include <stdbool.h>
/* Snapshot taken only while ChaoN owns the FAT mount, before USB starts. */
typedef struct {
    bool valid;
    uint64_t used_bytes;
    uint64_t total_bytes;
    char volume_label[12]; /* FAT short-label, sanitized for 5x7 font */
} chaon_usb_volume_t;
const chaon_usb_volume_t *chaon_usb_volume_snapshot(void);
/* Boot failure diagnostic, not a storage operation. */
const char *chaon_usb_boot_fault_reason(void);
/* Runs before USB enumeration, with no other SD task running.
 * A missing or invalid marker selects MSC, but failed FAT mount/unmount\n * blocks USB entirely. Neither case may select HID. */
chaon_usb_mode_t chaon_usb_boot_mode(void);
/* Owns the SD host/card for the entire USB MSC personality.
 * Do NOT start the read-only HID SD worker in this personality. */
bool chaon_usb_storage_start(tinyusb_msc_storage_handle_t *out);
#endif
