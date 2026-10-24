# Safety and authorization

ChaoN is an experimental USB HID engineering device for **owned or explicitly authorized** computers. It sends keystrokes to the focused host application and cannot determine whether that application is safe.

## Current deliberate execution model (Phase 2E.2)

- USB insertion, reconnect and passive SD discovery must not type.
- With no SD selection, the embedded benign demo requires separate BOOT ARM and RUN gestures.
- BROWSE and PICK are non-executing actions. PICK requires a valid bounded `.CHN` inspection before the application owns a selected program.
- The selected program's filename remains visible in READY and ARMED. ARM expires after 30 seconds; a second distinct short BOOT gesture requests RUN.
- Selected RUN requires one-use authorization bound to selection generation, USB epoch and source SHA-256, an independent live SD re-read/hash PASS within the verification window, and an immutable execution-owned copy.
- Missing or changed source, SD errors, invalid selection, LCD unavailability, USB loss, cancellation, stale authorization or verification timeout deny selected execution. Failed selected execution must not silently trigger the fixed demo.
- BOOT or USB loss during execution requests cancellation. All VM exits attempt an all-keys HID release; failed release is a FAULT, not a successful cancellation.
- BOOT held at power-on still enters the ESP32-S3 ROM loader.

Script v0 is deliberately small: bounded `TYPE`, `DELAY`, `TAB` and `ENTER`, US layout only. This is **not** DuckyScript, host shell scripting or unattended network execution. FAT32 script storage and SD reads are read-only.

## Limitations and trust model

- This is not a tamper-resistant or production-qualified security product.
- Local SD files are operator-provided input, **not inherently trusted** because they parsed successfully; physical authorization is still required.
- Host focus, host keyboard layout and post-disconnect key state are ultimately host behaviors. Use a harmless fixture and focused text editor.
- Firmware/USB identity policy, release signing and provenance remain open decisions. No OTA or remote-control capability is claimed.
- Physical acceptance was **14 PASS / 0 FAIL / 4 BLOCKED / 1 PARTIAL**. Blocked cases involve specialized mutation/timing/read-error injection. Powered SD removal during verification is not a valid manual original-board test.

See [selection/authority contract](phase2e2-selection-contract.md), [hardware evidence](phase2e2-hardware-evidence.md), and [release issue #5](https://github.com/thetechbenders/chaon-usb/issues/5).
