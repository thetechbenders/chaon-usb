# T-Dongle-S3 Plus: M1 inert board bring-up (unverified hardware)

**Status:** software port in draft; **not flashed, physically tested, or approved for USB HID execution**. The original T-Dongle-S3 target remains the full ChaoN product; S3 Plus intentionally has no HID initialization, ARM/RUN handling, or script execution in this milestone.

## Rationale and sources

The S3 Plus uses ESP32-S3, 16 MB QSPI flash and 8 MB OPI PSRAM. The published Plus pin table lists BOOT=GPIO0, APA102 data/clock=40/39 and SDMMC CLK/CMD/D0=12/16/14. LILYGO lists a 160×80 ST7735 SPI LCD. The **Plus-capable official factory-screen example** explicitly defines LCD MOSI=3, CLK=5, CS=4, DC=2, RST=1, BL=38 with backlight **active low**. It is explicitly intended for both the original S3 and S3 Plus, so the current ChaoN LCD GPIO mapping is **vendor-software-confirmed for both targets**, not a speculative extrapolation. Board-revision-specific physical behavior remains untested.

- https://wiki.lilygo.cc/products/t-dongle-series/t-dongle-s3-plus/
- https://wiki.lilygo.cc/products/t-dongle-series/t-dongle-s3-plus/quick-start.html
- https://github.com/Xinyuan-LilyGO/T-Dongle-S3/blob/main/schematic/T-Dongle-S3-Plus.pdf
- https://github.com/Xinyuan-LilyGO/T-Dongle-S3/blob/main/examples/factory_screen/factory_screen.ino (shared LCD pin assignments; active-low backlight; Plus-specific features conditional on board definition)
- https://github.com/Xinyuan-LilyGO/T-Dongle-S3/blob/main/docs/en/t-dongle-s3-plus/REAMDE.MD (Plus pin table)
- Original LCD example: https://github.com/Xinyuan-LilyGO/T-Dongle-S3/blob/main/examples/lcd/lcd.ino

Plus-only PDM microphone (GPIO8/9), IR emitter (GPIO7) and optional I2C ATECC508A (GPIO10/11) are **not initialized or used**. Do not configure or lock the security chip. PSRAM remains unused, despite its presence: all original static/DMA constraints stay intact until measured otherwise.

## Reproducible builds

Use a clean build directory when switching board profiles. On ESP-IDF v5.5.3:

```sh
# Original S3, unchanged default behavior
idf.py -B build-original -DCHAON_BOARD=tdongle_s3 build

# S3 Plus: inert hardware-diagnostic build only
idf.py -B build-plus -DCHAON_BOARD=tdongle_s3_plus build
```

The default profile when `CHAON_BOARD` is omitted is `tdongle_s3`. The CI Plus job is compile-only and **does not upload a Plus flash package**. Do not reuse the original board's accepted binary, `sdkconfig` provenance, memory configuration, or flash evidence as Plus hardware acceptance. These builds share the same 16 MB flash default, with no PSRAM allocation dependency.

The Plus image sets `CHAON_BOARD_INERT_BRINGUP=1`, initializes the GPIO/APA102, attempts LCD init and starts the **read-only** SD monitor task, then idles. It does **not call `tinyusb_driver_install()`**, enter the controller/VM event loop, or send HID reports. Its compiled binary is not released and should not be described as having full ChaoN functionality.

## Before any separately authorized physical test

1. Inspect actual hardware variant, Plus revision, power, native USB/ROM path, flash size and PSRAM identity. Verify actual LCD behavior and active-low backlight on that unit using the **documented vendor pins**, rather than treating those pins as unknown.
2. Secure a *Plus-specific* recovery/readback backup and stable ROM recovery procedure. Do not assume the original S3 recovery artifact is compatible.
3. Verify the exact source SHA and Plus build output, flash configuration and no-HID diagnostic gate.
4. With separate explicit permission, run supervised display, LED, BOOT and read-only SD tests. Confirm no USB keyboard enumeration or unsolicited typing.
5. Add hardware observations and any corrected pin assignments to a Plus-specific acceptance record before proposing a separately gated full HID/selection build.

**Stop condition for M1:** draft PR, original plus inert target CI pass and documented unresolved physical acceptance. No merge, flash, release, or activation of HID on Plus.
