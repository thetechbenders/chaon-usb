"""Strict second-stage patch for pinned esp_tinyusb 2.0.0 MSC source.

The existing activity patch validates the original upstream SHA-256 first.
This stage removes unsafe acknowledged-before-SD-write deferral and adds
device eject readiness guards. Build-local copy only, never managed sources.

WARNING: synchronous SD writes block TinyUSB's task; benchmark/timeout review
and physical requalification required before flashing or release.
"""
from pathlib import Path
import sys


def once(text, old, new):
    count = text.count(old)
    if count != 1:
        raise ValueError(f"MSC upstream contract changed: {old[:75]!r}, count={count}")
    return text.replace(old, new, 1)


def replace_block(text, start, end, replacement):
    if text.count(start) != 1 or text.count(end) != 1:
        raise ValueError("MSC callback anchors missing or duplicated")
    i, j = text.index(start), text.index(end)
    if i >= j:
        raise ValueError("MSC callback end precedes start")
    block = text[i:j]
    if block.count("\n}") != 1:
        raise ValueError("MSC callback body layout changed")
    return text[:i] + replacement + text[j:]


def patch_source(text):
    # This source already passed tools/patch_msc_activity.py's pinned SHA gate.
    # Remove deferred worker/queue and its one-buffer operation: WRITE10 now
    # waits for the SDMMC driver result before reporting SCSI success.
    start = "/**\n * @brief Handles deferred USB MSC write operations."
    end = "static esp_err_t vfs_fat_format(BYTE format_flags)"
    if text.count(start) != 1 or text.count(end) != 1:
        raise ValueError("MSC deferred-worker function anchors changed")
    begin, finish = text.index(start), text.index(end)
    if not begin < finish:
        raise ValueError("MSC deferred functions reordered")
    removed = text[begin:finish]
    if removed.count("static void tusb_write_func(") != 1 or removed.count(
        "static inline esp_err_t msc_storage_write_sector_deferred(") != 1:
        raise ValueError("unexpected MSC deferred-worker content")
    text = text[:begin] + text[finish:]

    # The ESP-specific LUN getter's NULL guard must precede dereference.
    text = once(text,
        "ESP_RETURN_ON_FALSE(p_msc_driver->dynamic.storage[lun]->medium != NULL,",
        "ESP_RETURN_ON_FALSE(p_msc_driver->dynamic.storage[lun] != NULL, ESP_ERR_NOT_FOUND, TAG, \"Unallocated LUN %d\", lun);\n"
        "    ESP_RETURN_ON_FALSE(p_msc_driver->dynamic.storage[lun]->medium != NULL,")

    # A backend that was not allocated must never reach read/write callbacks.
    sense = "tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, SCSI_CODE_ASC_MEDIUM_NOT_PRESENT, SCSI_CODE_ASCQ);"
    anchor = "#define SCSI_CODE_ASCQ                                  0x00"
    text = once(text, anchor, anchor + """

/* Product-owned media latch; all callbacks and blocking writes run on the
 * TinyUSB task. Never mount host-owned SD media in the application. */
extern bool chaon_msc_device_ready(uint8_t lun, bool usb_owned);
extern bool chaon_msc_device_start_stop(uint8_t lun, bool start, bool load_eject);
extern bool chaon_msc_write_failed(void);

static bool chaon_msc_host_ready(uint8_t lun)
{
    msc_storage_obj_t *storage = NULL;
    if (msc_storage_get_by_lun(lun, &storage) != ESP_OK || chaon_msc_write_failed()) return false;
    return chaon_msc_device_ready(
        lun, storage->mount_point == TINYUSB_MSC_STORAGE_MOUNT_USB);
}
""")
    # Compile-time check: this SDMMC adapter requires whole 512-byte sectors.
    text = once(text, "#define MSC_STORAGE_BUFFER_SIZE CONFIG_TINYUSB_MSC_BUFSIZE",
        "#define MSC_STORAGE_BUFFER_SIZE CONFIG_TINYUSB_MSC_BUFSIZE\n"
        "_Static_assert(CONFIG_TINYUSB_MSC_BUFSIZE >= 512 && "
        "CONFIG_TINYUSB_MSC_BUFSIZE % 512 == 0, \"MSC FIFO must fit complete sectors\");")
    text = once(text, "extern bool chaon_msc_write_failed(void);",
        "extern bool chaon_msc_write_failed(void);\n"
        "extern bool chaon_msc_transfer_valid(uint32_t lba, uint32_t offset, "
        "uint32_t bytes, uint32_t sector_count, uint32_t sector_size);")
    insert_before = "bool tud_msc_test_unit_ready_cb(uint8_t lun)\n{"
    helper = """static bool chaon_msc_request_valid(uint8_t lun, uint32_t lba,
                                    uint32_t offset, uint32_t bytes)
{
    msc_storage_obj_t *storage = NULL;
    return bytes <= MSC_STORAGE_BUFFER_SIZE &&
        chaon_msc_host_ready(lun) &&
        msc_storage_get_by_lun(lun, &storage) == ESP_OK &&
        chaon_msc_transfer_valid(lba, offset, bytes,
                                 storage->sector_count, storage->sector_size);
}

"""
    text = once(text, insert_before, helper + insert_before)
    start = "bool tud_msc_test_unit_ready_cb(uint8_t lun)\n{"
    end = "// Invoked when received SCSI_CMD_READ_CAPACITY_10"
    text = replace_block(text, start, end, """bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
    if (chaon_msc_host_ready(lun)) return true;
    """ + sense + """
    return false;
}

""")
    text = once(text,
        "if (msc_storage_get_by_lun(lun, &storage) == ESP_OK) {\n        tinyusb_msc_get_storage_capacity",
        "if (chaon_msc_host_ready(lun) && msc_storage_get_by_lun(lun, &storage) == ESP_OK) {\n        tinyusb_msc_get_storage_capacity")
    start = "bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject)\n{"
    end = "// Invoked when received SCSI READ10 command"
    text = replace_block(text, start, end, """bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject)
{
    (void)power_condition;
    if (chaon_msc_device_start_stop(lun, start, load_eject)) return true;
    """ + sense + """
    return false;
}

""")
    for direction, buffer in (("read", "void *buffer"), ("write", "uint8_t *buffer")):
        anchor = (f"int32_t tud_msc_{direction}10_cb(uint8_t lun, uint32_t lba, "
                  f"uint32_t offset, {buffer}, uint32_t bufsize)\n{{\n")
        check = ("""    if (!chaon_msc_host_ready(lun)) {
        """ + sense + """
        return -1;
    }
""")
        text = once(text, anchor, anchor + check)
        text = once(text, anchor + check,
            anchor + check + """    if (!chaon_msc_request_valid(lun, lba, offset, bufsize)) {
        tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x21, 0x00);
        return -1;
    }
""")

    text = once(text,
        "msc_storage_write_sector_deferred(lun, lba, offset, bufsize, buffer)",
        "msc_storage_write_sector(lun, lba, offset, bufsize, buffer)")
    text = once(text,
        'ESP_LOGE(TAG, "WRITE(10) command failed, %s", esp_err_to_name(err));\n        goto error;',
        'ESP_LOGE(TAG, "WRITE(10) command failed, %s", esp_err_to_name(err));\n'
        '        extern void chaon_msc_write_failure(void);\n'
        '        chaon_msc_write_failure();\n        goto error;')
    # The existing callback's size rejection also latches a sticky failure.
    text = once(text,
        'ESP_LOGE(TAG, "Buffer size %"PRIu32" exceeds maximum allowed size %d", bufsize, MSC_STORAGE_BUFFER_SIZE);\n        goto error;',
        'ESP_LOGE(TAG, "Buffer size %"PRIu32" exceeds maximum allowed size %d", bufsize, MSC_STORAGE_BUFFER_SIZE);\n'
        '        extern void chaon_msc_write_failure(void);\n'
        '        chaon_msc_write_failure();\n        goto error;')

    # Accepting PREVENT/ALLOW on a non-existent LUN is misleading. The
    # core's built-in commands use the guarded LUN getter/readiness path.
    text = once(text,
        "int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void *buffer, uint16_t bufsize)\n{\n    int32_t ret;",
        "int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void *buffer, uint16_t bufsize)\n{\n"
        "    if (lun != 0) {\n"
        "        tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, SCSI_CODE_ASC_INVALID_COMMAND_OPERATION_CODE, SCSI_CODE_ASCQ);\n"
        "        return -1;\n"
        "    }\n    int32_t ret;")

    # These would silently re-introduce false success if upstream changes.
    if "msc_storage_write_sector_deferred" in text or "usbd_defer_func(" in text:
        raise ValueError("unsafe deferred write still exists")
    return text


if __name__ == "__main__":
    source = Path(sys.argv[1]).read_text()
    Path(sys.argv[2]).write_text(patch_source(source))
