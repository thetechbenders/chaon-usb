"""Source-level safeguards against accidental app-owned MSC media or formatting.

These checks are conservative. They cannot establish physical SD or SCSI behavior.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
MAIN = ROOT / "main"


def uncommented(source: str) -> str:
    """Remove C comments before inspecting policy-bearing source."""
    return re.sub(r"/\*.*?\*/|//[^\n]*", "", source, flags=re.S)


def all_main_sources() -> dict[str, str]:
    return {str(p.relative_to(ROOT)): uncommented(p.read_text())
            for p in MAIN.rglob("*") if p.suffix in {".c", ".h"}}


class UsbMscPolicyTests(unittest.TestCase):
    def test_storage_driver_disables_automatic_app_mount(self):
        source = all_main_sources()["main/chaon_usb_storage.c"]
        self.assertRegex(source, r"\.auto_mount_off\s*=\s*(?:true|1)\b")
        self.assertIn("tinyusb_msc_install_driver(&driver)", source)
        self.assertLess(source.index("tinyusb_msc_install_driver(&driver)"),
                        source.index("tinyusb_msc_new_storage_sdmmc(&config, out)"))

    def test_storage_is_usb_owned_and_no_format_paths(self):
        files = all_main_sources()
        source = files["main/chaon_usb_storage.c"]
        self.assertRegex(source, r"\.do_not_format\s*=\s*true\b")
        self.assertRegex(source, r"\.mount_point\s*=\s*TINYUSB_MSC_STORAGE_MOUNT_USB\b")
        self.assertRegex(source, r"\.format_if_mount_failed\s*=\s*false\b")
        banned = (
            r"\btinyusb_msc_format_storage\s*\(",
            r"\btinyusb_msc_set_storage_mount_point\s*\(",
            r"\bf_mkfs\s*\(",
            r"\besp_vfs_fat_sdcard_format\s*\(",
            r"\.format_if_mount_failed\s*=\s*(?:true|1)\b",
            r"\.do_not_format\s*=\s*(?:false|0)\b",
            r"\.auto_mount_off\s*=\s*(?:false|0)\b",
        )
        for path, text in files.items():
            for pattern in banned:
                with self.subTest(path=path, pattern=pattern):
                    self.assertNotRegex(text, pattern)

    def test_no_usb_driver_after_backend_failure(self):
        source = all_main_sources()["main/app_main.c"]
        self.assertIn("if (!media_ok)", source)
        self.assertLess(source.index("if (!media_ok)"),
                        source.index("ESP_ERROR_CHECK(tinyusb_driver_install(&usb));"))
        segment = source[source.index("if (!media_ok)"):
                         source.index("ESP_ERROR_CHECK(tinyusb_driver_install(&usb));")]
        self.assertIn("for (;;)", segment)

    def test_unexpected_mode_cannot_select_hid(self):
        source = all_main_sources()["main/app_main.c"]
        guard = source.index("if (mode != CHAON_USB_HID)")
        install = source.index("usb.descriptor.device = &hid_device_descriptor;")
        self.assertLess(guard, install)
        self.assertIn("for (;;)", source[guard:install])


if __name__ == "__main__":
    unittest.main()
