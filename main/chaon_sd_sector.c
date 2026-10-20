/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_sd_sector.h"
#include <string.h>

chaon_sd_sector_kind_t chaon_sd_sector_classify(const uint8_t *s, size_t len)
{
    if (!s || len < 512 || s[510] != 0x55 || s[511] != 0xAA)
        return CHAON_SD_SECTOR_UNKNOWN;

    /* A plausible FAT VBR has a jump opcode and a FAT type marker. Treat
     * the markers as hints only, never trusted metadata or executable data.
     */
    if ((s[0] == 0xEB || s[0] == 0xE9) &&
        (memcmp(s + 54, "FAT12   ", 8) == 0 ||
         memcmp(s + 54, "FAT16   ", 8) == 0 ||
         memcmp(s + 82, "FAT32   ", 8) == 0))
        return CHAON_SD_SECTOR_FAT_BOOT;

    /* Four 16-byte MBR entries at offset 446. Do not read sector offsets,
     * sizes or filesystem records on the strength of this hint.
     */
    for (size_t entry = 0; entry < 4; ++entry) {
        const size_t off = 446 + entry * 16;
        if ((s[off] == 0x00 || s[off] == 0x80) &&
            s[off + 4] != 0 &&
            (s[off + 12] | s[off + 13] |
             s[off + 14] | s[off + 15]) != 0)
            return CHAON_SD_SECTOR_MBR;
    }
    return CHAON_SD_SECTOR_UNKNOWN;
}
