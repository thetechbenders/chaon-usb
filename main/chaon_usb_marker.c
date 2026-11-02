/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_usb_marker.h"
#include <errno.h>

chaon_usb_mode_t chaon_usb_marker_finish(const chaon_usb_marker_ops_t *ops,
                                         void *ctx, bool *unmount_failed)
{
    if (unmount_failed) *unmount_failed = true;
    if (!ops || !ops->stat_file || !ops->unlink_file ||
        !ops->unmount_card) return CHAON_USB_BLOCKED;
    const char *path = "/chaon/DUCKY";
    struct stat info;
    chaon_marker_state_t marker = CHAON_MARKER_IO_ERROR;
    bool removed = false, absent_after = false;
    errno = 0;
    if (ops->stat_file(ctx, path, &info) == 0) {
        marker = (S_ISREG(info.st_mode) && info.st_size == 0)
                 ? CHAON_MARKER_EMPTY_REGULAR : CHAON_MARKER_INVALID;
        if (marker == CHAON_MARKER_EMPTY_REGULAR &&
            ops->unlink_file(ctx, path) == 0) {
            removed = true;
            errno = 0;
            absent_after = (ops->stat_file(ctx, path, &info) < 0 &&
                            errno == ENOENT);
        }
    } else if (errno == ENOENT) {
        marker = CHAON_MARKER_ABSENT;
    }
    if (ops->unmount_card(ctx) != 0) return CHAON_USB_BLOCKED;
    if (unmount_failed) *unmount_failed = false;
    return chaon_usb_mode_select(marker, removed, absent_after);
}
