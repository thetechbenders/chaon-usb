"""Exercise the source-gated eject patch with a narrowly pinned synthetic fixture.

The real esp_tinyusb source SHA is checked by the preceding activity hook.
This test checks transformations and anchor drift without requiring ESP-IDF.
"""
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("eject_patch", ROOT / "tools/patch_msc_eject.py")
patch = importlib.util.module_from_spec(spec)
spec.loader.exec_module(patch)


def fixture():
    return r'''/* upstream source prefix */
#define MSC_STORAGE_BUFFER_SIZE CONFIG_TINYUSB_MSC_BUFSIZE
static inline esp_err_t msc_storage_get_by_lun(uint8_t lun, msc_storage_obj_t **storage)
{
    ESP_RETURN_ON_FALSE(p_msc_driver->dynamic.storage[lun]->medium != NULL, ESP_ERR_NOT_FOUND, TAG, "missing");
}
static inline esp_err_t msc_storage_write_sector(uint8_t lun, uint32_t lba, uint32_t offset, size_t n, const void *ptr)
{
    return ESP_OK;
}
/**
 * @brief Handles deferred USB MSC write operations.
 */
static void tusb_write_func(void *p) {
    ESP_LOGE(TAG, "Write failed, error=0x%x", err);
    /* ChaoN modification: sticky write-failure evidence. */
    extern void chaon_msc_write_failure(void);
    chaon_msc_write_failure();
}
static inline esp_err_t msc_storage_write_sector_deferred(uint8_t lun, uint32_t lba, uint32_t offset, size_t n, const void *p) {
    usbd_defer_func(tusb_write_func, (void *)storage, false);
    return ESP_OK;
}

static esp_err_t vfs_fat_format(BYTE format_flags) { return ESP_OK; }
#define SCSI_CODE_ASCQ                                  0x00
bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
    return true;
}
// Invoked when received SCSI_CMD_READ_CAPACITY_10
void tud_msc_capacity_cb(uint8_t lun)
{
    if (msc_storage_get_by_lun(lun, &storage) == ESP_OK) {
        tinyusb_msc_get_storage_capacity(p, &capacity);
    }
}
bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject)
{
    return true;
}
// Invoked when received SCSI READ10 command
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize)
{
    /* ChaoN modification: nonblocking data-attempt signal only. */
    return bufsize;
}
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize)
{
    if (bufsize > MSC_STORAGE_BUFFER_SIZE) {
        ESP_LOGE(TAG, "Buffer size %"PRIu32" exceeds maximum allowed size %d", bufsize, MSC_STORAGE_BUFFER_SIZE);
        goto error;
    }
    esp_err_t err = msc_storage_write_sector_deferred(lun, lba, offset, bufsize, buffer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WRITE(10) command failed, %s", esp_err_to_name(err));
        goto error;
    }
    return bufsize;
error:
    return -1;
}
int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void *buffer, uint16_t bufsize)
{
    int32_t ret;
    return 0;
}
'''


class PatchTests(unittest.TestCase):
    def test_sync_write_and_eject_guards(self):
        result = patch.patch_source(fixture())
        self.assertNotIn("usbd_defer_func(", result)
        self.assertNotIn("msc_storage_write_sector_deferred(", result)
        self.assertIn("msc_storage_write_sector(lun, lba, offset, bufsize, buffer)", result)
        self.assertIn("chaon_msc_device_start_stop(lun, start, load_eject)", result)
        self.assertIn("if (chaon_msc_host_ready(lun)) return true;", result)
        self.assertEqual(result.count("if (!chaon_msc_host_ready(lun))"), 2)
        self.assertIn('p_msc_driver->dynamic.storage[lun] != NULL', result)
        self.assertIn("chaon_msc_write_failure();", result)
        self.assertEqual(result.count("chaon_msc_write_failure();"), 2)
        self.assertIn("SCSI_SENSE_NOT_READY", result)
        self.assertIn("|| chaon_msc_write_failed()", result)
        self.assertIn("return bytes <= MSC_STORAGE_BUFFER_SIZE &&", result)
        self.assertIn("if (lun != 0)", result)
        self.assertEqual(result.count("if (!chaon_msc_request_valid(lun, lba, offset, bufsize))"), 2)
        self.assertIn("_Static_assert(CONFIG_TINYUSB_MSC_BUFSIZE >= 512", result)
        self.assertIn("storage->sector_count, storage->sector_size", result)

    def test_missing_or_duplicate_anchor_is_rejected(self):
        original = fixture()
        for source in (original.replace("static void tusb_write_func(", "static void other("),
                       original + original,
                       original.replace("static esp_err_t vfs_fat_format", "static esp_err_t other")):
            with self.assertRaises(ValueError):
                patch.patch_source(source)


if __name__ == "__main__":
    unittest.main()
