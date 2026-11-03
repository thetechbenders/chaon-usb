"""Strict-contract tests for the pinned TinyUSB transport source patch."""
import hashlib
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    "msc_transport_patch", ROOT / "tools/patch_msc_transport.py")
patch = importlib.util.module_from_spec(spec)
spec.loader.exec_module(patch)

FIXTURE = b'''/* representative pinned callback anchor */
static uint8_t rdwr10_validate_cmd(msc_cbw_t const* cbw) {
  uint8_t status = MSC_CSW_STATUS_PASSED;
  uint16_t const block_count = rdwr10_get_blockcount(cbw);
  if (block_count == 0) return MSC_CSW_STATUS_FAILED;
  return status;
}
'''


class TransportPatchTests(unittest.TestCase):
    def test_exact_hash_and_single_insertion(self):
        result = patch.patch_source(
            FIXTURE, hashlib.sha256(FIXTURE).hexdigest())
        self.assertEqual(result.count("ChaoN: inconsistent READ10/WRITE10 CBW geometry"), 1)
        self.assertIn("cbw->total_bytes != (uint32_t)block_count * 512u", result)
        self.assertIn("media_sectors - lba", result)
        self.assertIn("tud_msc_capacity_cb(cbw->lun", result)
        self.assertLess(result.index("tud_msc_capacity_cb(cbw->lun"),
                        result.index("if (block_count == 0)"))

    def test_hash_drift_rejected(self):
        with self.assertRaises(ValueError):
            patch.patch_source(FIXTURE, "0" * 64)

    def test_missing_or_duplicate_anchor_rejected(self):
        for source in (FIXTURE.replace(b"rdwr10_validate_cmd", b"renamed"),
                       FIXTURE + FIXTURE):
            with self.subTest(source_len=len(source)), self.assertRaises(ValueError):
                patch.patch_source(source, hashlib.sha256(source).hexdigest())


if __name__ == "__main__":
    unittest.main()
