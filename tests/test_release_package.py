#!/usr/bin/env python3
"""Offline release-package integrity and negative-path tests."""
import importlib.util
import json
from unittest import mock
import pathlib
import tempfile
import unittest
import zipfile

SCRIPT = pathlib.Path(__file__).resolve().parents[1] / "tools" / "release_package.py"
spec = importlib.util.spec_from_file_location("release_package", SCRIPT)
pkg = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pkg)
SOURCE = "a" * 40
TREE = "b" * 64


class PackagingTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = pathlib.Path(self.tmp.name)
        self.files = {name: (name + "-fixture").encode()
                      for name, _ in pkg.REQUIRED}
        self.files.update({name: name.encode() for name in pkg.METADATA})
        self.archive = self.root / "candidate.zip"

    def create(self):
        manifest = pkg.manifest_for(self.files, SOURCE, TREE, "v5.5.3", "2.0.0")
        pkg.make_archive(self.archive, self.files, manifest)
        return manifest

    def test_roundtrip_and_determinism(self):
        manifest = self.create()
        first = self.archive.read_bytes()
        self.create()
        self.assertEqual(first, self.archive.read_bytes())
        self.assertEqual(manifest, pkg.verify_archive(self.archive, SOURCE))

    def test_source_mismatch_rejected(self):
        self.create()
        with self.assertRaisesRegex(ValueError, "source commit mismatch"):
            pkg.verify_archive(self.archive, "c" * 40)

    def test_missing_input_rejected(self):
        root = self.root / "build"
        root.mkdir()
        with self.assertRaisesRegex(ValueError, "missing/unsafe"):
            pkg.read_inputs(root)

    def test_version_guard(self):
        with self.assertRaisesRegex(ValueError, "unexpected"):
            pkg.manifest_for(self.files, SOURCE, TREE, "v5.6.0", "2.0.0")

    def test_modified_firmware_rejected(self):
        self.create()
        contents = {}
        with zipfile.ZipFile(self.archive) as archive:
            for name in archive.namelist():
                contents[name] = archive.read(name)
        contents["chaon.bin"] = b"TAMPERED"
        with zipfile.ZipFile(self.archive, "w") as archive:
            for name, data in contents.items():
                archive.writestr(name, data)
        with self.assertRaisesRegex(ValueError, "integrity mismatch"):
            pkg.verify_archive(self.archive)

    def test_unexpected_entry_rejected(self):
        self.create()
        with zipfile.ZipFile(self.archive, "a") as archive:
            archive.writestr("additional.bin", b"unexpected")
        with self.assertRaisesRegex(ValueError, "unexpected entries"):
            pkg.verify_archive(self.archive)

    def mutate_manifest(self, mutate):
        self.create()
        with zipfile.ZipFile(self.archive) as archive:
            payloads = {name: archive.read(name) for name in archive.namelist()}
        manifest = json.loads(payloads["manifest.json"])
        mutate(manifest)
        payloads["manifest.json"] = pkg.stable_json(manifest)
        with zipfile.ZipFile(self.archive, "w") as archive:
            for name, data in payloads.items():
                archive.writestr(name, data)

    def test_manifest_provenance_tampering_rejected(self):
        changes = (
            lambda m: m.update(esp_idf_version="v9.9.9-EVIL"),
            lambda m: m.update(esp_tinyusb_version="9.9.9"),
            lambda m: m.update(board="attacker"),
            lambda m: m.pop("source_tree_sha256"),
            lambda m: m.update(source_tree_sha256="bogus"),
            lambda m: m.update(status="RELEASED"),
            lambda m: m.pop("limitations"),
            lambda m: m.update(unexpected="field"),
            lambda m: m.update(source_commit="not-a-commit"),
        )
        for change in changes:
            with self.subTest(change=repr(change)):
                self.mutate_manifest(change)
                with self.assertRaises(ValueError):
                    pkg.verify_archive(self.archive)

    def test_verification_failure_preserves_prior_archive(self):
        self.create()
        before = self.archive.read_bytes()
        with mock.patch.object(pkg, "verify_archive", side_effect=ValueError("bad temp")):
            with self.assertRaisesRegex(ValueError, "bad temp"):
                pkg.make_archive(self.archive, self.files,
                                 pkg.manifest_for(self.files, SOURCE, TREE,
                                                  "v5.5.3", "2.0.0"))
        self.assertEqual(self.archive.read_bytes(), before)
        self.assertEqual(list(self.root.glob("tmp*.zip")), [])

    def test_offsets_are_fixed(self):
        self.create()
        self.assertEqual(pkg.verify_archive(self.archive)["image_offsets"],
                         {"bootloader/bootloader.bin": "0x0",
                          "partition_table/partition-table.bin": "0x8000",
                          "chaon.bin": "0x10000"})


if __name__ == "__main__":
    unittest.main()
