# ChaoN MSC B10 eject remediation: pinned-source integration candidate

**UNFLASHED DEVELOPMENT ONLY.** Baseline: frozen, physically qualified `bfea174590ff88707ccfd5717cef08565ea40dbd`. Changes are isolated on `fix/msc-eject-state`; do not update PR #23's branch, flash hardware, merge, or tag on the strength of this document.

## Evidence and proven upstream behavior

macOS `diskutil eject` reports success but volume remounts within ~5 s (2/2 observations). `diskutil unmountDisk` remains unmounted; cold reconnect restores MSC. No USB command trace confirms specific SCSI commands or who initiates the remount.

Pinned `esp_tinyusb` 2.0.0 upstream currently acknowledges START STOP UNIT eject with a call to `msc_storage_mount_to_app()`. Because ChaoN uses `auto_mount_off=1`, that helper does nothing. TEST UNIT READY still returns ready while USB owns the SD. It also uses `usbd_defer_func()` for physical SD writes and returns successful WRITE10 length before the write executes. This function returns void, so an internal queue insertion failure cannot be reported to the caller. The deferred callback reuses a storage-object buffer, so independent host acknowledgment is not evidence of persistence. The prior sticky red latch observed only failures in the deferred worker, not silent queue insertion failure. This is a substantive data-integrity gap.

Pinned TinyUSB core `msc_device.c` supports `TUD_MSC_RET_ASYNC`, but `tud_msc_async_io_done()` also relies on `usbd_defer_func()`. For this single-card implementation, a straightforward blocking SD write inside the USB callback is easier to reason about than modifying the USB event-queue API. It trades safety auditability against USB task latency and watchdog/timeout risks: physical validation and failure injection remain mandatory.

## Implemented candidate (requires CI/review)

Two strict source transformations run after the existing original-SHA-256 activity patch:
- A second script removes `tusb_write_func()` and `msc_storage_write_sector_deferred()` from the build-local copy of pinned `tinyusb_msc.c`. WRITE10 now calls `msc_storage_write_sector()` **synchronously** and only returns successful length if SDMMC reports ESP_OK. On failure, existing red error evidence is latched and SCSI command fails. The generated source retains upstream license notices. The managed ESP-IDF component remains unchanged.
- Eject START STOP UNIT latches an in-RAM single-boot ejected state. TEST UNIT READY fails with NOT READY / MEDIUM NOT PRESENT; READ10/WRITE10 reject further I/O with not-ready sense; READ CAPACITY returns zero blocks while ejected. A START before eject is a no-op success, but an in-session LOAD/START does **not** rearm after eject. Power-cycle/cold boot is the intended rearm.
- A missing LUN storage pointer is explicitly checked before dereferencing. GET_MAX_LUN still reports one configured LUN to TinyUSB, which subtracts one for the USB wire reply. Further adversarial transport validation remains necessary.
- `auto_mount_off=1`, `do_not_format=true`, USB-only storage ownership, and marker-to-HID behavior are unchanged.

The portable eject policy tests include malformed LUNs, readiness, pending-write rejection (as a model constraint), pre-eject START, latched eject, post-eject refusal, no silent reload, and counter limits. A synthetic transformation test checks callback and synchronous-write injection, plus missing/duplicate source anchors. Both are invoked by `make test`, including under UBSan.

**Important distinction:** the production write path is synchronous and never creates a pending-write model reservation; the pending counter is retained as a pure-state test for future async policy. Do not claim it measures hardware queue depth.

## Risk and review gates

- Blocking SDMMC writes occur inside the TinyUSB task. Measure worst-case normal and failure latency and host timeouts. Evaluate watchdog exposure; do not assume successful file hashes prove bounded latency.
- `sdmmc` write completion is an ESP-IDF driver result, not proof of nonvolatile NAND persistence; SD cards can cache internally. Preserve independent-reader readback. Check SCSI SYNCHRONIZE CACHE behavior before release.
- The silent `usbd_defer_func` queue failure can still affect other TinyUSB internal deferred operations, but the product WRITE10 data path no longer uses that mechanism.
- Unsupported-LUN handling has been hardened in the upstream storage getter; broader malformed-CBW/SCSI robustness is not proven. No fuzzing on the qualified board.
- Further review of write-error sense, zero-length writes, capacity-after-eject behavior, USB BOT reset, and same-session LOAD/START host compatibility is required.
- Built candidate source must be checked against pinned original SHA and generated-source transformations; native and UBSan checks are necessary but insufficient.
- Hardware authorization requires exact ESP-IDF v5.5.3 original T-Dongle-S3 build, new exact-commit artifact hashes, independent review, and explicit supervised flash decision. Re-run B03 integrity, B05/B06 HID one-shot, B08 missing-card failure, B09 ROM recovery, B10 macOS eject and unmount. Capture SCSI/USB traces when possible.

**No hardware flash, merge, release, eFuse, rollback modification, or automated formatting is authorized.**
