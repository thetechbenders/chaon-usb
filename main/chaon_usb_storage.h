/* SPDX-License-Identifier: AGPL-3.0-only */
#ifndef CHAON_USB_STORAGE_H
#define CHAON_USB_STORAGE_H
#include "chaon_usb_mode.h"
#include "tinyusb_msc.h"
/* Runs before USB enumeration, with no other SD task running.
 * A missing or invalid marker selects MSC, but failed FAT mount/unmount\n * blocks USB entirely. Neither case may select HID. */
chaon_usb_mode_t chaon_usb_boot_mode(void);
/* Owns the SD host/card for the entire USB MSC personality.
 * Do NOT start the read-only HID SD worker in this personality. */
bool chaon_usb_storage_start(tinyusb_msc_storage_handle_t *out);
#endif
