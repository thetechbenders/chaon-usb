/* SPDX-License-Identifier: AGPL-3.0-only */
#ifndef CHAON_USB_MODE_H
#define CHAON_USB_MODE_H
#include <stdbool.h>
#include <stddef.h>
/* A personality is fixed before installing USB. Never select HID unless the
 * exact empty marker was consumed and its absence was verified. */
typedef enum {
    CHAON_USB_MSC = 0,
    CHAON_USB_HID = 1
} chaon_usb_mode_t;
typedef enum {
    CHAON_MARKER_ABSENT,
    CHAON_MARKER_EMPTY_REGULAR,
    CHAON_MARKER_INVALID,
    CHAON_MARKER_IO_ERROR
} chaon_marker_state_t;
chaon_usb_mode_t chaon_usb_mode_select(chaon_marker_state_t marker,
                                       bool delete_ok, bool absent_after);
#endif
