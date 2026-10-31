"""Source-level guardrails for the pinned esp_tinyusb 2.0.0 MSC configuration.

These tests prevent accidental reintroduction of unsafe driver defaults.
They do not simulate FAT, SCSI, SD card faults, or a physical Windows eject.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
STORAGE = ROOT / "main" / "chaon_usb_storage.c"
APP = ROOT / "main" / "app_main.c"

class UsbMscPolicyTests(unittest.TestCase):
    def test_storage_driver_disables_automatic_app_mount(self):
        source = STORAGE.read_text()
        self.assertRegex(source, r"\\.auto_mount_off\\s*=\\s*(?:true|1)\\b")
        self.assertIn("tinyusb_msc_install_driver(&driver)", source)
        self.assertLess(source.index("tinyusb_msc_install_driver(&driver)"),
                        source.index("tinyusb_msc_new_storage_sdmmc(&config, out)"))

    def test_storage_never_formats_and_starts_usb_owned(self):
        source = STORAGE.read_text()
        self.assertRegex(source, r"\\.do_not_format\\s*=\\s*true\\b")
        self.assertRegex(source, r"\\.mount_point\\s*=\\s*TINYUSB_MSC_STORAGE_MOUNT_USB\\b")
        self.assertNotIn("tinyusb_msc_format_storage", source)

    def test_no_usb_driver_after_backend_failure(self):
        source = APP.read_text()
        self.assertIn("if (!media_ok)", source)
        self.assertLess(source.index("if (!media_ok)"),
                        source.index("ESP_ERROR_CHECK(tinyusb_driver_install(&usb));"))
        segment = source[source.index("if (!media_ok)"):
                         source.index("ESP_ERROR_CHECK(tinyusb_driver_install(&usb));")]
        self.assertIn("for (;;)", segment)

if __name__ == "__main__":
    unittest.main()
