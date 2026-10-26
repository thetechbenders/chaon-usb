/* SPDX-License-Identifier: AGPL-3.0-only */
/* Cold-boot, single-owner media arbitration. No USB driver is running yet. */
#include "chaon_usb_storage.h"
#include <errno.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "driver/gpio.h"
#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "esp_log.h"
#include "sdmmc_cmd.h"

static const char *TAG = "chaon_msc";
static sdmmc_card_t storage_card;

static sdmmc_host_t sd_host(void)
{
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = 10000; /* matches hardware-tested 1-bit read path */
    return host;
}
static sdmmc_slot_config_t sd_slot(void)
{
    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.width = 1;
    slot.clk = GPIO_NUM_12;
    slot.cmd = GPIO_NUM_16;
    slot.d0 = GPIO_NUM_14;
    slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    return slot;
}

chaon_usb_mode_t chaon_usb_boot_mode(void)
{
    sdmmc_host_t host = sd_host();
    sdmmc_slot_config_t slot = sd_slot();
    const esp_vfs_fat_sdmmc_mount_config_t mount = {
        .format_if_mount_failed = false, .max_files = 2,
        .allocation_unit_size = 512,
    };
    sdmmc_card_t *card = NULL;
    esp_err_t err = esp_vfs_fat_sdmmc_mount("/chaon", &host, &slot,
                                            &mount, &card);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "BOOT_MARKER_SCAN_UNAVAILABLE %s; HID forbidden",
                 esp_err_to_name(err));
        return CHAON_USB_MSC;
    }
    struct stat info;
    const char *path = "/chaon/DUCKY";
    chaon_marker_state_t marker = CHAON_MARKER_IO_ERROR;
    bool removed = false, absent_after = false;
    errno = 0;
    if (stat(path, &info) == 0) {
        marker = (S_ISREG(info.st_mode) && info.st_size == 0)
                 ? CHAON_MARKER_EMPTY_REGULAR : CHAON_MARKER_INVALID;
        if (marker == CHAON_MARKER_EMPTY_REGULAR && unlink(path) == 0) {
            removed = true;
            errno = 0;
            absent_after = (stat(path, &info) < 0 && errno == ENOENT);
        }
    } else if (errno == ENOENT) {
        marker = CHAON_MARKER_ABSENT;
    }
    /* No MSC handler or read-only worker has started; unmount before
     * transferring media ownership to either personality. */
    err = esp_vfs_fat_sdcard_unmount("/chaon", card);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SD_UNMOUNT_FAILED; HID forbidden");
        return CHAON_USB_MSC;
    }
    const chaon_usb_mode_t mode =
        chaon_usb_mode_select(marker, removed, absent_after);
    ESP_LOGI(TAG, "cold boot USB personality: %s",
             mode == CHAON_USB_HID ? "HID (marker consumed)" : "MSC");
    return mode;
}

bool chaon_usb_storage_start(tinyusb_msc_storage_handle_t *out)
{
    if (!out) return false;
    *out = NULL;
    sdmmc_host_t host = sd_host();
    sdmmc_slot_config_t slot = sd_slot();
    esp_err_t err = sdmmc_host_init();
    if (err != ESP_OK) goto unavailable;
    err = sdmmc_host_init_slot(host.slot, &slot);
    if (err != ESP_OK) goto cleanup;
    err = sdmmc_card_init(&host, &storage_card);
    if (err != ESP_OK) goto cleanup;
    if (storage_card.csd.sector_size != 512 || !storage_card.csd.capacity) {
        err = ESP_ERR_NOT_SUPPORTED;
        goto cleanup;
    }
    const tinyusb_msc_storage_config_t config = {
        .medium.card = &storage_card,
    };
    err = tinyusb_msc_new_storage_sdmmc(&config, out);
    if (err == ESP_OK) return true;
cleanup:
    (void)sdmmc_host_deinit();
unavailable:
    ESP_LOGE(TAG, "MSC_MEDIA_UNAVAILABLE %s; no HID fallback",
             esp_err_to_name(err));
    return false;
}
