<div align="center">
  <img src="assets/chaon-logo.svg" alt="ChaoN: d20 and charcoal/neon-green wordmark" width="100%" />
</div>

# ChaoN-USB

**The Chaotic Neutral HID.** Experimental, screen-equipped USB HID instrument written in C for the **original LILYGO T-Dongle-S3** (ESP32-S3).

> **First physical bring-up candidate.** The validated ESP-IDF v5.5.3 CI artifact has been flashed to Dan's original T-Dongle-S3 after two matching full factory backups. Windows HID enumeration, inert insertion, BOOT arming/expiry and the exact harmless text test have passed on that host. See the [bring-up evidence](docs/bringup.md) for completed gates and remaining limits; LCD and microSD are unimplemented.

## What exists in this branch

- Native ESP-IDF / FreeRTOS firmware structure, ESP32-S3 USB OTG and TinyUSB keyboard device configuration.
- An inert-on-plug-in USB HID demo. Two deliberate, separate **press-and-release** actions on the BOOT button (GPIO0) arm then execute a fixed harmless text-only test. Arming expires after 30 seconds; USB unplug/restart disarms.
- A small product-owned, bounded C script parser and executor for `TYPE`, `DELAY`, `ENTER` and `TAB`. No DuckyScript-compatibility claim, scripting from SD, network execution, or host command execution.
- A US keyboard-layout HID usage mapper; no arbitrary keyboard-layout claims.
- APA102 indicator output, host-only parser/execution tests, initial GitHub CI, diagrams and bring-up documents.

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

CI runs host tests and an ESP32-S3 build on PRs and manual dispatch. The branch is intentionally unmerged pending the software build and physical acceptance gates.

## Documentation

- [Project architecture](docs/architecture.md)
- [Hardware fact register](docs/hardware.md)
- [Script language v0](docs/script-language.md)
- [Security / safe use](docs/security.md)
- [Board bring-up and recovery](docs/bringup.md)
- [Milestones and acceptance](docs/roadmap.md)
- [Branding / asset source](docs/branding.md)

## License

**Undecided.** The repository is private and no open-source license has been selected. Do not describe this code as licensed or released until the owners make that decision.

Copyright © Techbenders contributors. All rights reserved pending explicit licensing.

