<div align="center">
  <img src="assets/chaon-logo.svg" alt="ChaoN: d20 and charcoal/neon-green wordmark" width="100%" />
</div>

# ChaoN-USB

**The Chaotic Neutral HID.** Experimental, screen-equipped USB HID instrument written in C for the **original LILYGO T-Dongle-S3** (ESP32-S3).

> **Development status:** Firmware `4d8d9e0` passed ESP-IDF v5.5.3 CI, verified flashing, and the agreed LCD/LED/HID smoke tests on an original T-Dongle-S3 and Windows host. The factory backup and verified `323c180` rollback package remain preserved. PRs #1 and #6 have since merged into `main`. The latest BOOT controller refactor (`fb85ce8`) passed host tests and ESP32-S3 CI but **has not been physically flashed or retested**. See [bring-up evidence and limits](docs/bringup.md) and [LCD/LED results](docs/status-ui.md). microSD scripting is not implemented and no production firmware release has been published.

## Current implementation

- Native ESP-IDF / FreeRTOS firmware structure, ESP32-S3 USB OTG and TinyUSB keyboard device configuration.
- An inert-on-plug-in USB HID demo. Two deliberate, separate **press-and-release** actions on the BOOT button (GPIO0) arm then execute a fixed harmless text-only test. Arming expires after 30 seconds; USB unplug/restart disarms.
- A small product-owned, bounded C script parser and executor for `TYPE`, `DELAY`, `ENTER` and `TAB`. No DuckyScript-compatibility claim, scripting from SD, network execution, or host command execution.
- A US keyboard-layout HID usage mapper; no arbitrary keyboard-layout claims.
- State-driven APA102 colors (ChaoN green READY, yellow flashing ARMED, red CANCELLED), native ST7735 160×80 status-display candidate, portable renderer tests, GitHub CI and hardware validation documentation.

**The demo writes into whichever window currently has keyboard focus. Use only on a computer you own or are explicitly authorized to test.** Plugging the device in will not intentionally type anything until physical confirmation. The button is also the boot-strap/download pin, so never hold it during power-on when you want normal boot.

## Scope

- **Hardware:** original T-Dongle-S3, not S3 Plus, Dual S3, or T-Dongle-C5.
- **Language:** C11, not Arduino, C++, or cloned firmware.
- **SDK:** ESP-IDF v5.5.3 initially; the project uses Espressif's supported TinyUSB component for USB transport rather than attempting to implement USB framing from scratch.
- **Separate project:** not Dragon-family firmware and not a BandiC5 fork.

The vendor's original-board documentation specifies **16 MB flash, 512 KB on-chip SRAM, and no PSRAM**. A listing for the S3 Plus must not be used as the original board's BOM.

## Development

Install ESP-IDF **v5.5.3** and an ESP32-S3 toolchain, then:

    idf.py set-target esp32s3
    idf.py build

Run portable host tests without ESP-IDF:

    make test

CI runs host tests and an ESP32-S3 build on PRs and manual dispatch. The latest `main` firmware still requires a physical BOOT-controller regression test before a production release.

## Documentation

- [Project architecture](docs/architecture.md)
- [Hardware fact register](docs/hardware.md)
- [Script language v0](docs/script-language.md)
- [Security / safe use](docs/security.md)
- [Board bring-up and recovery](docs/bringup.md)
- [Milestones and acceptance](docs/roadmap.md)
- [Branding / asset source](docs/branding.md)
- [LCD / LED state colors and physical test plan](docs/status-ui.md)

## License

ChaoN-USB's original source code, documentation, and project artwork are licensed under the **GNU Affero General Public License v3.0 only** ([AGPL-3.0-only](LICENSE)). Copyright © 2026 Techbenders contributors.

The repository is public and AGPL-licensed, but **no production firmware has been released**. Third-party dependencies (including ESP-IDF and TinyUSB) retain their respective licenses; ChaoN's AGPL does not replace those separate notices.

