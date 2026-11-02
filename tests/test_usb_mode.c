/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_usb_mode.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    for (int state = CHAON_MARKER_ABSENT; state <= CHAON_MARKER_IO_ERROR; ++state)
        for (int deleted = 0; deleted <= 1; ++deleted)
            for (int absent = 0; absent <= 1; ++absent) {
                chaon_usb_mode_t got = chaon_usb_mode_select(
                    (chaon_marker_state_t)state, deleted != 0, absent != 0);
                bool permitted = state == CHAON_MARKER_EMPTY_REGULAR &&
                                 deleted && absent;
                assert(got == (permitted ? CHAON_USB_HID : CHAON_USB_MSC));
            }
    puts("USB mode: 16 fail-cold combinations pass");
    return 0;
}
