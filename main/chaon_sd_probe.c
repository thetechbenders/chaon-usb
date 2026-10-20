/* SPDX-License-Identifier: AGPL-3.0-only */
/* Phase 2A initial experiment: read-only SDMMC card/sector-0 observation.
 * Original T-Dongle-S3. GPIO signals come from the vendor SD_MMC example;
 * card-detect and actual board pullups have not been bench-validated.
 * The task never changes the USB/BOOT controller or executes card data.
 */
#include "chaon_sd_probe.h"
#include "chaon_sd_sector.h"

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/sdmmc_host.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdmmc_cmd.h"

enum {
    CHAON_SD_POLL_MS = 3000,
    CHAON_SD_RETRY_MS = 5000,
    CHAON_SD_TASK_STACK = 4096
};
static const char *TAG = "chaon_sd";

/* Card and DMA sector buffer are static to avoid large task-stack objects
 * on the 512 KiB SRAM/no-PSRAM original board.
 */
static sdmmc_card_t card;
static uint8_t sector_zero[512] __attribute__((aligned(64)));

static void probe_task(void *ctx)
{
    (void)ctx;
    bool reported_unavailable = false;
    for (;;) {
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
            err = sdmmc_host_init_slot(host.slot, &slot);
            if (err == ESP_OK) {
                err = sdmmc_card_init(&host, &card);
            }
            if (err == ESP_OK) {
                /* Only use the verified sector buffer size; unsupported
                 * sector sizes must not overrun the DMA allocation. */
                if (card.csd.sector_size != sizeof sector_zero) {
                    ESP_LOGW(TAG, "UNSUPPORTED_SECTOR_SIZE %lu",
                             (unsigned long)card.csd.sector_size);
                    err = ESP_ERR_NOT_SUPPORTED;
                } else {
                    err = sdmmc_read_sectors(&card, sector_zero, 0, 1);
                }
            }
            if (err == ESP_OK) {
                reported_unavailable = false;
                const chaon_sd_sector_kind_t kind =
                    chaon_sd_sector_classify(sector_zero,
                                             sizeof sector_zero);
                ESP_LOGI(TAG, "SD_PRESENT sector0_hint=%s (NOT mounted)",
                         kind == CHAON_SD_SECTOR_FAT_BOOT ? "FAT_VBR" :
                         kind == CHAON_SD_SECTOR_MBR ? "MBR" : "UNKNOWN");
                /* No card-detect GPIO is known: poll card status and
                 * reinitialize only after SDMMC reports an error. */
                do {
                    vTaskDelay(pdMS_TO_TICKS(CHAON_SD_POLL_MS));
                    err = sdmmc_get_status(&card);
                } while (err == ESP_OK);
                ESP_LOGW(TAG, "SD_LOST_OR_IO_ERROR %s",
                         esp_err_to_name(err));
            } else if (!reported_unavailable) {
                ESP_LOGW(TAG, "SD_UNAVAILABLE_OR_UNREADABLE %s",
                         esp_err_to_name(err));
                reported_unavailable = true;
            }
            const esp_err_t cleanup_err = sdmmc_host_deinit();
            if (cleanup_err != ESP_OK) {
                ESP_LOGW(TAG, "SD_HOST_DEINIT_FAIL %s",
                         esp_err_to_name(cleanup_err));
            }
        } else if (!reported_unavailable) {
            ESP_LOGW(TAG, "SD_HOST_INIT_FAIL %s", esp_err_to_name(err));
            reported_unavailable = true;
        }
        vTaskDelay(pdMS_TO_TICKS(CHAON_SD_RETRY_MS));
    }
}

void chaon_sd_probe_start(void)
{
    if (xTaskCreate(probe_task, "chaon_sd_probe", CHAON_SD_TASK_STACK,
                    NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
        ESP_LOGW(TAG, "SD_PROBE_TASK_UNAVAILABLE; USB HID unaffected");
    }
}
