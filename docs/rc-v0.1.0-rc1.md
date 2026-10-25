# ChaoN-USB v0.1.0-rc1 | Experimental release candidate

**Status:** release-candidate qualification only. This is not a stable release, a production-certified device, or permission to use scripts on third-party computers.

## Scope and evidence

- **Hardware:** only the original LILYGO T-Dongle-S3, ESP32-S3, 16 MB flash, no PSRAM. Never flash T-Dongle-S3 Plus or unrelated ESP32-S3 boards using these instructions.
- **Software:** native USB HID keyboard, ST7735 LCD/APA102 indicators, read-only FAT32-root DOS 8.3 `.CHN` scripts, deliberate PICK / ARM / RUN, and one-use freshness checks. No Wi-Fi, OTA, BLE, automatic remote execution, signed firmware boot or secure-boot guarantee.
- **Host coverage:** Windows physically observed on one test setup. Other operating systems, USB hosts and non-US keyboard layouts are not qualified. The device maps US HID usages.
- **USB identity:** accepted hardware enumerated ESPressif default `303A:4004`; application uses TinyUSB default device descriptor, Techbenders / ChaoN USB HID strings, and fixed serial `CHN-S3-DEV`. Two units on the same host may collide because the serial is not unique. No USB-IF certification or ownership of Espressif's VID is claimed. Obtain component-source confirmation of the exact pinned default before broad public promotion.
- **Local scripts are untrusted input** even if they parse. They send keystrokes to the currently focused application. Only use benign test fixtures on owned or explicitly authorized hosts.

### Do not conflate physical validation with an RC rebuild

Phase 2E.2 hardware acceptance belongs to **source `ae70e489ccd7ca22e2dcbe466bc5f544940a55f7` and its specific artifact**, not automatically to a newly generated RC binary. That session recorded **14 PASS / 0 FAIL / 4 BLOCKED / 1 PARTIAL / 2 NOT APPLICABLE / 1 NOT REQUIRED**. Four physical specialized injections remain unperformed (in-place same-identity SD mutation, BOOT during the brief verification window, forced verification timeout, injected SD read fault); the no-demo-fallback physical observation is partial. Powered microSD removal is physically impossible as a manual test while the original board is USB-inserted and is excluded.

An RC branch passing CI establishes *build and packaging integrity*, not a new physical hardware acceptance. The exact RC binary needs supervised device verification before the project can claim it was physically RC-qualified.

## Candidate provenance and integrity

RC candidate artifacts are built by `.github/workflows/ci.yml` on a **stable `rc/*` branch push**, not on ephemeral PR merge refs. The Actions artifact (30-day retention) contains:

- `chaon-experimental-candidate.zip` with `chaon.bin`, bootloader, partition table, generated flash arguments, dependency lock and `manifest.json`.
- `rc-provenance.json` with the run ID, attempt, full source commit and SHA-256 of the candidate ZIP.
- Raw build inputs and ELF for traceability.

The packager checks actual ESP-IDF image format (S3 chip identity, segment checksum and hash), flash offsets, settings and file checksums. The provenance sidecar points to a GitHub Actions run and **is not a cryptographic signature**. A recipient should retrieve artifacts only from the official Techbenders repository and compare hashes to an independently trusted communication or official release record. The tool cannot prove source-to-binary reproducibility or authenticity of a repackaged archive.

The `rc/*` branch must remain unchanged once the candidate is selected; if it moves, a **new candidate** with a new digest and validation record must be used. Do not distribute PR-merge-ref artifacts as an RC.

## Conservative Windows manual flash/recovery procedure

**No unattended installer exists. The following is a supervised procedure, not a command to flash anything now.** Before considering a write:

1. Confirm the physical silkscreen, original-board identity, 16 MB flash, and your authorization to modify that exact board. Preserve personal data and back up the original flash when feasible. Keep backup images private, with their SHA-256 hashes. Confirm USB port identity carefully; unplug other serial devices if practical.
2. Download the RC candidate from the official repository's exact identified CI run. Preserve the original ZIP; verify its digest against `rc-provenance.json` from that same run and, independently, the run's GitHub identity. Note that two files from one compromised source are not an authenticity guarantee.
3. Extract `chaon-experimental-candidate.zip` in a new directory. Run `python tools/release_package.py verify --archive path/to/chaon-experimental-candidate.zip --source-commit FULL_SHA` from the corresponding repository revision. Never trust scripts or executables embedded in arbitrary third-party firmware ZIPs.
4. With the device unplugged, hold **BOOT while inserting USB** to enter ESP32-S3 ROM download mode. Identify the new COM port (not another attached device). Use `esptool --chip esp32s3 --port COMx flash-id` or another read-only identity check and verify 16 MB flash before proceeding. Do not manipulate security eFuses, erase the full chip, enable flash encryption or disable ROM recovery.
5. If the device is wrong, the expected ROM port is missing, flash size differs, the image verification fails, or backup/recovery is unavailable, **STOP**; do not flash.
6. Once separately authorized for a physical RC test, flash **only** these verified candidate images at `0x0` (bootloader), `0x8000` (partition table), and `0x10000` (application), with **DIO / 80 MHz / 16 MB** settings. Consult the packaged `flash_args` / `flasher_args.json` and use the esptool version compatible with the pinned toolchain. Do not substitute any other filename/offset or manually erase the entire device. Verify-flash independently afterward, then reconnect with BOOT released.
7. With an owned Windows test host and an innocuous text editor focused, confirm inert insertion, correct HID identity, displayed selected script, separate ARM/RUN, exact harmless text, cancellation/all-keys release, and ROM recovery. Record firmware BIN digests, port/chip identity, observations, and any deviations in an RC-specific evidence ledger. **Do not mark RC physically accepted without these observations.**

If the candidate misbehaves, recover using BOOT-held ROM mode and a verified earlier accepted image or a privately held board-specific full-flash backup. Never publish another operator's factory backup.

## Legal, dependencies, and distribution

ChaoN's original code, documentation and artwork are provided under **AGPL-3.0-only** in [LICENSE](../LICENSE). Third-party dependencies retain their own license terms; an RC promotion requires verifying and including the actual notices and distribution obligations for ESP-IDF, `esp_tinyusb`, TinyUSB and all redistributed components. This page does not pretend that the dependency audit is complete.

## RC promotion checklist

- [ ] Candidate comes from the exact stable `rc/v0.1.0-rc1` commit and successful native, UBSan, ESP-IDF build and package validation jobs.
- [ ] Verify CI artifact ZIP digest, provenance sidecar and expected SHA/manifests.
- [ ] Confirm the pinned USB default descriptor behavior and document fixed-serial limitations.
- [ ] Verify third-party source/artwork rights, dependency notices and redistribution requirements.
- [ ] Complete RC-specific hardware smoke test against exact candidate or clearly label it **UNTESTED RC**.
- [ ] Record explicit acceptance of unresolved physical fault-injection limitations for this experimental audience.
- [ ] Independent source/artifact review and explicit sign-off on publishable release notes.
- [ ] If publishing a public GitHub Release, ensure it is **marked pre-release**, uses the frozen commit and the *same exact ZIP*, includes the notices and warnings above and is not confused with a stable release.

Keep [issue #5](https://github.com/thetechbenders/chaon-usb/issues/5) open until the final release decision is separately recorded. An RC may be staged without claiming production readiness.
