"""Compile and exercise extracted *generated* MSC callbacks with fake SD I/O.

Runs on the CI host after ESP-IDF creates build/chaon_tinyusb_msc.c.
This validates the actual rewritten callback bodies, not a synthetic fixture.
The fake backend cannot prove hardware timing, USB transaction semantics or
real SD persistence.
"""
from pathlib import Path
import subprocess
import sys
import tempfile


FUNCTIONS = (
    "static bool chaon_msc_host_ready(",
    "static bool chaon_msc_request_valid(",
    "bool tud_msc_test_unit_ready_cb(",
    "bool tud_msc_start_stop_cb(",
    "int32_t tud_msc_read10_cb(",
    "int32_t tud_msc_write10_cb(",
)


def extract(source: str, prefix: str) -> str:
    if source.count(prefix) != 1:
        raise ValueError(f"Missing/duplicate generated callback: {prefix}")
    begin = source.index(prefix)
    opening = source.index("{", begin)
    depth = 0
    for offset in range(opening, len(source)):
        if source[offset] == "{":
            depth += 1
        elif source[offset] == "}":
            depth -= 1
            if depth == 0:
                return source[begin:offset + 1]
    raise ValueError(f"Unclosed callback: {prefix}")


HEADER = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "chaon_msc_eject.h"
#define ESP_OK 0
#define MSC_STORAGE_BUFFER_SIZE 512u
#define TINYUSB_MSC_STORAGE_MOUNT_USB 1
#define SCSI_SENSE_NOT_READY 2u
#define SCSI_SENSE_ILLEGAL_REQUEST 5u
#define SCSI_CODE_ASC_MEDIUM_NOT_PRESENT 0x3au
#define SCSI_CODE_ASCQ 0u
#define ESP_LOGE(...) ((void)0)
#define TAG "msc-test"
typedef int esp_err_t;
typedef struct {
    uint32_t sector_count;
    uint32_t sector_size;
    int mount_point;
} msc_storage_obj_t;
static msc_storage_obj_t fake_card = {100, 512, TINYUSB_MSC_STORAGE_MOUNT_USB};
static int read_count, write_count, sense_count;
static bool write_fault, write_backend_fail;
static uint8_t last_sense;
static int msc_storage_get_by_lun(uint8_t lun, msc_storage_obj_t **s)
{
    if (lun != 0) return -1;
    *s = &fake_card;
    return ESP_OK;
}
static esp_err_t msc_storage_read_sector(uint8_t lun, uint32_t lba,
    uint32_t offset, size_t size, void *dst)
{
    (void)lun; (void)lba; (void)offset; (void)size; (void)dst;
    ++read_count;
    return ESP_OK;
}
static esp_err_t msc_storage_write_sector(uint8_t lun, uint32_t lba,
    uint32_t offset, size_t size, const void *src)
{
    (void)lun; (void)lba; (void)offset; (void)size; (void)src;
    ++write_count;
    return write_backend_fail ? -1 : ESP_OK;
}
static bool tud_msc_set_sense(uint8_t lun, uint8_t key, uint8_t asc, uint8_t ascq)
{
    (void)lun; (void)key; (void)ascq;
    ++sense_count;
    last_sense = asc;
    return true;
}
bool chaon_msc_write_failed(void) { return write_fault; }
void chaon_msc_write_failure(void) { write_fault = true; }
void chaon_msc_activity_event(unsigned int event) { (void)event; }
'''

TEST = r'''
static void test_invalid(void)
{
    uint8_t buffer[1024] = {0};
    static const uint32_t invalid_sizes[] = {0, 1, 511, 513};
    for (unsigned int i=0; i < sizeof(invalid_sizes)/sizeof(invalid_sizes[0]); ++i) {
        uint32_t n = invalid_sizes[i];
        assert(tud_msc_write10_cb(0, 0, 0, buffer, n) == -1);
        assert(tud_msc_read10_cb(0, 0, 0, buffer, n) == -1);
    }
    assert(tud_msc_write10_cb(0, 0, 1, buffer, 512) == -1);
    assert(tud_msc_read10_cb(0, 0, 1, buffer, 512) == -1);
    assert(tud_msc_write10_cb(0, 100, 0, buffer, 512) == -1);
    assert(tud_msc_read10_cb(0, 100, 0, buffer, 512) == -1);
    assert(tud_msc_write10_cb(0, 99, 0, buffer, 1024) == -1);
    assert(tud_msc_read10_cb(0, 99, 0, buffer, 1024) == -1);
    assert(tud_msc_write10_cb(1, 0, 0, buffer, 512) == -1);
    assert(tud_msc_read10_cb(1, 0, 0, buffer, 512) == -1);
    assert(read_count == 0 && write_count == 0);
    assert(!write_fault);
}
int main(int argc, char **argv)
{
    uint8_t buffer[1024] = {0};
    (void)argc;
    assert(tud_msc_test_unit_ready_cb(0));
    assert(!tud_msc_test_unit_ready_cb(1));
    test_invalid();
    assert(tud_msc_write10_cb(0, 99, 0, buffer, 512) == 512);
    assert(tud_msc_read10_cb(0, 99, 0, buffer, 512) == 512);
    assert(read_count == 1 && write_count == 1);
    if (strcmp(argv[1], "fault") == 0) {
        write_backend_fail = true;
        assert(tud_msc_write10_cb(0, 0, 0, buffer, 512) == -1);
        assert(write_fault);
        assert(!tud_msc_test_unit_ready_cb(0));
        assert(tud_msc_write10_cb(0, 0, 0, buffer, 512) == -1);
        assert(tud_msc_read10_cb(0, 0, 0, buffer, 512) == -1);
        assert(write_count == 2 && read_count == 1);
    } else {
        assert(tud_msc_start_stop_cb(0, 0, false, true));
        assert(!tud_msc_test_unit_ready_cb(0));
        assert(tud_msc_write10_cb(0, 0, 0, buffer, 512) == -1);
        assert(tud_msc_read10_cb(0, 0, 0, buffer, 512) == -1);
        assert(write_count == 1 && read_count == 1);
        assert(!tud_msc_start_stop_cb(0, 0, true, true));
    }
    puts("Generated MSC callback harness PASS");
    return 0;
}
'''


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit("usage: test_generated_msc_callbacks.py build/chaon_tinyusb_msc.c")
    source = Path(sys.argv[1]).read_text()
    extracted = "\n\n".join(extract(source, symbol) for symbol in FUNCTIONS)
    with tempfile.TemporaryDirectory(prefix="chaon-msc-harness-") as tmp:
        path = Path(tmp) / "callback_harness.c"
        binary = Path(tmp) / "callback_harness"
        path.write_text(HEADER + "\n" + extracted + "\n" + TEST)
        root = Path(__file__).resolve().parents[1]
        subprocess.run(["cc", "-std=c11", "-O1", "-g", "-Wall", "-Wextra",
                        "-Werror", "-Wno-unused-variable",
                        "-I", str(root / "main"),
                        str(path), str(root / "main/chaon_msc_eject.c"),
                        "-o", str(binary)], check=True)
        for mode in ("eject", "fault"):
            subprocess.run([str(binary), mode], check=True)


if __name__ == "__main__":
    main()
