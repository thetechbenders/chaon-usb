# Roadmap and release gates

**As of 2026-10-24:** Phase 2E.1 and 2E.2 are merged into `main`. This page tracks current gates, not historical draft-branch status.

## Completed milestones

- [x] Original T-Dongle-S3 C11/ESP-IDF architecture, parser, TinyUSB HID, BOOT input and APA102 status.
- [x] Physical fixed-demo arm/run/cancel, USB enumeration, inert insertion and ROM recovery on the original board.
- [x] ST7735 160×80 display bring-up and status interface.
- [x] SDMMC observation, FAT root metadata preview and one-button browser (Phase 2A/2B).
- [x] Strict bounded FAT32 root `.CHN` reading, source inspection, and immutable program staging (Phase 2C/2D).
- [x] Phase 2E.1 inert execution-authority foundation, merged via PR #17.
- [x] Phase 2E.2 PICK → ARM → RUN, one-use grants, fresh SD reread/hash, selected display and no-demo fallback, merged via PR #18.
- [x] Phase 2E.2 controlled original-board acceptance: **14 PASS, 0 FAIL, 4 BLOCKED, 1 PARTIAL, 2 N/A, 1 NOT REQUIRED**, on source `ae70e489`.

## Remaining validation

- [ ] Physical injection of same-identity file mutation after PICK.
- [ ] Timing-controlled BOOT cancellation during SD verification.
- [ ] Forced verification timeout and injected live SD read failure.
- [ ] Last-moment verification rejection with explicit no-demo-fallback observation.
- [ ] Broader host/layout, stress and fault-coverage evidence when specifically scoped.

These are unresolved test limitations, **not** reported firmware failures. Powered card removal during verification is outside the original board's manual test scope.

## Release readiness

- [ ] Resolve [issue #5](https://github.com/thetechbenders/chaon-usb/issues/5): license/distribution confirmation, USB identifiers, firmware provenance, recovery/update packaging, script trust and layout limitations.
- [ ] Independently review exact release tree and artifact before publishing.
- [ ] Obtain explicit owner approval for release/tag/publication.

Hardware acceptance is not product qualification and does not itself authorize a public firmware release.

## Deferred ideas

Optional Wi-Fi script management, OTA, signed scripts, BLE HID and remote administration remain separate proposals, not current capabilities.
