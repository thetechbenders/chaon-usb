# MSC eject/reopen investigation and proposed state contract

Status: **SOURCE-ONLY DESIGN / NATIVE TESTS. NOT CONNECTED TO USB CALLBACKS. NOT FLASHABLE AS AN EJECT FIX.**

Baseline: frozen supervised-test firmware `bfea174590ff88707ccfd5717cef08565ea40dbd`.
Evidence: on macOS, `diskutil eject` reported success but the FAT32 volume remounted within ~5 seconds, observed twice. `diskutil unmountDisk` stayed unmounted, and cold reconnect restored MSC. The test SD card's ten files passed independent-reader hashes. No USB SCSI trace establishes which commands macOS sent.

## Confirmed source mechanism

Pinned `espressif/esp_tinyusb` v2.0.0, source `device/esp_tinyusb/tinyusb_msc.c` at `b8c36cdd...`:
- `tud_msc_start_stop_cb` calls `msc_storage_mount_to_app()` on LOAD/EJECT + STOP and returns success.
- ChaoN configures `auto_mount_off=1`, so that mount helper skips all storage. Keep this setting and `do_not_format=true`; they prevent dangerous host/app ownership overlap and automatic formatting.
- `tud_msc_test_unit_ready_cb` reports ready whenever LUN 0 is USB-owned; no ejected latch exists.
- READ10/WRITE10 callbacks lack ejected-state checks.
- WRITE10 queues physical SD writes via `usbd_defer_func(tusb_write_func, ...)` and may acknowledge the host before physical SD success/failure. The existing sticky red fault only catches the known deferred error branch.
- Unsupported LUN 1 can dereference a missing storage pointer, inherited upstream; not fixed by the policy model here.

**Inference, not proven causality:** After eject, continued readiness may allow macOS to rescan/remount the still-attached device. Host USB-command evidence is required to attribute the observed remount conclusively.

## New source-only model

`main/chaon_msc_eject.[ch]` is a pure, native-testable model. It is not compiled into ESP-IDF firmware and does not change either USB personality.

Required integration contract:
1. All state transitions in actual MSC callbacks must be serialized with deferred-write start/completion (or use an audited synchronization implementation).
2. Maintain a correct count of accepted but not yet physically completed writes. Every reserved write is completed exactly once on both success and error, with no counter overflow/underflow.
3. An eject with pending writes must not report success prematurely; consider a nonblocking deferred completion design and SCSI semantics rather than busy-waiting the USB task.
4. Valid eject latches media unavailable; TEST UNIT READY returns NOT READY / MEDIUM NOT PRESENT; READ10/WRITE10 reject post-eject access with correct SCSI sense, without app FAT access.
5. A fresh firmware reset/cold boot rearms media. Same-session reload behavior is intentionally rejected in this initial model and needs OS compatibility review.
6. Do not let a backend fault be disguised as an ejection. The existing fault-red latch remains independently observable until reset/power loss.
7. Reject unsupported LUN safely in all callbacks, including GET_MAX_LUN consistency, before any raw host fuzzing.
8. Preserve normal HID marker handling and NO-AUTO-FORMAT / single-owner invariants.

## Native tests

`tests/test_msc_eject.c` verifies: ready-only USB ownership and LUN 0, invalid LUN rejection, pending-write rejection of eject, release then latched eject, post-eject reads/writes denied via policy, explicit in-session reload denied, NULL-state handling, and counter overflow. `make test` / UBSan now include the executable. These tests do **not** prove pinned TinyUSB integration or hardware USB behavior.

## Release/test boundaries

- Frozen `bfea174` firmware stays untouched; do not flash this new branch.
- No merge, release, tag, full erase, eFuse modification, or rollback change.
- Before integration: independent review of deferred-write queue/ordering and host START STOP semantics; decide whether accepted write completion can be observed at the correct point.
- After integration: strict native and UBSan checks, real ESP-IDF v5.5.3 original T-Dongle-S3 build, generated-source pin/hash verification, protocol-level SCSI tests, explicit exact-branch package hashes, and separate permission for an incremental disposable-media flash.
- Re-run B01, B03 (eject/replug/readback hashes), B05/B06, B08, B09, and B10, with USB trace where possible. Lack of remount alone does not prove write durability.
