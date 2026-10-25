#!/usr/bin/env python3
"""Stage and verify original T-Dongle-S3 experimental firmware inputs.

No flash operations, uploads, signing claims, or release creation.
"""
import argparse
import hashlib
import json
import pathlib
import re
import shlex
import struct
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



# Release policy for the hardware-tested original S3 configuration.
FLASH_SETTINGS = {"flash_mode": "dio", "flash_freq": "80m", "flash_size": "16MB"}
APP_PARTITION_SIZE = 0x100000
FLASH_CAPACITY = 0x1000000
CHIP_ID_ESP32S3 = 9


def _parse_flash_args(files):
    try:
        args = json.loads(files["flasher_args.json"])
    except (UnicodeDecodeError, json.JSONDecodeError) as err:
        raise ValueError("invalid flasher_args.json") from err
    if not isinstance(args, dict):
        raise ValueError("flash arguments must be an object")
    layout = args.get("flash_files")
    if not isinstance(layout, dict) or len(layout) != len(REQUIRED):
        raise ValueError("unexpected flash_files layout")
    offsets = {}
    for offset, name in layout.items():
        try:
            address = int(offset, 0)
        except (ValueError, TypeError) as err:
            raise ValueError("invalid flash offset") from err
        if address in offsets or not isinstance(name, str):
            raise ValueError("duplicate flash offset or invalid filename")
        offsets[address] = name
    expected = {int(offset, 0): name for name, offset in REQUIRED}
    if offsets != expected:
        raise ValueError("flash layout differs from expected original-board layout")
    settings = args.get("flash_settings")
    if settings != FLASH_SETTINGS:
        raise ValueError("flash settings differ from original-board policy")
    extra = args.get("extra_esptool_args")
    if not isinstance(extra, dict) or extra.get("chip") != "esp32s3":
        raise ValueError("wrong or absent ESP32-S3 target")
    def parse_flags(tokens):
        if not isinstance(tokens, list) or not all(isinstance(x, str) for x in tokens):
            raise ValueError("invalid flash option tokens")
        opts = {}
        index = 0
        while index < len(tokens) and tokens[index].startswith("--"):
            key = tokens[index].replace("_", "-")
            if key not in ("--flash-mode", "--flash-size", "--flash-freq"):
                raise ValueError("unexpected flash parameter")
            if key in opts or index + 1 >= len(tokens):
                raise ValueError("duplicated/missing flash parameter")
            opts[key] = tokens[index + 1]
            index += 2
        expected = {"--flash-mode": "dio", "--flash-size": "16MB",
                    "--flash-freq": "80m"}
        if opts != expected:
            raise ValueError("flash parameters disagree with original-board policy")
        return tokens[index:]

    if parse_flags(args.get("write_flash_args")):
        raise ValueError("unexpected trailing write_flash_args")
    try:
        positional = shlex.split(files["flash_args"].decode("utf-8"))
    except (UnicodeDecodeError, ValueError) as err:
        raise ValueError("invalid flash_args") from err
    remaining = parse_flags(positional)
    if len(remaining) != 2 * len(REQUIRED):
        raise ValueError("flash_args region count mismatch")
    actual = {}
    for i in range(0, len(remaining), 2):
        try:
            offset = int(remaining[i], 0)
        except ValueError as err:
            raise ValueError("invalid flash_args offset") from err
        if offset in actual:
            raise ValueError("duplicate flash_args offset")
        actual[offset] = remaining[i + 1]
    if actual != expected:
        raise ValueError("flash_args layout disagree with flasher_args.json")
    return {name: hex(offset) for offset, name in sorted(expected.items())}


def _validate_esp_image(data, filename, max_size):
    """Validate Espressif image header, segments, XOR checksum, optional SHA-256."""
    if len(data) < 33 or len(data) > max_size or data[0] != 0xE9:
        raise ValueError(f"invalid or oversized ESP image: {filename}")
    count = data[1]
    if not 1 <= count <= 16:
        raise ValueError(f"invalid image segment count: {filename}")
    chip = struct.unpack_from("<H", data, 12)[0]
    if chip != CHIP_ID_ESP32S3:
        raise ValueError(f"wrong image chip ID: {filename}")
    if data[23] != 1:
        raise ValueError(f"missing image SHA-256: {filename}")
    cursor = 24
    checksum = 0xEF
    for _ in range(count):
        if cursor + 8 > len(data):
            raise ValueError(f"truncated segment header: {filename}")
        seg_length = struct.unpack_from("<I", data, cursor + 4)[0]
        cursor += 8
        if seg_length > max_size or cursor + seg_length > len(data):
            raise ValueError(f"invalid segment bounds: {filename}")
        for byte in data[cursor:cursor + seg_length]:
            checksum ^= byte
        cursor += seg_length
    footer = (cursor + 15) & ~15
    if footer + 32 != len(data) or data[footer - 1] != checksum:
        raise ValueError(f"image checksum/length mismatch: {filename}")
    if any(data[cursor:footer - 1]):
        raise ValueError(f"nonzero image padding: {filename}")
    if hashlib.sha256(data[:footer]).digest() != data[footer:]:
        raise ValueError(f"ESP image SHA-256 mismatch: {filename}")


def validate_flash_package(files):
    offsets = _parse_flash_args(files)
    for name, _ in REQUIRED:
        max_size = (APP_PARTITION_SIZE if name == "chaon.bin"
                    else (0x8000 if name.startswith("bootloader/")
                          else FLASH_CAPACITY))
        if name == "partition_table/partition-table.bin":
            data = files[name]
            if len(data) != 0xC00 or data[:2] != b"\\xaa\\x50":
                raise ValueError("invalid ESP partition table")
            continue
        _validate_esp_image(files[name], name, max_size)
    for name, offset in REQUIRED:
        if int(offset, 0) + len(files[name]) > FLASH_CAPACITY:
            raise ValueError("image exceeds physical flash capacity")
    return offsets

def manifest_for(files, source, firmware_sha, idf, component):
    if not COMMIT.fullmatch(source):
        raise ValueError("source SHA must be an exact 40-character lowercase git SHA")
    if not SHA.fullmatch(firmware_sha):
        raise ValueError("firmware source tree hash must be lowercase SHA-256")
    if idf != "v5.5.3" or component != "2.0.0":
        raise ValueError("unexpected ESP-IDF or esp_tinyusb version")
    validated_offsets = validate_flash_package(files)
    return {
        "schema_version": 1,
        "status": "EXPERIMENTAL_CANDIDATE_NOT_RELEASED",
        "board": "original LILYGO T-Dongle-S3, ESP32-S3, 16MB flash, no PSRAM",
        "source_commit": source,
        "source_tree_sha256": firmware_sha,
        "esp_idf_version": idf,
        "esp_tinyusb_version": component,
        "image_offsets": validated_offsets,
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
        # Verify before replacing an existing known-good candidate.
        verify_archive(temp_path, manifest["source_commit"])
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
        if not isinstance(manifest, dict):
            raise ValueError("manifest must be a JSON object")
        entries = manifest.get("files")
        if not isinstance(entries, dict) or set(entries) != expected - {"manifest.json"}:
            raise ValueError("manifest file-set mismatch")
        # Rebuild every field from the packaged bytes and the validated,
        # pinned metadata policy. Unknown or removed keys cannot pass.
        source = manifest.get("source_commit")
        tree = manifest.get("source_tree_sha256")
        if not isinstance(source, str) or not COMMIT.fullmatch(source):
            raise ValueError("invalid source commit")
        if not isinstance(tree, str) or not SHA.fullmatch(tree):
            raise ValueError("invalid source tree SHA-256")
        if expected_source and source != expected_source:
            raise ValueError("source commit mismatch")
        files = {name: archive.read(name) for name in sorted(entries)}
        canonical = manifest_for(files, source, tree, "v5.5.3", "2.0.0")
        if manifest != canonical:
            raise ValueError("manifest metadata or integrity mismatch")
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
            print(f"Candidate: {args.out} SHA-256 {digest(args.out.read_bytes())}")
        else:
            manifest = verify_archive(args.archive, args.source_commit)
            print(f"VERIFIED: {manifest['source_commit']}")
    except (ValueError, OSError, KeyError, json.JSONDecodeError, zipfile.BadZipFile) as err:
        parser.exit(2, f"ERROR: {err}\n")


if __name__ == "__main__":
    main()
