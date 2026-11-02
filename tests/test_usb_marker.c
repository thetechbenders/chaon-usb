/* SPDX-License-Identifier: AGPL-3.0-only */
#define _DEFAULT_SOURCE
#include "chaon_usb_marker.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    int first_errno, after_errno, unlink_errno;
    bool exists, nonempty, directory, after_exists, unmount_error;
    int stats, unlinks, unmounts;
} fixture_t;
static int stub_stat(void *ctx, const char *path, struct stat *info)
{
    fixture_t *f = ctx;
    assert(strcmp(path, "/chaon/DUCKY") == 0);
    memset(info, 0, sizeof *info);
    ++f->stats;
    if (f->stats > 1) {
        if (f->after_exists) { info->st_mode = S_IFREG; return 0; }
        errno = f->after_errno ? f->after_errno : ENOENT;
        return -1;
    }
    if (!f->exists) { errno = f->first_errno ? f->first_errno : ENOENT; return -1; }
    info->st_mode = f->directory ? S_IFDIR : S_IFREG;
    info->st_size = f->nonempty ? 1 : 0;
    return 0;
}
static int stub_unlink(void *ctx, const char *path)
{
    fixture_t *f = ctx;
    assert(strcmp(path, "/chaon/DUCKY") == 0);
    ++f->unlinks;
    if (f->unlink_errno) { errno = f->unlink_errno; return -1; }
    return 0;
}
static int stub_unmount(void *ctx)
{
    fixture_t *f = ctx;
    ++f->unmounts;
    return f->unmount_error ? -1 : 0;
}
static void check(fixture_t *f, chaon_usb_mode_t expected, int expected_unlinks)
{
    const chaon_usb_marker_ops_t ops = {stub_stat, stub_unlink, stub_unmount};
    bool unmount_failed = true;
    assert(chaon_usb_marker_finish(&ops, f, &unmount_failed) == expected);
    assert(unmount_failed == f->unmount_error);
    assert(f->unmounts == 1);
    assert(f->unlinks == expected_unlinks);
}
int main(void)
{
    fixture_t f = {0};
    check(&f, CHAON_USB_MSC, 0); /* absent */
    f = (fixture_t){.exists=true};
    check(&f, CHAON_USB_HID, 1); /* deleted, confirmed absent */
    f = (fixture_t){.exists=true,.nonempty=true};
    check(&f, CHAON_USB_MSC, 0);
    f = (fixture_t){.exists=true,.directory=true};
    check(&f, CHAON_USB_MSC, 0);
    f = (fixture_t){.first_errno=EIO};
    check(&f, CHAON_USB_MSC, 0);
    f = (fixture_t){.exists=true,.unlink_errno=EROFS};
    check(&f, CHAON_USB_MSC, 1);
    f = (fixture_t){.exists=true,.after_exists=true};
    check(&f, CHAON_USB_MSC, 1);
    f = (fixture_t){.exists=true,.after_errno=EIO};
    check(&f, CHAON_USB_MSC, 1);
    f = (fixture_t){.exists=true,.unmount_error=true};
    check(&f, CHAON_USB_BLOCKED, 1);
    f = (fixture_t){.first_errno=EIO,.unmount_error=true};
    check(&f, CHAON_USB_BLOCKED, 0);
    puts("MSC marker transaction tests passed");
}
