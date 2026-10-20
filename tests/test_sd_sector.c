/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_sd_sector.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void test_invalid(void)
{
    uint8_t s[514] = {0};
    s[512] = 0xCC;
    s[513] = 0xDD;
    assert(chaon_sd_sector_classify(NULL, 512) == CHAON_SD_SECTOR_UNKNOWN);
    assert(chaon_sd_sector_classify(s, 0) == CHAON_SD_SECTOR_UNKNOWN);
    assert(chaon_sd_sector_classify(s, 511) == CHAON_SD_SECTOR_UNKNOWN);
    assert(chaon_sd_sector_classify(s, 512) == CHAON_SD_SECTOR_UNKNOWN);
    s[510] = 0x55;
    s[511] = 0xAA;
    assert(chaon_sd_sector_classify(s, 512) == CHAON_SD_SECTOR_UNKNOWN);
    assert(s[512] == 0xCC && s[513] == 0xDD);
}

static void test_mbr(void)
{
    uint8_t s[512] = {0};
    s[510] = 0x55;
    s[511] = 0xAA;
    s[446] = 0x7F; /* invalid boot indicator */
    s[450] = 0x0C; /* FAT32 partition type */
    s[458] = 8; /* non-zero length */
    assert(chaon_sd_sector_classify(s, 512) == CHAON_SD_SECTOR_UNKNOWN);
    s[446] = 0x00;
    assert(chaon_sd_sector_classify(s, 512) == CHAON_SD_SECTOR_MBR);
    s[511] = 0;
    assert(chaon_sd_sector_classify(s, 512) == CHAON_SD_SECTOR_UNKNOWN);
}

static void test_fat_vbr(void)
{
    uint8_t s[512] = {0};
    s[0] = 0xEB;
    s[510] = 0x55;
    s[511] = 0xAA;
    memcpy(s + 82, "FAT32   ", 8);
    assert(chaon_sd_sector_classify(s, 512) == CHAON_SD_SECTOR_FAT_BOOT);
    memset(s + 82, 0, 8);
    memcpy(s + 54, "FAT16   ", 8);
    assert(chaon_sd_sector_classify(s, 512) == CHAON_SD_SECTOR_FAT_BOOT);
    s[0] = 0;
    assert(chaon_sd_sector_classify(s, 512) == CHAON_SD_SECTOR_UNKNOWN);
}

int main(void)
{
    test_invalid();
    test_mbr();
    test_fat_vbr();
    puts("ChaoN SD sector-zero hint tests: PASS");
    return 0;
}
