"""Compile the *actual generated* TinyUSB RW10 command validator on the host.

The test extracts the upstream-patched validator and its byte-order helpers,
then simulates CBW fields and a media-capacity callback. No hardware writes.
"""
from pathlib import Path
import subprocess
import sys
import tempfile


FUNCTIONS = (
    "TU_ATTR_ALWAYS_INLINE static inline uint32_t rdwr10_get_lba(",
    "TU_ATTR_ALWAYS_INLINE static inline uint16_t rdwr10_get_blockcount(",
    "static uint8_t rdwr10_validate_cmd(",
)


def extract(source: str, prefix: str) -> str:
    if source.count(prefix) != 1:
        raise ValueError(f"Missing/duplicate TinyUSB transport symbol: {prefix}")
    begin = source.index(prefix)
    opening = source.index("{", begin)
    depth = 0
    for end in range(opening, len(source)):
        if source[end] == "{":
            depth += 1
        elif source[end] == "}":
            depth -= 1
            if depth == 0:
                return source[begin:end + 1]
    raise ValueError(f"Unclosed TinyUSB function {prefix}")


HEADER = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#define TU_ATTR_ALWAYS_INLINE
#define TU_LOG_DRV(...) ((void)0)
enum { MSC_CSW_STATUS_PASSED = 0, MSC_CSW_STATUS_FAILED = 1,
       MSC_CSW_STATUS_PHASE_ERROR = 2 };
enum { SCSI_CMD_READ_10 = 0x28, SCSI_CMD_WRITE_10 = 0x2a };
enum { SCSI_SENSE_NOT_READY = 2, SCSI_SENSE_ILLEGAL_REQUEST = 5 };
typedef struct __attribute__((packed)) {
    uint8_t cmd_code, reserved;
    uint32_t lba;
    uint8_t reserved2;
    uint16_t block_count;
    uint8_t control;
} scsi_write10_t;
typedef struct {
    uint32_t total_bytes;
    uint8_t dir, lun;
    uint8_t command[16];
} msc_cbw_t;
static uint32_t media_count = 100;
static uint16_t media_size = 512;
static unsigned int status_calls;
static uint8_t sense_key, sense_asc;
static void tud_msc_capacity_cb(uint8_t lun, uint32_t *count, uint16_t *size)
{
    ++status_calls;
    *count = lun == 0 ? media_count : 0;
    *size = lun == 0 ? media_size : 0;
}
static bool tud_msc_set_sense(uint8_t lun, uint8_t key, uint8_t asc, uint8_t q)
{
    (void)lun; (void)q;
    sense_key = key;
    sense_asc = asc;
    return true;
}
static bool is_data_in(uint8_t dir) { return (dir & 0x80u) != 0u; }
static uint32_t tu_unaligned_read32(const void *p)
{
    uint32_t v;
    memcpy(&v, p, sizeof(v));
    return v;
}
static uint16_t tu_unaligned_read16(const void *p)
{
    uint16_t v;
    memcpy(&v, p, sizeof(v));
    return v;
}
static uint32_t tu_ntohl(uint32_t n) { return __builtin_bswap32(n); }
static uint16_t tu_ntohs(uint16_t n) { return __builtin_bswap16(n); }
'''

TEST = r'''
static msc_cbw_t cbw(uint8_t op, uint8_t lun, uint32_t lba,
                     uint16_t blocks, uint32_t bytes)
{
    msc_cbw_t c = {0};
    c.total_bytes = bytes;
    c.command[0] = op;
    c.dir = op == SCSI_CMD_READ_10 ? 0x80 : 0;
    c.lun = lun;
    c.command[2] = (uint8_t)(lba >> 24);
    c.command[3] = (uint8_t)(lba >> 16);
    c.command[4] = (uint8_t)(lba >> 8);
    c.command[5] = (uint8_t)lba;
    c.command[7] = (uint8_t)(blocks >> 8);
    c.command[8] = (uint8_t)blocks;
    return c;
}
static uint8_t check(uint8_t op, uint8_t lun, uint32_t lba,
                     uint16_t blocks, uint32_t bytes)
{
    msc_cbw_t c = cbw(op, lun, lba, blocks, bytes);
    return rdwr10_validate_cmd(&c);
}
int main(void)
{
    for (unsigned int index=0; index<2; ++index) {
        uint8_t op = index == 0 ? SCSI_CMD_WRITE_10 : SCSI_CMD_READ_10;
        assert(check(op, 0, 0, 1, 512) == MSC_CSW_STATUS_PASSED);
        assert(check(op, 0, 5, 4, 2048) == MSC_CSW_STATUS_PASSED);
        assert(check(op, 0, 99, 1, 512) == MSC_CSW_STATUS_PASSED);
        assert(check(op, 0, 0, 0, 0) == MSC_CSW_STATUS_PASSED);
        assert(check(op, 0, 5, 4, 1024) == MSC_CSW_STATUS_PHASE_ERROR);
        assert(check(op, 0, 5, 1, 513) == MSC_CSW_STATUS_PHASE_ERROR);
        assert(check(op, 0, 5, 1, 0) == MSC_CSW_STATUS_PHASE_ERROR);
        assert(check(op, 0, 5, 0, 512) == MSC_CSW_STATUS_PHASE_ERROR);
        assert(check(op, 0, 98, 4, 2048) == MSC_CSW_STATUS_FAILED);
        assert(check(op, 0, 100, 1, 512) == MSC_CSW_STATUS_FAILED);
        assert(check(op, 0, UINT32_MAX, 1, 512) == MSC_CSW_STATUS_FAILED);
        assert(check(op, 1, 0, 1, 512) == MSC_CSW_STATUS_FAILED);
        media_size = 1024;
        assert(check(op, 0, 0, 1, 512) == MSC_CSW_STATUS_FAILED);
        media_size = 512;
        media_count = 0;
        assert(check(op, 0, 0, 1, 512) == MSC_CSW_STATUS_FAILED);
        media_count = 100;
        assert(check(op, 0, 0, 65535, 33553920u) == MSC_CSW_STATUS_FAILED);
        assert(sense_key != 0 && sense_asc != 0);
    }
    assert(status_calls >= 30);
    puts("Generated TinyUSB CBW geometry test PASS");
    return 0;
}
'''


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit("usage: test_generated_msc_transport.py build/chaon_tinyusb_msc_transport.c")
    source = Path(sys.argv[1]).read_text()
    extracted = "\n\n".join(extract(source, prefix) for prefix in FUNCTIONS)
    with tempfile.TemporaryDirectory(prefix="chaon-cbw-harness-") as tmp:
        path = Path(tmp) / "cbw_test.c"
        binary = Path(tmp) / "cbw_test"
        path.write_text(HEADER + "\n" + extracted + "\n" + TEST)
        subprocess.run(["cc", "-std=c11", "-O1", "-g", "-Wall", "-Wextra",
                        "-Werror", "-Wpedantic", "-fsanitize=undefined",
                        "-fno-omit-frame-pointer", str(path), "-o", str(binary)],
                       check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
