# USB storage-first personality | implementation roadmap

**Target:** original LILYGO T-Dongle-S3 (ESP32-S3, 16 MB flash, no PSRAM). This milestone is separate from publicly released v0.1.0-rc1; no OTA or automatic HID execution.

## Mini roadmap

1. **Boot decision (native tests):** Cold-boot check for an ordinary, empty FAT root file named `DUCKY` (case-insensitive FAT short name). Absent → MSC; present + safely removed and disappearance confirmed → HID for **this boot only**; malformed/inaccessible/undeletable marker → never HID. No persisted NVS mode.
2. **Exclusive SD ownership:** Before USB starts, firmware mounts FAT only long enough to check/consume the marker, then unmounts. In MSC personality, only the USB storage backend touches the card. In HID personality, only the existing read-only SD worker touches it. No simultaneous host-USB and firmware filesystem access.
3. **USB descriptors and integration:** Storage-only USB interface by default, HID-only interface after marker consumption. Do not add a keyboard interface to the MSC descriptor. Preserve BOOT-held ROM recovery. Missing media is not an excuse to select HID.
4. **Validation and candidate build:** Native mode-decision boundaries, ESP-IDF v5.5.3 ESP32-S3 build, CI artifact and adversarial review; then supervised original-board Windows testing: default drive, file copy/readback, flush/eject, cold reconnect into inert HID, one-use marker consumption, subsequent reconnect into MSC, missing/invalid media, and ROM recovery.
5. **Release:** Treat successful CI as *flashable candidate*, not physical qualification or public release. Publish separately after exact-build physical testing and owner approval.

## Marker contract

- Root file name is exactly `DUCKY` (no extension); it must be an **empty regular file**. Do not accept directories, renamed variants, or marker content.
- Place file from the host in MSC mode, **safely eject**, physically disconnect, then reconnect. No hot in-session personality switching.
- A successful marker deletion is required before HID enumeration. Any uncertainty stays storage-only or explicitly unavailable; never accidentally expose HID.
- Mode selection never arms or runs a script. Existing PICK → ARM → RUN, source hashing, cancellation and no-fallback rules remain intact.
- FAT integrity remains the host/operator's responsibility: never unplug during a write; no filesystem auto-format; test with a disposable card first.
- BOOT held on insertion remains ROM download, regardless of the marker.

## Review gates

- Inspect generated USB descriptors on Windows to show MSC and HID are mutually exclusive.
- Establish media error behavior if boot FAT mounting or MSC backend startup fails.
- Verify no SD worker starts in MSC mode and no MSC backend exists in HID mode.
- Confirm the exact pinned TinyUSB v2.0.0 driver and config behavior in CI and original-board hardware.
- Verify no further filesystem writes are introduced outside the single marker unlink.
