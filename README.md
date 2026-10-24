<div align="center">
  <img src="assets/chaon-logo.svg" alt="ChaoN: d20 and charcoal/neon-green wordmark" width="100%" />
</div>

# ChaoN-USB

**The Chaotic Neutral HID.** Experimental USB HID keyboard firmware written in C and ESP-IDF for the **original LILYGO T-Dongle-S3** (ESP32-S3).

> **Project status (2026-10-24):** Phase 2E.1 and Phase 2E.2 are merged into `main`. The exact Phase 2E.2 firmware source `ae70e489ccd7ca22e2dcbe466bc5f544940a55f7` is **hardware accepted with documented physical limitations**, not production-qualified or released. On the original board, supervised physical validation recorded **14 PASS, 0 FAIL, 4 BLOCKED, 1 PARTIAL, 2 NOT APPLICABLE and 1 NOT REQUIRED**. See [hardware evidence](docs/phase2e2-hardware-evidence.md) and [validation plan](docs/phase2e2-hardware-validation.md). Later documentation and merge commits are not different hardware-tested images.

## What works

- ESP-IDF v5.5.3 / FreeRTOS, native ESP32-S3 USB OTG, and TinyUSB HID keyboard.
- **Inert insertion:** USB connection, reboot, and SD discovery do not intentionally type anything.
- **Fixed benign demo:** with no SD selection, two distinct BOOT press/release gestures ARM then RUN the embedded text. The arm window expires after 30 seconds.
- **Read-only microSD library:** bounded FAT32 root `.CHN` file reading, DOS 8.3 filename selection, strict script validation, LCD inspection and one immutable selected program. FAT12/16 root metadata preview exists, but **script contents are only supported from FAT32**.
- **Selected SD execution:** BROWSE → PICK (asynchronous validation) → READY → ARM → RUN. The selected file's identity is shown in READY and ARMED. RUN requires a one-use authorization, a fresh independent SD re-read and SHA-256 match, and current USB/source/display eligibility. A rejected selected run does **not** fall back to the demo.
- **Cancellation and recovery:** BOOT or USB loss cancels active execution; the VM attempts to release all keyboard keys. BOOT held during power-on enters the ESP32-S3 ROM download loader.
- APA102 status indicator, ST7735 160×80 LCD, bounded C Script v0 parser and execution engine, native tests and ESP32-S3 CI.

**Safety:** HID output goes to whichever application has keyboard focus. Use ChaoN only on systems you own or are explicitly authorized to test, with a harmless test script and text editor focused. If USB remains mounted, a failed final HID release is treated as a fault, not proof of a clean cancellation; USB loss instead returns the controller to its disconnected state.

## Supported hardware and software

- **Board:** original LILYGO T-Dongle-S3, **not** T-Dongle-S3 Plus, Dual S3 or C5.
- **Chip / memory:** ESP32-S3, 16 MB flash, 512 KB internal SRAM, **no PSRAM**.
- **Control:** BOOT / GPIO0, also the ROM-download strap.
- **Display / storage:** ST7735 160×80, APA102 LED and microSD.
- **Language:** C11; no Arduino firmware, Dragon-core dependency, or DuckyScript compatibility.
- **Keyboard mapping:** US HID layout only. On non-US host layouts, visible characters may differ.
- **Connectivity:** USB only for execution. No Wi-Fi script upload, remote HID execution or OTA feature is claimed.

## Quick start for developers

Install **ESP-IDF v5.5.3** with an ESP32-S3 toolchain, then run:

```sh
idf.py set-target esp32s3
idf.py build
make test
```

The native test suite uses the repository Makefile. Building firmware does **not** flash it or establish hardware acceptance. Consult [bring-up and recovery](docs/bringup.md) and the [hardware evidence](docs/phase2e2-hardware-evidence.md) before any authorized hardware work. Never assume that a newer artifact is byte-equivalent to the tested image.

## Script format and interaction

ChaoN Script v0 supports `TYPE "text"`, `DELAY N`, `TAB`, and `ENTER`, with fixed bounds and strict whole-program rejection on invalid input. See [script language](docs/script-language.md).

Select a supported FAT32-root `.CHN` file through the physical browser; PICK validates and owns the selection but does **not** arm or run it. A separate short press/release arms, and another deliberately executes it. Each run requires fresh source verification. An invalidated selection must be re-established deliberately. See [selection and authorization contract](docs/phase2e2-selection-contract.md).

## Verification and limitations

The hardware-accepted candidate was independently flashed and read back on the original board, with correct USB identity (`303A:4004`, serial `CHN-S3-DEV`), display/LED response, selected execution, cancellation, reconnect and ROM recovery. **Four specialized physical fault-injection checks remain BLOCKED**: same-identity in-place source modification, BOOT during the brief verification window, forced verification timeout and injected live SD read failure. The dedicated last-moment no-demo-fallback physical check is PARTIAL, with native regression evidence. Powered SD removal during verification is **not a valid manual acceptance test** on this board because the slot is inaccessible when inserted in USB.

The tested source is `ae70e489ccd7ca22e2dcbe466bc5f544940a55f7`; artifact `12573854980`, ZIP SHA-256 `abe10ebffa2d2949de1fb860b41f701097f930e65c86d45b7eb0e946183d69c7`. No public production release or tag is implied. [Release policy decisions](https://github.com/thetechbenders/chaon-usb/issues/5) remain open, including USB identifier policy, packaging and provenance.

## Documentation

- [Architecture and trust boundaries](docs/architecture.md)
- [Hardware facts](docs/hardware.md)
- [Script language v0](docs/script-language.md)
- [Security and safe use](docs/security.md)
- [Historical Phase 2B browser validation](docs/browser-controls.md)
- [Phase 2E.2 selection/authorization contract](docs/phase2e2-selection-contract.md)
- [Execution-authority design history](docs/execution-authority.md)
- [Hardware validation plan](docs/phase2e2-hardware-validation.md) and [physical evidence](docs/phase2e2-hardware-evidence.md)
- [Board bring-up and recovery](docs/bringup.md)
- [Roadmap and release gates](docs/roadmap.md)
- [Contributing](CONTRIBUTING.md)

Older phase-specific records document the state at their historical test dates. They must not be interpreted as current functionality.

## License

ChaoN-USB's original source, documentation and project artwork are licensed under the **GNU Affero General Public License v3.0 only** ([AGPL-3.0-only](LICENSE)). Copyright © 2026 Techbenders contributors. Third-party dependencies retain their respective licenses.
