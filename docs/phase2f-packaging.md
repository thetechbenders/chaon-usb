# Phase 2F experimental firmware packaging (not a release)

**Status:** packaging candidate implemented; no published/downloadable firmware, release tag, hardware flash or production sign-off.

## Verified USB descriptor policy and remaining check

ChaoN currently uses `TINYUSB_DEFAULT_CONFIG()`, sets `usb.descriptor.device = NULL` and supplies its own HID configuration and strings. The original board enumerated `VID 303A:PID 4004`, serial `CHN-S3-DEV` during Phase 2E.2 acceptance. `main/idf_component.yml` pins `espressif/esp_tinyusb 2.0.0` with ESP-IDF 5.5.3.

Espressif's [USB VID/PID policy](https://docs.espressif.com/projects/esp-iot-solution/en/release-v2.0/usb/usb_overview/usb_vid_pid.html) permits its VID `0x303A` with TinyUSB's **default PID** for standard USB-class devices; no separate PID application is required under the documented conditions. The [esp_tinyusb Kconfig default PID range](https://github.com/espressif/esp-usb/blob/master/device/esp_tinyusb/Kconfig) is `0x4000`–`0x4007`. The installed 2.0.0 source and compiled exact candidate descriptor still need explicit inspection as a distribution gate; the physical result corroborates but does not replace it. This permission does **not** constitute USB-IF certification, ownership of Espressif's VID or permission to advertise official USB compliance.

Do **not** change descriptors in Phase 2F packaging work. A descriptor modification creates a new firmware candidate requiring CI and supervised enumeration/no-automatic-HID regression testing.

## Inputs and boundaries

The offline helper `tools/release_package.py` packages the **currently supplied ESP-IDF build output**. It does not download or silently substitute the hardware-accepted artifact, sign binaries, flash a device, create GitHub Releases, or publish a file.

Mandatory inputs, inside `build/`:

| File | Flash offset |
| --- | --- |
| `bootloader/bootloader.bin` | `0x0` |
| `partition_table/partition-table.bin` | `0x8000` |
| `chaon.bin` | `0x10000` |

The archive also includes `flash_args`, `flasher_args.json`, `dependencies.lock` and `manifest.json`. Its manifest records filenames, byte counts, SHA-256 hashes, source commit, SHA-256 of `git archive HEAD`, pinned ESP-IDF/component versions, and fixed image offsets. No private factory backup is included.

The inputs are checked for presence, nonzero length and symlink avoidance. Packaging and verification also cross-check the three expected regions and flash mode/frequency/size against both ESP-IDF `flasher_args.json` and `flash_args`, require an ESP32-S3 chip ID in both image headers, validate segment bounds, XOR checksum and appended SHA-256, and enforce the current 1 MiB app-size bound. The partition table is checked for its standard magic and expected generated length; this is not a complete partition-table semantic audit. A verifier rejects missing/extra/duplicated archive members, changed bytes or metadata, mismatched source commit and inconsistent flash settings. The ZIP has normalized metadata for deterministic assembly of identical inputs.

**Limits:** a hash manifest detects archive corruption and mismatch; it does not prove who built the files, cryptographically sign firmware, guarantee binary reproducibility across independent build hosts, check silicon identity, or certify flash safety. The input filenames and offsets are release policy and are now cross-checked with the generated ESP-IDF flash configuration. These checks do not authenticate the firmware's origin or prove the source-to-binary relationship. Keep the original-board requirement prominent.

## Local dry run

From a clean checkout with ESP-IDF v5.5.3 available, build for `esp32s3` as documented in the README. Then:

```sh
python3 -m unittest discover -s tests -p test_release_package.py
git archive --format=tar HEAD | sha256sum
python3 tools/release_package.py pack \
  --build-dir build --lock dependencies.lock \
  --source-commit "$(git rev-parse HEAD)" \
  --source-tree-sha256 "$(git archive --format=tar HEAD | sha256sum | awk '{print $1}')" \
  --out build/chaon-experimental-candidate.zip
python3 tools/release_package.py verify \
  --archive build/chaon-experimental-candidate.zip \
  --source-commit "$(git rev-parse HEAD)"
sha256sum build/chaon-experimental-candidate.zip
```

The CI ESP-IDF job performs the same *staging and verification only*, retaining the ZIP as a short-lived CI artifact. GitHub Actions' synthetic merge commit on PR builds may differ from the PR head; the manifest records exactly the source checkout actually built. **Do not label a PR merge-ref build as the previously tested physical image.**

## Future public download gate

A public GitHub release can be considered only after explicit owner decisions on license/dependency notices, permitted USB identity, Windows-first host support, acceptable physical fault-injection limitations, manual ROM flashing/recovery directions, artifact retention/provenance and final security review. The documented physical acceptance source remains `ae70e489ccd7ca22e2dcbe466bc5f544940a55f7` with **14 PASS / 0 FAIL / 4 BLOCKED / 1 PARTIAL / 2 N/A / 1 NOT REQUIRED**; new build artifacts are not automatically hardware accepted.

For human-controlled ROM recovery: connect the **original** T-Dongle-S3 with BOOT held to enter ROM, verify chip identity and target flash size, preserve backup/rollback material, and use the manifest's three region offsets only after separately approved verification. Do not enable eFuses, secure boot, flash encryption or full-chip erase. See [Phase 2E.2 hardware evidence](phase2e2-hardware-evidence.md) and [bring-up/recovery history](bringup.md).

**Stopping boundary:** no automated flashing utility, GitHub Release, tag, public upload, or firmware-ID change is authorized by this packaging work.
