/* SPDX-License-Identifier: AGPL-3.0-only */
/* Focused host regression for the real SD-task inspection path. */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

unsigned test_lock_entries;

/* Including the production translation unit keeps static scratch ownership
 * intact while the lightweight headers replace only ESP-IDF host services. */
#include "chaon_sd_probe.c"

enum {
    IMAGE_SECTORS = 1024,
    CARD_SECTORS = 90000,
    FAT_LBA = 32,
    DATA_LBA = 832,
    ROOT_LBA = DATA_LBA,
    FILE_LBA = DATA_LBA + 1
};

static uint8_t image[IMAGE_SECTORS][512];
static uint32_t fail_lba;
static uint32_t hook_lba;
static void (*read_hook)(void);

esp_err_t sdmmc_read_sectors(sdmmc_card_t *unused_card, void *destination,
    size_t start, size_t count)
{
    (void)unused_card;
    assert(count == 1);
    assert(diagnostic_lock.depth == 0);
    if (start == hook_lba && read_hook) {
        void (*hook)(void) = read_hook;
        read_hook = NULL;
        hook();
    }
    if (start == fail_lba) return ESP_FAIL;
    if (start >= IMAGE_SECTORS) {
        memset(destination, 0, 512);
        return ESP_OK;
    }
    memcpy(destination, image[start], 512);
    return ESP_OK;
}

static void write16(uint8_t *destination, uint32_t value)
{
    destination[0] = (uint8_t)value;
    destination[1] = (uint8_t)(value >> 8);
}

static void write32(uint8_t *destination, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i)
        destination[i] = (uint8_t)(value >> (8 * i));
}

static void reset_state(void)
{
    memset(image, 0, sizeof image);
    memset(&catalogue, 0, sizeof catalogue);
    memset(&mailbox, 0, sizeof mailbox);
    memset(&program_stage, 0, sizeof program_stage);
    memset(sector_zero, 0, sizeof sector_zero);
    memset(inspection_source, 0, sizeof inspection_source);
    memset(&inspection_workspace, 0, sizeof inspection_workspace);
    memset(&candidate_program, 0, sizeof candidate_program);
    diagnostic = CHAON_SD_INIT;
    catalogue_valid = false;
    diagnostic_lock.depth = 0;
    fail_lba = UINT32_MAX;
    hook_lba = UINT32_MAX;
    read_hook = NULL;
    card.csd.capacity = CARD_SECTORS;
    card.csd.sector_size = 512;
}

static void build_single_file(const char raw_name[11], const uint8_t *content,
    uint32_t size)
{
    uint8_t *boot = image[0];
    boot[0] = 0xEB;
    boot[510] = 0x55;
    boot[511] = 0xAA;
    write16(boot + 11, 512);
    boot[13] = 1;
    write16(boot + 14, FAT_LBA);
    boot[16] = 1;
    write32(boot + 32, CARD_SECTORS);
    write32(boot + 36, 800);
    write32(boot + 44, 2);

    write32(image[FAT_LBA] + 2 * 4, 0x0FFFFFFF);
    write32(image[FAT_LBA] + 3 * 4, 0x0FFFFFFF);

    uint8_t *entry = image[ROOT_LBA];
    memcpy(entry, raw_name, 11);
    entry[11] = 0x20;
    write16(entry + 26, 3);
    write32(entry + 28, size);
    if (content && size) {
        const size_t copy = size < 512 ? size : 512;
        memcpy(image[FILE_LBA], content, copy);
    }
}

static chaon_fat_catalog_t list_catalogue(void)
{
    chaon_fat_catalog_t found = {0};
    assert(chaon_fat_list_root(read_sector_only, &card, CARD_SECTORS,
        sector_zero, &found) == CHAON_FAT_OK);
    assert(found.count == 1);
    return found;
}

static bool all_zero(const void *data, size_t size)
{
    const volatile uint8_t *byte = data;
    for (size_t i = 0; i < size; ++i)
        if (byte[i] != 0) return false;
    return true;
}

static chaon_inspection_t inspect_direct(const chaon_fat_entry_t *entry)
{
    memset(sector_zero, 0xA5, sizeof sector_zero);
    const chaon_inspection_t result = inspect_one(0, entry,
                                                  &candidate_program);
    assert(all_zero(sector_zero, sizeof sector_zero));
    return result;
}

static void test_success_and_parser_rejection(void)
{
    static const uint8_t valid[] = "TYPE \"SAFE\"\nDELAY 10\n";
    reset_state();
    build_single_file("VALID   CHN", valid, sizeof valid - 1);
    chaon_fat_catalog_t found = list_catalogue();
    chaon_inspection_t result = inspect_direct(&found.entries[0]);
    assert(result.state == CHAON_INSPECT_VALID);
    assert(candidate_program.count == 2);

    static const uint8_t invalid[] = "TYPE \"SAFE\"\nBROKEN\n";
    reset_state();
    build_single_file("BAD     CHN", invalid, sizeof invalid - 1);
    found = list_catalogue();
    result = inspect_direct(&found.entries[0]);
    assert(result.state == CHAON_INSPECT_BAD_SYNTAX);
    assert(all_zero(&candidate_program, sizeof candidate_program));
}

static void test_reader_rejections(void)
{
    static const uint8_t valid[] = "ENTER\n";
    reset_state();
    build_single_file("READERR CHN", valid, sizeof valid - 1);
    chaon_fat_catalog_t found = list_catalogue();
    fail_lba = FILE_LBA;
    assert(inspect_direct(&found.entries[0]).state == CHAON_INSPECT_IO_ERROR);

    reset_state();
    build_single_file("STALE   CHN", valid, sizeof valid - 1);
    found = list_catalogue();
    chaon_fat_entry_t stale = found.entries[0];
    ++stale.file_size;
    assert(inspect_direct(&stale).state == CHAON_INSPECT_STALE);

    reset_state();
    build_single_file("TEXT    TXT", valid, sizeof valid - 1);
    found = list_catalogue();
    assert(inspect_direct(&found.entries[0]).state ==
           CHAON_INSPECT_NOT_SCRIPT);

    reset_state();
    build_single_file("EMPTY   CHN", NULL, 0);
    found = list_catalogue();
    assert(inspect_direct(&found.entries[0]).state ==
           CHAON_INSPECT_TOO_LARGE);

    reset_state();
    build_single_file("OVER    CHN", NULL, CHAON_MAX_SOURCE);
    found = list_catalogue();
    assert(inspect_direct(&found.entries[0]).state ==
           CHAON_INSPECT_TOO_LARGE);
}

static void cancel_selection(void)
{
    chaon_sd_probe_inspect_clear();
}

static void test_mid_read_cancellation(void)
{
    static const uint8_t valid[] = "TYPE \"CANCEL\"\n";
    reset_state();
    build_single_file("CANCEL  CHN", valid, sizeof valid - 1);
    const chaon_fat_catalog_t found = list_catalogue();
    publish_catalog(&found);
    assert(chaon_sd_probe_inspect_request(0, &found.entries[0]));
    hook_lba = FILE_LBA;
    read_hook = cancel_selection;
    assert(!process_inspection());
    assert(read_hook == NULL);
    assert(all_zero(sector_zero, sizeof sector_zero));
    assert(mailbox.report.state == CHAON_INSPECT_NONE);
    assert(program_stage.state == CHAON_PROGRAM_STAGE_EMPTY);
}

int main(void)
{
    test_success_and_parser_rejection();
    test_reader_rejections();
    test_mid_read_cancellation();
    printf("ChaoN SD inspection scratch clearing tests: PASS "
           "(locks=%u)\n", test_lock_entries);
    return 0;
}
