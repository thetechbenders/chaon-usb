# Roadmap and release gates

## Phase 0: foundations (this PR)

- [x] Decide original S3 / native C / ESP-IDF architecture and ChaoN identity
- [x] Collect vendor-confirmed original-board pin/memory facts
- [x] Prototype bounded parser, US HID mapper, execution callback layer, HID USB application
- [x] Prepare brand SVG and engineering docs
- [x] Host and ESP-IDF builds green on original firmware head `323c180`
- [ ] Review prototype HID timing, button state transitions, and report-release contract

## Phase 1: physical proof

- [x] Receive original board, verify ROM identity, flash size and USB HID routing
- [x] Windows USB enumeration: keyboard, no unsolicited typing
- [x] First Windows smoke test: arm, expire, execute fixed text and cancel (operator-verified)
- [x] Windows ROM download and recovery validated; USB suspend and other OSes pending
- [x] APA102 state indication and normal BOOT control regression on original board / Windows (2026-10-19); electrical timing and endurance remain untested

## Phase 2: display and script storage

- [x] Physically verify native 160×80 ST7735 status display on original board
- [x] Read-only SDMMC sector probe, cold empty-slot/restoration and FAT32 root 8.3 preview (Phase 2A, merged PRs #9/#11/#12)
- [x] Powered SD hot-removal is NOT APPLICABLE: card slot inaccessible while dongle is plugged into USB
- [x] Phase 2B: one-button root filename browsing and display-only PICK marker; CI and applicable original-board/Windows/FAT32 smoke tests passed 2026-10-20 (PR #13). Still no SD file reads or execution. See [browser controls](browser-controls.md).
- [ ] Phase 2C: bounded read-only script inspection, independent of HID
- [ ] Phase 2D: separately threat-modeled/authorized execution of validated selected scripts
- [ ] Script directory rules, bounded reading, versioned format and negative tests
- [ ] Tests for parser fuzz targets and edge cases

## Phase 3: product hardening

- [ ] Host layout selection and user documentation
- [ ] Decide USB IDs and release packaging (source repository public under AGPL-3.0-only; no production firmware release)
- [ ] Threat model script ingestion and firmware update pathway
- [ ] End-to-end on-device test matrix, reproducible release artifacts
- [ ] No release without owner approval

Deferred, not promised: Wi-Fi script management, OTA, signed scripts, remote control, BLE HID. Keep the initial path deliberately simple and auditable.

