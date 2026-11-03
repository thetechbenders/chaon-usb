"""Fail the ESP-IDF CI build if the compiled MSC source loses safety guards.

This inspects the *real* build-local source after both pinned source transforms,
not the synthetic fixture used by unit tests.
"""
import re
import sys
from pathlib import Path


def verify(source: str, config: str) -> None:
    setting = re.search(r"^#define CONFIG_TINYUSB_MSC_BUFSIZE\s+(\d+)\s*$",
                        config, flags=re.MULTILINE)
    if setting is None or int(setting.group(1)) != 512:
        raise ValueError("Original-board MSC test profile requires a 512-byte FIFO")
    required_counts = {
        "if (!chaon_msc_host_ready(lun))": 2,
        "if (!chaon_msc_request_valid(lun, lba, offset, bufsize))": 2,
        "chaon_msc_transfer_valid(lba, offset, bytes,": 1,
        "chaon_msc_device_start_stop(lun, start, load_eject)": 1,
        "chaon_msc_write_failed()": 1,
        "chaon_msc_write_verified(lun, lba, offset, buffer, bufsize)": 1,
        "static uint8_t chaon_msc_verify_buffer[MSC_STORAGE_BUFFER_SIZE]": 1,
        "return bytes <= MSC_STORAGE_BUFFER_SIZE &&": 1,
        "msc_storage_write_sector(lun, lba, offset, bufsize, buffer)": 1,
        "p_msc_driver->dynamic.storage[lun] != NULL": 1,
        "_Static_assert(CONFIG_TINYUSB_MSC_BUFSIZE >= 512": 1,
    }
    for needle, expected in required_counts.items():
        count = source.count(needle)
        if count != expected:
            raise ValueError(f"Generated MSC guard {needle!r}: expected {expected}, found {count}")
    forbidden = ("msc_storage_write_sector_deferred(", "usbd_defer_func(",
                 "msc_storage_mount_to_app();")
    for needle in forbidden:
        if needle in source:
            raise ValueError(f"Unsafe MSC path survived: {needle}")
    # All physical media accesses must be gated; don't confuse the helper
    # definition with a call in each callback.
    for name in ("read", "write"):
        begin = source.index(f"int32_t tud_msc_{name}10_cb(")
        end = source.find("\n}", begin)
        if end < 0:
            raise ValueError(f"MSC {name} callback did not end")
        body = source[begin:end]
        if body.index("chaon_msc_request_valid(") > body.index(
            f"msc_storage_{name}_sector(" if name == "read"
            else "msc_storage_write_sector("):
            raise ValueError(f"MSC {name} guard runs after storage access")


def main() -> None:
    if len(sys.argv) != 3:
        raise SystemExit("usage: verify_generated_msc.py GENERATED_MSC.c SDKCONFIG.h")
    verify(Path(sys.argv[1]).read_text(), Path(sys.argv[2]).read_text())
    print("Generated MSC callbacks, transfer guards, and 512-byte target profile verified")


if __name__ == "__main__":
    main()
