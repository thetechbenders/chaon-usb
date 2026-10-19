# Roadmap and release gates

## Phase 0: foundations (this PR)

- [x] Decide original S3 / native C / ESP-IDF architecture and ChaoN identity
- [x] Collect vendor-confirmed original-board pin/memory facts
- [x] Prototype bounded parser, US HID mapper, execution callback layer, HID USB application
- [x] Prepare brand SVG and engineering docs
- [ ] Host and ESP-IDF builds green on exact PR head
- [ ] Review prototype HID timing, button state transitions, and report-release contract

## Phase 1: physical proof

- [ ] Receive board, check hardware and USB-A routing
- [ ] USB enum: HID keyboard, no unsolicited typing
- [ ] Verify two-step arming, 30-second expiry, cancellation and key release
- [ ] Validate ESP-IDF USB logs, cold boot, ROM download and recovery
- [ ] Verify APA102 electrical/color behavior and flash/PSRAM reports

## Phase 2: display and script storage

- [ ] Independent 160×80 ST7735 driver and UI
- [ ] SDMMC mount/read-only, card absence and hot-removal handling
- [ ] UI: selected script, preview, armed/run/fault state; accidental keypress mitigation
- [ ] Script directory rules, bounded reading, versioned format and negative tests
- [ ] Tests for parser fuzz targets and edge cases

## Phase 3: product hardening

- [ ] Host layout selection and user documentation
- [ ] Decide USB IDs, licensing/publication, release packaging
- [ ] Threat model script ingestion and firmware update pathway
- [ ] End-to-end on-device test matrix, reproducible release artifacts
- [ ] No release without owner approval

Deferred, not promised: Wi-Fi script management, OTA, signed scripts, remote control, BLE HID. Keep the initial path deliberately simple and auditable.

