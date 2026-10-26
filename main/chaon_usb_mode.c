/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_usb_mode.h"
chaon_usb_mode_t chaon_usb_mode_select(chaon_marker_state_t marker,
                                       bool delete_ok, bool absent_after)
{
    return marker == CHAON_MARKER_EMPTY_REGULAR && delete_ok && absent_after
           ? CHAON_USB_HID : CHAON_USB_MSC;
}
