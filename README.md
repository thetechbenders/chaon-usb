<div align="center">
  <img src="assets/chaon-logo.svg" alt="ChaoN: d20 and charcoal/neon-green wordmark" width="100%" />
</div>

# ChaoN-USB

**The Chaotic Neutral HID.** Experimental, screen-equipped USB HID instrument written in C for the **original LILYGO T-Dongle-S3** (ESP32-S3).

> **Development status (2026-10-23):** The inert, physically BOOT-gated HID controller, read-only SD probe, FAT32 root preview and single-button browser passed their documented original-board/Windows/FAT32 smoke tests through Phase 2B. Stacked Phase 2C/2D draft work adds bounded read-only `.CHN` inspection and one-slot inert program staging, and it **passed combined original-board physical acceptance (33 PASS, 0 FAIL, 1 NOT TESTED, 2 NOT APPLICABLE) on 2026-10-23**, but remains unmerged and cannot execute SD content. The physical in-flight SD-read supersession race was not established; native adversarial tests cover it. The embedded fixed demo remains the only VM/HID program. See [inspection](docs/script-inspection.md), [staging](docs/program-staging.md) and [future execution-authority design](docs/execution-authority.md). No production firmware has been released.

## Current implementation

- Native ESP-IDF / FreeRTOS firmware structure, ESP32-S3 USB OTG and TinyUSB keyboard device configuration.
- An inert-on-plug-in USB HID demo. Two deliberate, separate **press-and-release** actions on the BOOT button (GPIO0) arm then execute a fixed harmless text-only test. Arming expires after 30 seconds; USB unplug/restart disarms.
- A small product-owned, bounded C script parser and executor for `TYPE`, `DELAY`, `ENTER` and `TAB`. No DuckyScript-compatibility claim, scripting from SD, network execution, or host command execution.
- A US keyboard-layout HID usage mapper; no arbitrary keyboard-layout claims.
- State-driven APA102 colors (ChaoN green READY, yellow flashing ARMED, red CANCELLED), native ST7735 160×80 status display, portable renderer tests, GitHub CI and hardware validation documentation.
- **Hardware smoke-tested storage features (PRs #9/#12/#13):** read-only SDMMC sector observation, bounded FAT12/16/32 root-directory metadata listing, and BOOT-operated, display-only navigation/PICK selection. Physical filesystem coverage is FAT32 only; this is neither a full file manager nor an SD-script interpreter.
- **Unmerged, physically smoke-tested Phase 2C/2D draft:** bounded FAT32 root `.CHN` reading, Script v0 validation, summary-only LCD diagnostics and one fixed-capacity staged program. Staging has no execution authority and no path to HID.

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

CI runs host tests and an ESP32-S3 build on configured PR targets and manual dispatch. The BOOT controller, FAT32 root preview and display-only browser have passed the documented hardware smoke tests on the original board and Windows host; this is **not** production qualification.

## Documentation

- [Project architecture](docs/architecture.md)
- [Hardware fact register](docs/hardware.md)
- [Script language v0](docs/script-language.md)
- [Security / safe use](docs/security.md)
- [Board bring-up and recovery](docs/bringup.md)
- [Milestones and acceptance](docs/roadmap.md)
- [Branding / asset source](docs/branding.md)
- [LCD / LED state colors and physical test plan](docs/status-ui.md)
- [Read-only SD probe and LCD diagnostics](docs/sd-storage.md)
- [Bounded FAT root preview and 2026-10-20 hardware results](docs/fat-root-preview.md)
- [Single-button browser contract and Phase 2B validation](docs/browser-controls.md)
- [Bounded script inspection](docs/script-inspection.md)
- [Bounded program staging ownership and invalidation](docs/program-staging.md)
- [Future removable-media execution-authority design](docs/execution-authority.md)

## License

ChaoN-USB's original source code, documentation, and project artwork are licensed under the **GNU Affero General Public License v3.0 only** ([AGPL-3.0-only](LICENSE)). Copyright © 2026 Techbenders contributors.

The repository is public and AGPL-licensed, but **no production firmware has been released**. Third-party dependencies (including ESP-IDF and TinyUSB) retain their respective licenses; ChaoN's AGPL does not replace those separate notices.

