/* SPDX-License-Identifier: AGPL-3.0-only */
#ifndef CHAON_USB_MARKER_H
#define CHAON_USB_MARKER_H
#include "chaon_usb_mode.h"
#include <sys/stat.h>
/* All hooks are synchronous and run before USB enumeration. The unmount hook
 * is always called after a successful mount, even if marker I/O fails. */
typedef struct {
    int (*stat_file)(void *ctx, const char *path, struct stat *info);
    int (*unlink_file)(void *ctx, const char *path);
    int (*unmount_card)(void *ctx);
} chaon_usb_marker_ops_t;
chaon_usb_mode_t chaon_usb_marker_finish(const chaon_usb_marker_ops_t *ops,
                                         void *ctx, bool *unmount_failed);
#endif
