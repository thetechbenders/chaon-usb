# Original T-Dongle-S3 hardware fact register

**Target:** LILYGO T-Dongle-S3 (original; ESP32-S3). Exclude S3 Plus, Dual S3 and C5 variants.

## Reference-confirmed (vendor documentation, not physical measurements)

| Resource | Vendor value | Status |
| --- | --- | --- |
| SoC | ESP32-S3 | Vendor |
| SPI flash | 16 MB QSPI | Vendor |
| SRAM | 512 KB internal | Vendor |
| PSRAM | **None** | Vendor |
| LCD | 160 x 80 ST7735 SPI, 0.96 in | Vendor |
| BOOT button | GPIO0, active low | Vendor; also strapping/download mode |
| APA102 RGB data / clock | GPIO40 / GPIO39 | Vendor |
| SDMMC D0, D1, D2, D3 | GPIO14, 17, 21, 18 | Vendor |
| SDMMC CLK / CMD | GPIO12 / GPIO16 | Vendor |
| QWIIC serial TX / RX | GPIO43 / GPIO44 | Vendor |
| USB input | 4.8–5.5 V | Vendor |
| Advertised USB max current | 800 mA | Vendor; not measured |

Original-board LCD GPIOs from LILYGO's original-board `examples/lcd/lcd.ino`: MOSI 3, CLK 5, CS 4, D/C 2, RST 1, backlight 38. These are **software-example pin references**, not physically continuity-checked. GPIO0 pressed during power-on can enter ROM download mode. Flash/recovery documentation must preserve that distinction.

The original S3 lacks PSRAM despite the **S3 Plus** marketing listing advertising 8 MB. Firmware defaults should not enable PSRAM.

## Evidence

- Vendor quick start: https://github.com/Xinyuan-LilyGO/T-Dongle-S3/blob/main/docs/en/t-dongle-s3/REAMDE.MD
- Vendor schematic (original/QWIIC): https://github.com/Xinyuan-LilyGO/T-Dongle-S3/blob/main/schematic/T-Dongle-S3-QWIIC.pdf
- Vendor LCD example: https://github.com/Xinyuan-LilyGO/T-Dongle-S3/blob/main/examples/lcd/lcd.ino
- Vendor HID example: https://github.com/Xinyuan-LilyGO/T-Dongle-S3/blob/main/examples/usb_hid_keyboard/usb_hid_keyboard.ino

## To verify on Dan's physical board

- [ ] Product silk, MCU/module, actual flash and PSRAM detection
- [ ] USB-A plug directly wired to ESP32-S3 USB OTG and reliable host enumeration
- [ ] GPIO0 debounce, boot strap and recovery
- [ ] APA102 color ordering and status indications
- [ ] LCD actual panel orientation, offsets, reset/backlight behavior
- [ ] SD card socket presence, signal routing, insertion/removal
- [ ] USB current and operation on a USB 2.0 port
- [ ] Whether QWIIC is physically present and signal levels
- [ ] Host keyboard reliability (Windows/Linux/macOS, with authorized test setups)

No unverified GPIO assignment is a guaranteed ChaoN production pinout.

