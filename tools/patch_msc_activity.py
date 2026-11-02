"""Isolated integration patch for Apache-2.0 esp_tinyusb 2.0.0.

Generated source retains upstream copyright/license notices. No storage behavior
or return value changes; hook reports attempts, never durability or success.
"""
import hashlib
from pathlib import Path
import sys

SOURCE_SHA256 = "5888e22cbaf948ef74676111c6f0e7a78187a5fac9879ec0b7f34b8deb0a3051"


def patch_source(source, expected_sha=SOURCE_SHA256):
    if hashlib.sha256(source).hexdigest() != expected_sha:
        raise ValueError("esp_tinyusb MSC source changed; review hook before building")
    text = source.decode("utf-8")
    for direction, buffer in (("read", "void *buffer"), ("write", "uint8_t *buffer")):
        anchor = (f"int32_t tud_msc_{direction}10_cb(uint8_t lun, uint32_t lba, "
                  f"uint32_t offset, {buffer}, uint32_t bufsize)\n{{\n")
        if text.count(anchor) != 1:
            raise ValueError("MSC callback anchor changed")
        text = text.replace(anchor, anchor +
            "    /* ChaoN modification: nonblocking data-attempt signal only. */\n"
            "    extern void chaon_msc_activity_event(unsigned int event);\n"
            f"    if (bufsize != 0) chaon_msc_activity_event({1 if direction == 'read' else 2}u);\n")
    # Deferred SD writes may fail after the host has received GOOD status.
    # Report this failure independently from transfer-attempt pulses.
    failure = 'ESP_LOGE(TAG, "Write failed, error=0x%x", err);'
    if text.count(failure) != 1:
        raise ValueError("deferred MSC write-failure path changed")
    text = text.replace(failure, failure +
        "\\n        /* ChaoN modification: sticky write-failure evidence. */" +
        "\\n        extern void chaon_msc_write_failure(void);" +
        "\\n        chaon_msc_write_failure();")
    return text


if __name__ == "__main__":
    Path(sys.argv[2]).write_text(patch_source(Path(sys.argv[1]).read_bytes()))
