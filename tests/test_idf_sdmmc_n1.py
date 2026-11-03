"""Host checks for the exact, build-local ESP-IDF v5.5.3 N1 correction."""
import hashlib
import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("sdmmc_n1", ROOT / "tools/patch_idf_sdmmc_n1.py")
patch = importlib.util.module_from_spec(spec)
spec.loader.exec_module(patch)


class N1PatchTests(unittest.TestCase):
    def test_exact_block_and_error_propagation(self):
        data = ("/* fixture */\n" + patch.OLD + "\n").encode()
        result = patch.patch_source(data, patch.git_blob_hash(data))
        self.assertNotIn(patch.OLD, result)
        self.assertIn("return idle_err;", result)
        self.assertIn("idle_err != ESP_OK", result)
        self.assertIn("post-write sdmmc_wait_for_idle failed", result)

    def test_hash_and_anchor_drift_fail_closed(self):
        data = patch.OLD.encode()
        with self.assertRaises(ValueError):
            patch.patch_source(data, "0" * 40)
        for broken in (data + data, b"/* no matching completion */"):
            with self.assertRaises(ValueError):
                patch.patch_source(broken, patch.git_blob_hash(broken))


if __name__ == "__main__":
    unittest.main()
