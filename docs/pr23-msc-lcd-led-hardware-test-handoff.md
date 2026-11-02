# PR #23 original-board USB MSC LCD/LED test handoff

Status: **TEST PLAN ONLY, NOT HARDWARE AUTHORIZATION OR ACCEPTANCE**. Prepared 2026-11-02.
Repository: `thetechbenders/chaon-usb` / draft PR #23 (`feat/msc-ducky-one-shot`).
Last inspected source: `04f80a2fc3f37d8ca41c49a055e3961f741cbd5b`.
Source CI: [ChaoN CI run 147](https://github.com/thetechbenders/chaon-usb/actions/runs/41209344404), SUCCESS for native `-Werror` tests, policy checks, UBSan, original ESP-IDF v5.5.3 ESP32-S3 build and candidate packaging.

**Before any bench session**: Refresh the PR head SHA and verify exact-head CI again. If changed, disregard the SHA above as an executable candidate. Keep the PR draft. **Never flash the older `2c08c401...` candidate / artifact `12645771310`**: independent review identified an auto-remount/formatting hazard in that older image. Preserve the existing `v0.1.0-rc1` rollback image, flash commands, provenance and host-side recovery access. Flashing requires Dan's explicit, separate permission.

## What currently exists and does not exist

- Original LILYGO T-Dongle-S3, 16 MB flash, no PSRAM. Not S3 Plus.
- MSC default identity: VID:PID `303A:4002`. HID-only identity: `303A:4004` after safely consuming the empty root `DUCKY` file on cold boot.
- MSC LCD is intended to show `USB STORAGE`, sanitized FAT volume label or `USB DRIVE`, used/total capacity, utilization bar, and `AT BOOT` legend. Capacity is a **cold-boot snapshot**, not live.
- APA102 currently emits a dim green idle level with a brief heartbeat when `tud_mounted()`. **It does not indicate actual USB transfers**. Do not mark read/write LED activity as PASS or advertise it as implemented.
- USB MSC setup explicitly configures `auto_mount_off = 1`, `do_not_format = true`, USB ownership, and blocks enumeration on backend startup failure. Source-policy CI confirms the configuration, not physical USB behavior.
- Pinned `esp_tinyusb` v2.0.0 MSC backend has no supported per-transfer activity hook in this integration. Actual read/write indication remains future implementation, requiring careful upstream inspection, nonblocking signaling and target regression testing.
- Known upstream risk: synchronous MSC write errors may be acknowledged as success. Even a passing copy/eject/readback smoke test **does not establish reliable general-purpose storage**.

## Bench prerequisites and evidence

Use a sacrificial known-good FAT32 microSD containing no valuable data, an original T-Dongle-S3, an accessible Windows test host, tested USB data cable, serial logs, and a separately verified ROM recovery and rollback procedure. Save the card image/hash if its starting state matters. Capture the exact firmware commit SHA, CI URL and job outcomes, candidate archive + internal file hashes, flashing offsets (from verified package), board revision, host OS version, card model/capacity, photos/video of LCD+LED, Windows USB descriptors, serial log, and per-case results with time stamps.

**Go/no-go before flashing:** (1) named reviewer has checked the current source and pinned MSC implementation, including known write-error/format risk; (2) exact-head CI is green; (3) correct-board firmware bundle and hashes are verified; (4) Dan has explicitly authorized the test flash; (5) rollback/recovery verified. If any fails, stop.

## Supervised test sequence (only after approval)

| ID | Procedure | Expected / evidence |
| --- | --- | --- |
| B01 | Cold-connect valid FAT32 card with no `DUCKY` marker. | Windows sees only MSC `303A:4002`, no HID keyboard interface, no automatic keystrokes. Photograph `USB STORAGE`, volume label, usage bar, used/total and `AT BOOT`. |
| B02 | Observe LED idle for at least 10 seconds and while Windows enumerates. | Dim idle green with brief heartbeat when USB mounted. **No real transfer flash is expected in this version**. Film the sequence. |
| B03 | On disposable FAT32 card, write test files of several sizes, including a zero-length file and files crossing sector/cluster boundaries; safely eject. | Windows copy/eject reports and serial logs retained. No app-side mount/format. Disconnect, read card independently and compare cryptographic hashes/lengths to sources. Note any failures; a green UI is insufficient. |
| B04 | Reconnect after host writes. | Capacity bar may change only after this cold boot; verify `AT BOOT` remains truthful. Compare against actual free space with rounding tolerance. |
| B05 | Create exactly empty root regular file `DUCKY` on host; flush/eject, disconnect, cold-connect. | Marker safely deleted before USB startup, Windows enumerates HID-only `303A:4004`, not MSC, no auto-type. A deliberate BOOT PICK/ARM/RUN must still be required. Record serial and descriptors. |
| B06 | Disconnect/reconnect with marker already consumed. | Default MSC-only returns; no persistent HID latch and no keystrokes. |
| B07 | Put nonempty file named `DUCKY`, directory instead of marker, or inaccessible media (separate cases). | Never switch to HID. Invalid marker remains nonauthorizing; failed FAT mount/unmount may block USB entirely. Capture exact failure mode and logs, without asserting host data safety beyond evidence. |
| B08 | Test FAT unsupported/corrupt media and absent media on sacrificial card/fixture. | Never format, remount or expose HID as a fallback. A blocked USB device is acceptable when ownership cannot be guaranteed. Preserve card before/after images where feasible. |
| B09 | Hold BOOT during USB insertion for ROM download/recovery. | ROM recovery remains available, no unexpected keyboard interface or automatic execution. Follow documented recovery procedure; do not overwrite rollback artifact. |
| B10 | Abort/replug/eject under controlled conditions on sacrificial card; test post-operation integrity. | No app-side media owner starts in MSC mode. Capture disconnect behavior, error logs and post-reconnect hashes. **Do not intentionally interrupt active writes on valuable data.** |
| B11 | Attempt to trigger SD read/write errors with a fault-injection fixture, if available. | Record exact host-visible result versus card readback. This is currently an unresolved P1 and **BLOCKED**, not PASS, without the fixture. |
| B12 | In intentional HID mode, run previously accepted PICK -> ARM -> RUN and cancellation smoke cases. | No automatic script execution; selected-script safety and the read-only SD worker remain functional. No MSC backend coexists. |

## Proposed implementation follow-up: actual LED transfer activity

Do **not** infer activity from `tud_mounted()`, LED animation time, or LCD refresh. First inspect the exact pinned `esp_tinyusb` 2.0.0 MSC callbacks and their task/ISR/ownership context. If a clean supported hook is absent, assess an isolated, pinned component patch instead of double-defining TinyUSB callbacks or wrapping SDMMC arbitrarily. Signal only successful/attempted MSC data transfer events with clear semantics, using a lock-free/atomic counter or nonblocking queue; **do not call GPIO, LCD, logging or blocking primitives from hot transfer callbacks**. Render APA102 from the existing application loop, preserve dim idle heartbeat, and avoid saturating on repeated reads. Define honest behavior on failed writes and when USB is not configured. Add native event-state tests (idle/read/write bursts, rollover, tick wrap, event loss), strict target build and USB regression checks. New exact-head CI and supervised repeat of B01-B12 are required before claiming this feature physically works.

## Verdict template

- Tested commit and flashed image SHA:
- CI run / job URLs:
- Board, SD, Windows details:
- B01-B12 per-case PASS / FAIL / BLOCKED / N/A with evidence:
- LCD text and volume snapshot accuracy:
- Idle LED observation:
- **Real read/write activity LED: NOT IMPLEMENTED (unless a later independently validated commit adds it)**:
- Descriptor captures and no-auto-type:
- File hashes before/after eject and reconnect:
- FAT/no-format/ownership observations:
- Recovery and rollback result:
- Remaining faults and explicit risk disposition:
- Reviewer sign-off / Dan release decision:

**No merge, release or production reliability claim follows automatically from this plan or a green CI run.**
