/* SPDX-License-Identifier: AGPL-3.0-only */
/* Cold-boot, single-owner media arbitration. No USB driver is running yet. */
#include "chaon_usb_storage.h"
#include "chaon_usb_marker.h"
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
#include "ff.h"

static const char *TAG = "chaon_msc";
static sdmmc_card_t storage_card;
static chaon_usb_volume_t volume_snapshot;
static const char *boot_fault_reason = "BOOT UNKNOWN";

const char *chaon_usb_boot_fault_reason(void)
{
    return boot_fault_reason;
}

const chaon_usb_volume_t *chaon_usb_volume_snapshot(void)
{
    return &volume_snapshot;
}

/* Read FAT usage only before unmount and before USB ownership begins. */
static void volume_read_snapshot(void)
{
    uint64_t total = 0, free_bytes = 0;
    memset(&volume_snapshot, 0, sizeof volume_snapshot);
    if (esp_vfs_fat_info("/chaon", &total, &free_bytes) == ESP_OK &&
        total > 0 && free_bytes <= total) {
        volume_snapshot.valid = true;
        volume_snapshot.total_bytes = total;
        volume_snapshot.used_bytes = total - free_bytes;
    }
    char label[12] = {0};
    if (f_getlabel("", label, NULL) == FR_OK && label[0]) {
        size_t i = 0;
        while (i < sizeof volume_snapshot.volume_label - 1 && label[i]) {
            const unsigned char ch = (unsigned char)label[i];
            volume_snapshot.volume_label[i] = (char)(
                ch >= 'a' && ch <= 'z' ? ch - 'a' + 'A' :
                (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') ||
                ch == '-' || ch == '_' || ch == ' ' ? ch : ' ');
            ++i;
        }
        volume_snapshot.volume_label[i] = '\0';
    }
    if (!volume_snapshot.volume_label[0])
        memcpy(volume_snapshot.volume_label, "USB DRIVE", sizeof "USB DRIVE");
}


/* Diagnostics only. Application-side mounts and automatic formatting
 * of host-owned media are prohibited. */
static void msc_event_log(tinyusb_msc_storage_handle_t handle,
                          tinyusb_msc_event_t *event, void *ctx)
{
    (void)handle;
    (void)ctx;
    if (!event) return;
    ESP_LOGW(TAG, "MSC_EVENT id=%d owner=%d",
             (int)event->id, (int)event->mount_point);
}


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

static int marker_stat(void *ctx, const char *path, struct stat *info)
{
    (void)ctx;
    return stat(path, info);
}
static int marker_unlink(void *ctx, const char *path)
{
    (void)ctx;
    return unlink(path);
}
static int marker_unmount(void *ctx)
{
    sdmmc_card_t *card = ctx;
    return esp_vfs_fat_sdcard_unmount("/chaon", card) == ESP_OK ? 0 : -1;
}

chaon_usb_mode_t chaon_usb_boot_mode(void)
{
    memset(&volume_snapshot, 0, sizeof volume_snapshot);
    boot_fault_reason = "BOOT UNKNOWN";
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
        boot_fault_reason = "FAT MOUNT FAIL";
        ESP_LOGW(TAG, "BOOT_MARKER_SCAN_UNAVAILABLE %s; USB blocked",
                 esp_err_to_name(err));
        return CHAON_USB_BLOCKED;
    }
    volume_read_snapshot();
    const chaon_usb_marker_ops_t ops = {
        .stat_file = marker_stat, .unlink_file = marker_unlink,
        .unmount_card = marker_unmount,
    };
    bool unmount_failed = false;
    const chaon_usb_mode_t mode = chaon_usb_marker_finish(
        &ops, card, &unmount_failed);
    if (unmount_failed) {
        boot_fault_reason = "FAT UNMOUNT FAIL";
        ESP_LOGE(TAG, "SD_UNMOUNT_FAILED; USB blocked");
        return CHAON_USB_BLOCKED;
    }
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
    /* esp_tinyusb 2.0.0 defaults can mount on eject and format unknown
     * media. Prohibit both for a host-owned raw block device. */
    const tinyusb_msc_driver_config_t driver = {
        .user_flags = { .auto_mount_off = 1 },
        .callback = msc_event_log,
    };
    err = tinyusb_msc_install_driver(&driver);
    if (err != ESP_OK) goto cleanup;

    const tinyusb_msc_storage_config_t config = {
        .medium.card = &storage_card,
        .fat_fs = { .do_not_format = true },
        .mount_point = TINYUSB_MSC_STORAGE_MOUNT_USB,
    };
    err = tinyusb_msc_new_storage_sdmmc(&config, out);
    if (err == ESP_OK && *out != NULL) return true;
    /* The pinned implementation can leak on partial allocation failure.
     * Do not enumerate a missing LUN: upstream callbacks assume it exists.
     * Retain ownership until reboot, with USB disabled by the caller. */
    ESP_LOGE(TAG, "MSC_BACKEND_INIT_FAILED; USB must remain disabled");
    return false;
cleanup:
    (void)sdmmc_host_deinit();
unavailable:
    ESP_LOGE(TAG, "MSC_MEDIA_UNAVAILABLE %s; no HID fallback",
             esp_err_to_name(err));
    return false;
}
