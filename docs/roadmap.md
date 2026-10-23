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
- [ ] Phase 2C: bounded read-only FAT32 .CHN inspection backend and summary-only SD-task/LCD path implemented in draft PR #14; native/target CI and the earlier combined `e34e621` Phase 2D candidate's original-board acceptance provide evidence for it. A Phase 2C-only binary was not separately flashed. Review and merge remain gates. No SD execution. See [inspection design](script-inspection.md).
- [ ] Phase 2D: bounded one-slot program staging implemented on the Phase 2C draft; the earlier `e34e621` candidate passed observable original-board acceptance on 2026-10-23, while internal staged bytes and in-flight physical supersession remained uninstrumented. The later sector-scratch clearing source change has not been flashed or physically retested. Review and merge remain gates. Staging grants no HID authority.
- [ ] Future separately authorized milestone: implement and physically validate explicit, time-limited removable-media execution authority
- [ ] Script directory rules, bounded reading, versioned format and negative tests
- [ ] Tests for parser fuzz targets and edge cases

## Proposed future feature: dual-personality USB (not implemented)

**Status:** exploratory roadmap only; no implementation authorization, scheduled release, or change to Phase 2C/2D acceptance. The original T-Dongle-S3 has one BOOT/GPIO0 button and ESP32-S3 native USB. The currently flashed Phase 2D candidate is HID-only with direct read-only SDMMC access; USB MSC and a writable FAT/VFS path do not yet exist.

**Concept:** enumerate as an ordinary microSD-backed USB Mass Storage (MSC) device on normal insertion; a deliberate triple-click of BOOT requests a switch to ChaoN's USB HID identity, with an optional deliberate return to storage mode. Use **mutually exclusive USB personalities**, not an always-present MSC+keyboard composite, so ordinary insertion never exposes the keyboard interface. Switching would disconnect and re-enumerate USB, with correct descriptor/state teardown and no concurrent host/device filesystem ownership.

**Investigation gates (order tentative):**

1. **Read-only MSC prototype:** independently verify TinyUSB/ESP-IDF support, sector reads, host enumeration, correct capacity/bounds, and compatibility with original ESP32-S3 hardware. Keep firmware's filesystem access offline while MSC owns the card. No host writes in this stage.
2. **Mode switch and physical gestures:** design a higher-level BOOT event recognizer that safely distinguishes triple-click from current single/double/hold actions. A triple-click must not first arm or execute the existing fixed demo. Preserve BOOT-held-power-on ROM download; suppress transition/reconnection releases and cancel on USB loss. Prove mode-switch USB detach/re-enumeration on Windows and regression on the original board.
3. **Writable MSC only after a storage-integrity review:** evaluate host write callbacks, cache coherence, FAT geometry, media-change handling, pending writes, SYNCHRONIZE CACHE/eject, power-loss behavior, and write-protect/error policy. Do not claim a firmware-triggered USB disconnect guarantees Windows has flushed buffered data. Forbid simultaneous host writes and ChaoN reads/inspection/staging; acquire exclusive ownership only after confirmed safe transition, otherwise refuse switching and require physical unplug/eject.
4. **Product regression and recovery:** test host eject and unexpected disconnect, write interruption, card absence and cold replacement, media corruption, USB identity and OS driver behavior, SD service teardown/restart, staged-program invalidation, 20 ms button polling boundaries, benign fixed-demo gating, ROM recovery and rollback. Require separate approvals for firmware flash, removable-media writing and release.

**Safety boundary:** storage mode exposes no HID keyboard interface; storage writes (if ever enabled) must not translate into execution authorization. HID mode must not present writable MSC or concurrently access a host-owned card. Phase 2C/2D validation and the separate removable-media execution-authority review are prerequisites, not implied by this roadmap item.

## Phase 3: product hardening

- [ ] Host layout selection and user documentation
- [ ] Decide USB IDs and release packaging (source repository public under AGPL-3.0-only; no production firmware release)
- [ ] Threat model script ingestion and firmware update pathway
- [ ] End-to-end on-device test matrix, reproducible release artifacts
- [ ] No release without owner approval

Deferred, not promised: Wi-Fi script management, OTA, signed scripts, remote control, BLE HID. Keep the initial path deliberately simple and auditable.

