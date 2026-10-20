/* SPDX-License-Identifier: AGPL-3.0-only */
#ifndef CHAON_SD_SECTOR_H
#define CHAON_SD_SECTOR_H

#include <stddef.h>
#include <stdint.h>

/* Sector-0 signature hints only. These are NOT validated FAT mounts or
 * evidence that any file/directory data can safely be accessed.
 */
typedef enum {
    CHAON_SD_SECTOR_UNKNOWN = 0,
    CHAON_SD_SECTOR_MBR,
    CHAON_SD_SECTOR_FAT_BOOT
} chaon_sd_sector_kind_t;

/* Never reads beyond len, even on malformed/untrusted sector contents. */
chaon_sd_sector_kind_t chaon_sd_sector_classify(const uint8_t *sector,
                                                size_t len);

#endif
