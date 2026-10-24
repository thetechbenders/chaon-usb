/* SPDX-License-Identifier: AGPL-3.0-only */
/* Phase 2A initial experiment: read-only SDMMC card/sector-0 observation.
 * Original T-Dongle-S3. GPIO signals come from the vendor SD_MMC example;
 * card-detect and actual board pullups have not been bench-validated.
 * The task never changes the USB/BOOT controller or executes card data.
 */
#include "chaon_sd_probe.h"
#include "chaon_sd_sector.h"
#include "chaon_fat_ro.h"
#include "chaon_inspect_mailbox.h"
#include "chaon_program_stage.h"
#include <string.h>

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/sdmmc_host.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "sdmmc_cmd.h"

enum {
    CHAON_SD_POLL_MS = 3000,
    CHAON_SD_RETRY_MS = 5000,
    CHAON_SD_TASK_STACK = 4096
};
static const char *TAG = "chaon_sd";

/* Only a scalar copy occurs under the SMP lock; never SD, LCD or log I/O.
 * Main UI is the sole display owner. No volatile-only cross-core sharing. */
/* Task-context mutex: FAT I/O remains outside the lock, and ~3 KB
 * program publish/copy/clear no longer disables interrupts. */
static StaticSemaphore_t diagnostic_mutex_storage;
static SemaphoreHandle_t diagnostic_lock;
static bool lock_ready(void) { return diagnostic_lock != NULL; }
#define STATE_LOCK() do { (void)xSemaphoreTake(diagnostic_lock, portMAX_DELAY); } while (0)
#define STATE_UNLOCK() do { (void)xSemaphoreGive(diagnostic_lock); } while (0)
static chaon_sd_diag_t diagnostic = CHAON_SD_INIT;
static chaon_fat_catalog_t catalogue;
static bool catalogue_valid;
static chaon_inspect_mailbox_t mailbox;
/* One fixed-capacity program slot. The SD service owns it for the lifetime
 * of the firmware and all access is serialized by diagnostic_lock. */
static chaon_program_stage_t program_stage;

static void publish(chaon_sd_diag_t next)
{
    STATE_LOCK();
    diagnostic = next;
    catalogue_valid = false;
    chaon_inspect_mailbox_clear(&mailbox);
    chaon_program_stage_clear(&program_stage);
    STATE_UNLOCK();
}

chaon_sd_diag_t chaon_sd_probe_snapshot(void)
{
    STATE_LOCK();
    const chaon_sd_diag_t copy = diagnostic;
    STATE_UNLOCK();
    return copy;
}

static void publish_catalog(const chaon_fat_catalog_t *found)
{
    STATE_LOCK();
    catalogue = *found;
    catalogue_valid = true;
    diagnostic = CHAON_SD_CATALOG_OK;
    chaon_inspect_mailbox_clear(&mailbox);
    chaon_program_stage_clear(&program_stage);
    STATE_UNLOCK();
}

bool chaon_sd_probe_catalog_snapshot(chaon_fat_catalog_t *result)
{
    if (!result) return false;
    STATE_LOCK();
    const bool valid = catalogue_valid &&
                       diagnostic == CHAON_SD_CATALOG_OK;
    if (valid) *result = catalogue;
    STATE_UNLOCK();
    return valid;
}

/* All mailbox operations are short, lock-protected metadata copies.
 * Neither filesystem I/O nor parsing is performed in the UI/main task. */
bool chaon_sd_probe_inspect_request(uint8_t index,
                                    const chaon_fat_entry_t *expected)
{
    if (!expected) return false;
    STATE_LOCK();
    bool ok = catalogue_valid && diagnostic == CHAON_SD_CATALOG_OK;
    if (ok) {
        const bool inspect_ok = chaon_inspect_mailbox_request(&mailbox,
            &catalogue, index, expected);
        const bool stage_ok = chaon_program_stage_request(&program_stage,
            &catalogue, index, expected);
        ok = inspect_ok && stage_ok;
        if (!ok) {
            chaon_inspect_mailbox_clear(&mailbox);
            chaon_program_stage_clear(&program_stage);
        }
    }
    STATE_UNLOCK();
    return ok;
}

bool chaon_sd_probe_inspect_snapshot(uint8_t index,
    const chaon_fat_entry_t *expected, chaon_inspection_t *out)
{
    if (!expected || !out) return false;
    *out = (chaon_inspection_t){ .state = CHAON_INSPECT_NONE };
    STATE_LOCK();
    /* Validate catalogue identity BEFORE touching the caller's summary.
     * Otherwise a stale mailbox value could be copied on a false result. */
    const bool ok = catalogue_valid &&
        diagnostic == CHAON_SD_CATALOG_OK &&
        index < catalogue.count &&
        catalogue.count <= CHAON_FAT_MAX_FILES &&
        memcmp(catalogue.entries[index].raw_name, expected->raw_name, 11) == 0 &&
        catalogue.entries[index].first_cluster == expected->first_cluster &&
        catalogue.entries[index].file_size == expected->file_size &&
        catalogue.entries[index].is_directory == expected->is_directory &&
        chaon_inspect_mailbox_snapshot(&mailbox, index, expected, out);
    STATE_UNLOCK();
    return ok;
}

void chaon_sd_probe_inspect_clear(void)
{
    STATE_LOCK();
    if (mailbox.queued || mailbox.report.state != CHAON_INSPECT_NONE ||
        program_stage.state != CHAON_PROGRAM_STAGE_EMPTY) {
        chaon_inspect_mailbox_clear(&mailbox);
        chaon_program_stage_clear(&program_stage);
    }
    STATE_UNLOCK();
}

bool chaon_sd_probe_staged_program_copy(uint8_t index,
    const chaon_fat_entry_t *expected, uint32_t *generation,
    chaon_program_t *out)
{
    if (generation) *generation = 0;
    if (!generation || !out) return false;
    memset(out, 0, sizeof *out);
    if (!expected) return false;
    STATE_LOCK();
    const bool ok = catalogue_valid &&
        diagnostic == CHAON_SD_CATALOG_OK && index < catalogue.count &&
        catalogue.count <= CHAON_FAT_MAX_FILES &&
        memcmp(catalogue.entries[index].raw_name, expected->raw_name, 11) == 0 &&
        catalogue.entries[index].first_cluster == expected->first_cluster &&
        catalogue.entries[index].file_size == expected->file_size &&
        catalogue.entries[index].is_directory == expected->is_directory &&
        chaon_program_stage_copy(&program_stage, index, expected,
                                 generation, out);
    STATE_UNLOCK();
    return ok;
}

bool chaon_sd_probe_staged_program_copy_hashed(uint8_t index,
    const chaon_fat_entry_t *expected, uint32_t *generation,
    chaon_program_t *out, uint8_t digest[32])
{
    if (generation) *generation=0;
    if (out) memset(out,0,sizeof *out);
    if (digest) memset(digest,0,CHAON_SHA256_SIZE);
    if (!expected || !generation || !out || !digest) return false;
    STATE_LOCK();
    const bool ok = catalogue_valid &&
        diagnostic == CHAON_SD_CATALOG_OK &&
        index < catalogue.count &&
        catalogue.count <= CHAON_FAT_MAX_FILES &&
        memcmp(catalogue.entries[index].raw_name,expected->raw_name,11)==0 &&
        catalogue.entries[index].first_cluster==expected->first_cluster &&
        catalogue.entries[index].file_size==expected->file_size &&
        catalogue.entries[index].is_directory==expected->is_directory &&
        chaon_program_stage_copy_hashed(&program_stage,index,expected,
                                        generation,out,digest);
    STATE_UNLOCK();
    return ok;
}

static bool read_sector_only(void *ctx, uint32_t lba, uint8_t dest[512])
{
    return sdmmc_read_sectors((sdmmc_card_t *)ctx, dest, lba, 1) == ESP_OK;
}

/* Card and DMA sector buffer are static to avoid large task-stack objects
 * on the 512 KiB SRAM/no-PSRAM original board.
 */
static sdmmc_card_t card;
static uint8_t sector_zero[512] __attribute__((aligned(64)));

/* This shared DMA scratch may contain the final sector of a script. Volatile
 * stores make the post-inspection wipe explicit and prevent dead-store
 * elimination when a later SD operation overwrites the buffer. */
static void clear_sector_scratch(void)
{
    volatile uint8_t *byte = sector_zero;
    for (size_t i = 0; i < sizeof sector_zero; ++i) byte[i] = 0;
}

/* All three objects belong exclusively to the SD task and stay outside its
 * 4 KB stack. Source/workspace/candidate are wiped on every completion. */
static uint8_t inspection_source[CHAON_MAX_SOURCE];
static chaon_inspect_workspace_t inspection_workspace;
static chaon_program_t candidate_program;
static uint8_t candidate_digest[CHAON_SHA256_SIZE];

static chaon_inspection_t inspect_one(uint8_t index,
    const chaon_fat_entry_t *identity, chaon_program_t *candidate)
{
    size_t size = 0;
    chaon_inspection_t info = { .state = CHAON_INSPECT_BAD_MEDIA };
    memset(candidate, 0, sizeof *candidate);
    memset(candidate_digest, 0, sizeof candidate_digest);
    memset(&inspection_workspace, 0, sizeof inspection_workspace);
    const chaon_fat_file_result_t status =
        chaon_fat_read_script(read_sector_only, &card,
            (uint32_t)card.csd.capacity, sector_zero, index, identity,
            inspection_source, sizeof inspection_source, &size);
    switch (status) {
        case CHAON_FAT_FILE_OK:
            info = chaon_inspect_compile_bytes(inspection_source, size,
                &inspection_workspace, candidate);
            if (info.state == CHAON_INSPECT_VALID)
                chaon_source_sha256(inspection_source, size, candidate_digest);
            break;
        case CHAON_FAT_FILE_UNSUPPORTED:
            info.state = CHAON_INSPECT_NOT_SCRIPT; break;
        case CHAON_FAT_FILE_TOO_LARGE:
            info.state = CHAON_INSPECT_TOO_LARGE; break;
        case CHAON_FAT_FILE_STALE:
            info.state = CHAON_INSPECT_STALE; break;
        case CHAON_FAT_FILE_IO_ERROR:
            info.state = CHAON_INSPECT_IO_ERROR; break;
        default:
            info.state = CHAON_INSPECT_BAD_MEDIA; break;
    }
    clear_sector_scratch();
    memset(inspection_source, 0, sizeof inspection_source);
    memset(&inspection_workspace, 0, sizeof inspection_workspace);
    if (info.state != CHAON_INSPECT_VALID) {
        memset(candidate, 0, sizeof *candidate);
        memset(candidate_digest, 0, sizeof candidate_digest);
    }
    return info;
}

static bool process_inspection(void)
{
    uint32_t epoch = 0;
    uint32_t stage_epoch = 0;
    uint8_t index = 0;
    uint8_t stage_index = 0;
    chaon_fat_entry_t entry = {0};
    chaon_fat_entry_t stage_entry = {0};
    STATE_LOCK();
    const bool inspect_pending = catalogue_valid &&
        chaon_inspect_mailbox_take(&mailbox, &epoch, &index, &entry);
    const bool stage_pending = catalogue_valid &&
        chaon_program_stage_take(&program_stage, &stage_epoch, &stage_index,
                                 &stage_entry);
    bool pending = inspect_pending && stage_pending && index == stage_index &&
        entry.is_directory == stage_entry.is_directory &&
        entry.first_cluster == stage_entry.first_cluster &&
        entry.file_size == stage_entry.file_size &&
        memcmp(entry.raw_name, stage_entry.raw_name, 11) == 0;
    if ((inspect_pending || stage_pending) && !pending) {
        chaon_inspect_mailbox_clear(&mailbox);
        chaon_program_stage_clear(&program_stage);
    }
    STATE_UNLOCK();
    if (!pending) return false;

    chaon_inspection_t summary = inspect_one(index, &entry,
                                             &candidate_program);
    STATE_LOCK();
    if (catalogue_valid && diagnostic == CHAON_SD_CATALOG_OK) {
        if (summary.state == CHAON_INSPECT_VALID &&
            !chaon_program_stage_publish_hashed(&program_stage, stage_epoch,
                stage_index, &stage_entry, &candidate_program, candidate_digest)) {
            summary = (chaon_inspection_t){
                .state = CHAON_INSPECT_BAD_MEDIA
            };
        } else if (summary.state != CHAON_INSPECT_VALID) {
            (void)chaon_program_stage_publish(&program_stage, stage_epoch,
                stage_index, &stage_entry, NULL);
        }
        chaon_inspect_mailbox_publish(&mailbox, epoch, index, summary);
    }
    STATE_UNLOCK();
    memset(&candidate_program, 0, sizeof candidate_program);
    memset(candidate_digest, 0, sizeof candidate_digest);
    ESP_LOGI(TAG, "INSPECT state=%u count=%u line=%u (READ ONLY)",
             (unsigned)summary.state, (unsigned)summary.step_count,
             (unsigned)summary.error_line);
    return summary.state == CHAON_INSPECT_IO_ERROR;
}

static void probe_task(void *ctx)
{
    (void)ctx;
    bool reported_unavailable = false;
    for (;;) {
        publish(CHAON_SD_INIT);
        chaon_sd_diag_t failure = CHAON_SD_HOST_ERROR;
        sdmmc_host_t host = SDMMC_HOST_DEFAULT();
        host.max_freq_khz = 10000; /* conservative 10 MHz, 1-bit bus */
        sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
        slot.width = 1;
        slot.clk = GPIO_NUM_12;
        slot.cmd = GPIO_NUM_16;
        slot.d0 = GPIO_NUM_14;
        slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

        esp_err_t err = sdmmc_host_init();
        if (err == ESP_OK) {
            failure = CHAON_SD_SLOT_ERROR;
            err = sdmmc_host_init_slot(host.slot, &slot);
            if (err == ESP_OK) {
                failure = CHAON_SD_CARD_ERROR;
                err = sdmmc_card_init(&host, &card);
            }
            if (err == ESP_OK) {
                /* Only use the verified sector buffer size; unsupported
                 * sector sizes must not overrun the DMA allocation. */
                if (card.csd.sector_size != sizeof sector_zero) {
                    failure = CHAON_SD_SIZE_ERROR;
                    ESP_LOGW(TAG, "UNSUPPORTED_SECTOR_SIZE %lu",
                             (unsigned long)card.csd.sector_size);
                    err = ESP_ERR_NOT_SUPPORTED;
                } else {
                    failure = CHAON_SD_READ_ERROR;
                    err = sdmmc_read_sectors(&card, sector_zero, 0, 1);
                }
            }
            if (err == ESP_OK) {
                reported_unavailable = false;
                publish(CHAON_SD_READ_OK);
                const chaon_sd_sector_kind_t kind =
                    chaon_sd_sector_classify(sector_zero,
                                             sizeof sector_zero);
                ESP_LOGI(TAG, "SD_PRESENT sector0_hint=%s (READ-ONLY)",
                         kind == CHAON_SD_SECTOR_FAT_BOOT ? "FAT_VBR" :
                         kind == CHAON_SD_SECTOR_MBR ? "MBR" : "UNKNOWN");

                /* A second read-only scan lists ONLY bounded root metadata.
                 * No FAT/VFS mount, no SD writes, no card-sourced HID path.
                 * Never show names without BPB and LBA validation. */
                chaon_fat_catalog_t found = {0};
                chaon_fat_result_t result = CHAON_FAT_INVALID;
                if (card.csd.capacity > 0 &&
                    card.csd.capacity <= UINT32_MAX) {
                    result = chaon_fat_list_root(read_sector_only, &card,
                        (uint32_t)card.csd.capacity, sector_zero, &found);
                }
                if (result == CHAON_FAT_OK ||
                    result == CHAON_FAT_LIMIT) {
                    publish_catalog(&found);
                    ESP_LOGI(TAG, "FAT_ROOT type=%u count=%u more=%u%s",
                        (unsigned)found.type, (unsigned)found.count,
                        (unsigned)found.more,
                        result == CHAON_FAT_LIMIT ? " scan-limit" : "");
                } else {
                    publish(result == CHAON_FAT_UNSUPPORTED ?
                        CHAON_SD_FS_UNSUPPORTED : CHAON_SD_FS_ERROR);
                    ESP_LOGW(TAG, "FAT_ROOT_UNAVAILABLE error=%d", (int)result);
                }
                /* No card-detect GPIO is known: poll card status and
                 * reinitialize only after SDMMC reports an error. */
                unsigned time_to_status_ms = CHAON_SD_POLL_MS;
                do {
                    vTaskDelay(pdMS_TO_TICKS(100));
                    if (process_inspection()) {
                        err = ESP_FAIL; /* fail closed and reinitialize SD */
                        break;
                    }
                    if (time_to_status_ms <= 100) {
                        time_to_status_ms = CHAON_SD_POLL_MS;
                        err = sdmmc_get_status(&card);
                    } else {
                        time_to_status_ms -= 100;
                    }
                } while (err == ESP_OK);
                publish(CHAON_SD_LOST_IO);
                ESP_LOGW(TAG, "SD_LOST_OR_IO_ERROR %s",
                         esp_err_to_name(err));
            } else {
                publish(failure);
                if (!reported_unavailable) {
                    ESP_LOGW(TAG, "SD_UNAVAILABLE_OR_UNREADABLE %s",
                             esp_err_to_name(err));
                    reported_unavailable = true;
                }
            }
            const esp_err_t cleanup_err = sdmmc_host_deinit();
            if (cleanup_err != ESP_OK) {
                publish(CHAON_SD_CLEANUP_ERROR);
                ESP_LOGW(TAG, "SD_HOST_DEINIT_FAIL %s",
                         esp_err_to_name(cleanup_err));
            }
        } else {
            publish(CHAON_SD_HOST_ERROR);
            if (!reported_unavailable) {
                ESP_LOGW(TAG, "SD_HOST_INIT_FAIL %s", esp_err_to_name(err));
                reported_unavailable = true;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(CHAON_SD_RETRY_MS));
    }
}

void chaon_sd_probe_start(void)
{
    diagnostic_lock=xSemaphoreCreateMutexStatic(&diagnostic_mutex_storage);
    if (!lock_ready()) {
        ESP_LOGW(TAG, "SD_MUTEX_UNAVAILABLE; read-only SD task disabled");
        return;
    }
    if (xTaskCreate(probe_task, "chaon_sd_probe", CHAON_SD_TASK_STACK,
                    NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
        publish(CHAON_SD_TASK_ERROR);
        ESP_LOGW(TAG, "SD_PROBE_TASK_UNAVAILABLE; USB HID unaffected");
    }
}
