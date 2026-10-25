#!/usr/bin/env python3
"""Stage and verify original T-Dongle-S3 experimental firmware inputs.

No flash operations, uploads, signing claims, or release creation.
"""
import argparse
import hashlib
import json
import pathlib
import re
import shutil
import sys
import tempfile
import zipfile

REQUIRED = (
    ("bootloader/bootloader.bin", "0x0"),
    ("partition_table/partition-table.bin", "0x8000"),
    ("chaon.bin", "0x10000"),
)
METADATA = ("flash_args", "flasher_args.json", "dependencies.lock")
SHA = re.compile(r"^[0-9a-f]{64}$")
COMMIT = re.compile(r"^[0-9a-f]{40}$")


def digest(data):
    return hashlib.sha256(data).hexdigest()


def read_inputs(root):
    files = {}
    for name in [p for p, _ in REQUIRED] + [p for p in METADATA if p != "dependencies.lock"]:
        p = root / name
        if not p.is_file() or p.is_symlink():
            raise ValueError(f"missing/unsafe required input: {name}")
        data = p.read_bytes()
        if not data:
            raise ValueError(f"empty input: {name}")
        files[name] = data
    return files


def manifest_for(files, source, firmware_sha, idf, component):
    if not COMMIT.fullmatch(source):
        raise ValueError("source SHA must be an exact 40-character lowercase git SHA")
    if not SHA.fullmatch(firmware_sha):
        raise ValueError("firmware source tree hash must be lowercase SHA-256")
    if idf != "v5.5.3" or component != "2.0.0":
        raise ValueError("unexpected ESP-IDF or esp_tinyusb version")
    return {
        "schema_version": 1,
        "status": "EXPERIMENTAL_CANDIDATE_NOT_RELEASED",
        "board": "original LILYGO T-Dongle-S3, ESP32-S3, 16MB flash, no PSRAM",
        "source_commit": source,
        "source_tree_sha256": firmware_sha,
        "esp_idf_version": idf,
        "esp_tinyusb_version": component,
        "image_offsets": {name: offset for name, offset in REQUIRED},
        "files": {name: {"bytes": len(data), "sha256": digest(data)}
                  for name, data in sorted(files.items())},
        "limitations": "See docs/phase2e2-hardware-evidence.md; no product qualification",
    }


def stable_json(value):
    return (json.dumps(value, sort_keys=True, indent=2) + "\n").encode("utf-8")


def make_archive(out, files, manifest):
    out.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=out.parent, suffix=".zip", delete=False) as temp:
        temp_path = pathlib.Path(temp.name)
    try:
        with zipfile.ZipFile(temp_path, "w", compression=zipfile.ZIP_STORED) as archive:
            payloads = dict(files)
            payloads["manifest.json"] = stable_json(manifest)
            for name, data in sorted(payloads.items()):
                info = zipfile.ZipInfo(name, date_time=(2020, 1, 1, 0, 0, 0))
                info.compress_type = zipfile.ZIP_STORED
                info.external_attr = 0o100644 << 16
                archive.writestr(info, data)
        temp_path.replace(out)
    finally:
        temp_path.unlink(missing_ok=True)


def verify_archive(path, expected_source=None):
    with zipfile.ZipFile(path) as archive:
        names = archive.namelist()
        expected = set([p for p, _ in REQUIRED] + list(METADATA) + ["manifest.json"])
        if len(names) != len(expected) or set(names) != expected:
            raise ValueError("archive has missing, duplicate or unexpected entries")
        manifest = json.loads(archive.read("manifest.json"))
        if expected_source and manifest.get("source_commit") != expected_source:
            raise ValueError("source commit mismatch")
        if manifest.get("schema_version") != 1:
            raise ValueError("unsupported manifest schema")
        if manifest.get("image_offsets") != {name: offset for name, offset in REQUIRED}:
            raise ValueError("image offset mismatch")
        entries = manifest.get("files")
        if not isinstance(entries, dict) or set(entries) != expected - {"manifest.json"}:
            raise ValueError("manifest file-set mismatch")
        for name, entry in entries.items():
            data = archive.read(name)
            if entry != {"bytes": len(data), "sha256": digest(data)}:
                raise ValueError(f"image or metadata integrity failure: {name}")
        return manifest


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="action", required=True)
    pack = commands.add_parser("pack")
    pack.add_argument("--build-dir", type=pathlib.Path, required=True)
    pack.add_argument("--lock", type=pathlib.Path, required=True)
    pack.add_argument("--source-commit", required=True)
    pack.add_argument("--source-tree-sha256", required=True)
    pack.add_argument("--out", type=pathlib.Path, required=True)
    check = commands.add_parser("verify")
    check.add_argument("--archive", type=pathlib.Path, required=True)
    check.add_argument("--source-commit")
    args = parser.parse_args(argv)
    try:
        if args.action == "pack":
            files = read_inputs(args.build_dir)
            if not args.lock.is_file() or args.lock.is_symlink():
                raise ValueError("missing or unsafe dependency lock")
            files["dependencies.lock"] = args.lock.read_bytes()
            if not files["dependencies.lock"]:
                raise ValueError("empty dependency lock")
            manifest = manifest_for(files, args.source_commit,
                                    args.source_tree_sha256, "v5.5.3", "2.0.0")
            make_archive(args.out, files, manifest)
            verify_archive(args.out, args.source_commit)
            print(f"Candidate: {args.out} SHA-256 {digest(args.out.read_bytes())}")
        else:
            manifest = verify_archive(args.archive, args.source_commit)
            print(f"VERIFIED: {manifest['source_commit']}")
    except (ValueError, OSError, KeyError, json.JSONDecodeError, zipfile.BadZipFile) as err:
        parser.exit(2, f"ERROR: {err}\n")


if __name__ == "__main__":
    main()
