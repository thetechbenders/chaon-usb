"""Patch isolation and fail-closed validation; real source hash checked at build."""
import hashlib
import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location("patch", Path(__file__).resolve().parents[1] / "tools/patch_msc_activity.py")
patch = importlib.util.module_from_spec(spec)
spec.loader.exec_module(patch)

class PatchTests(unittest.TestCase):
    def setUp(self):
        self.source = ("/* SPDX-License-Identifier: Apache-2.0 */\n"
            "int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize)\n{\n    return bufsize;\n}\n"
            "int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize)\n{\n    return bufsize;\n}\n"
            "static void tusb_write_func(void *p)\n{\n    if (err != ESP_OK) {\n        ESP_LOGE(TAG, \"Write failed, error=0x%x\", err);\n    }\n}\n").encode()

    def test_changes_only_two_entry_signals(self):
        result = patch.patch_source(self.source, hashlib.sha256(self.source).hexdigest())
        for event in (1, 2):
            insertion = ("    /* ChaoN modification: nonblocking data-attempt signal only. */\n"
                "    extern void chaon_msc_activity_event(unsigned int event);\n"
                f"    if (bufsize != 0) chaon_msc_activity_event({event}u);\n")
            self.assertEqual(result.count(insertion), 1)
            result = result.replace(insertion, "")
        fault_insertion = ("\n        /* ChaoN modification: sticky write-failure evidence. */"
            "\n        extern void chaon_msc_write_failure(void);"
            "\n        chaon_msc_write_failure();")
        self.assertEqual(result.count(fault_insertion), 1)
        result = result.replace(fault_insertion, "")
        self.assertEqual(result.encode(), self.source)

    def test_changed_source_rejected(self):
        with self.assertRaises(ValueError):
            patch.patch_source(self.source)

    def test_duplicate_or_missing_anchor_rejected(self):
        for source in (self.source * 2, b"missing"):
            with self.assertRaises(ValueError):
                patch.patch_source(source, hashlib.sha256(source).hexdigest())

if __name__ == "__main__":
    unittest.main()
