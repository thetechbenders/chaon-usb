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
- [ ] Verify new APA102 color/blink behavior on hardware (original board has no PSRAM per vendor)

## Phase 2: display and script storage

- [ ] Physically verify native 160×80 ST7735 status-display candidate (implemented and CI green in stacked PR #6)
- [ ] SDMMC mount/read-only, card absence and hot-removal handling
- [ ] UI: selected script, preview, armed/run/fault state; accidental keypress mitigation
- [ ] Script directory rules, bounded reading, versioned format and negative tests
- [ ] Tests for parser fuzz targets and edge cases

## Phase 3: product hardening

- [ ] Host layout selection and user documentation
- [ ] Decide USB IDs and release packaging (source repository public under AGPL-3.0-only; no production firmware release)
- [ ] Threat model script ingestion and firmware update pathway
- [ ] End-to-end on-device test matrix, reproducible release artifacts
- [ ] No release without owner approval

Deferred, not promised: Wi-Fi script management, OTA, signed scripts, remote control, BLE HID. Keep the initial path deliberately simple and auditable.

