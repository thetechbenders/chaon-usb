#!/usr/bin/env python3
"""Offline release-package integrity and negative-path tests."""
import importlib.util
import json
import hashlib
import struct
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


def esp_image(chip=9, corrupt_hash=False):
    header = bytearray(24)
    header[:4] = bytes((0xE9, 1, 2, 0x4f))
    struct.pack_into("<H", header, 12, chip)
    header[23] = 1
    payload = bytes(range(16))
    image = bytes(header) + struct.pack("<II", 0x3c000020, len(payload)) + payload
    checksum = 0xEF
    for byte in payload:
        checksum ^= byte
    image += bytes((checksum,))
    image += bytes((-len(image)) % 16)
    result = image + hashlib.sha256(image).digest()
    if corrupt_hash:
        result = result[:-1] + bytes((result[-1] ^ 0xFF,))
    return result


def fixture_files():
    names = {name: esp_image() for name in
             ("bootloader/bootloader.bin", "chaon.bin")}
    names["partition_table/partition-table.bin"] = (
        b"\\xaa\\x50" + bytes(0xC00 - 2))
    names["flasher_args.json"] = json.dumps({
        "flash_files": {"0x0": "bootloader/bootloader.bin",
                        "0x8000": "partition_table/partition-table.bin",
                        "0x10000": "chaon.bin"},
        "flash_settings": {"flash_mode": "dio", "flash_freq": "80m",
                           "flash_size": "16MB"},
        "write_flash_args": ["--flash_mode", "dio", "--flash_size", "16MB",
                             "--flash_freq", "80m"],
        "extra_esptool_args": {"chip": "esp32s3"},
    }).encode()
    names["flash_args"] = (
        b"--flash_mode dio --flash_size 16MB --flash_freq 80m "
        b"0x0 bootloader/bootloader.bin "
        b"0x8000 partition_table/partition-table.bin "
        b"0x10000 chaon.bin")
    names["dependencies.lock"] = b"pinned-dependency-fixture"
    return names


class PackagingTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = pathlib.Path(self.tmp.name)
        self.files = fixture_files()
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

    def test_wrong_json_flash_offset_rejected(self):
        data = json.loads(self.files["flasher_args.json"])
        data["flash_files"]["0x20000"] = data["flash_files"].pop("0x10000")
        self.files["flasher_args.json"] = json.dumps(data).encode()
        with self.assertRaisesRegex(ValueError, "flash layout"):
            self.create()

    def test_conflicting_flash_args_rejected(self):
        self.files["flash_args"] = self.files["flash_args"].replace(
            b"0x10000 chaon.bin", b"0x20000 chaon.bin")
        with self.assertRaisesRegex(ValueError, "flash_args layout"):
            self.create()

    def test_wrong_chip_rejected(self):
        self.files["chaon.bin"] = esp_image(chip=0)
        with self.assertRaisesRegex(ValueError, "chip ID"):
            self.create()

    def test_truncated_and_corrupt_images_rejected(self):
        for candidate in (b"fake image", esp_image()[:-12],
                          esp_image(corrupt_hash=True)):
            with self.subTest(length=len(candidate)):
                self.files["chaon.bin"] = candidate
                with self.assertRaises(ValueError):
                    self.create()

    def test_oversized_app_rejected(self):
        self.files["chaon.bin"] = esp_image() + bytes(pkg.APP_PARTITION_SIZE)
        with self.assertRaisesRegex(ValueError, "oversized"):
            self.create()

    def test_corrupt_partition_table_rejected(self):
        self.files["partition_table/partition-table.bin"] = bytes(0xC00)
        with self.assertRaisesRegex(ValueError, "partition table"):
            self.create()

    def test_archive_revalidation_of_flash_metadata(self):
        self.mutate_manifest(lambda m: m.update(
            image_offsets={"chaon.bin": "0x20000"}))
        with self.assertRaises(ValueError):
            pkg.verify_archive(self.archive)

    def test_input_symlink_rejected(self):
        root = self.root / "build"
        root.mkdir()
        (root / "chaon.bin").symlink_to(self.archive)
        with self.assertRaisesRegex(ValueError, "missing/unsafe"):
            pkg.read_inputs(root)

    def test_offsets_are_fixed(self):
        self.create()
        self.assertEqual(pkg.verify_archive(self.archive)["image_offsets"],
                         {"bootloader/bootloader.bin": "0x0",
                          "partition_table/partition-table.bin": "0x8000",
                          "chaon.bin": "0x10000"})


if __name__ == "__main__":
    unittest.main()
