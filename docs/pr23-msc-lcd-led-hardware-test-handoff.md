# PR #23 original-board USB MSC LCD/LED test handoff

Status: **TEST PLAN ONLY, NOT HARDWARE AUTHORIZATION OR ACCEPTANCE**. Prepared 2026-11-02.
Repository: `thetechbenders/chaon-usb` / draft PR #23 (`feat/msc-ducky-one-shot`).
Implemented/tested firmware source: `b0a291d77dcdc71bf3f97d7cfb4c886df3b9a864`.
PR-associated integration CI: [ChaoN CI run 149](https://github.com/thetechbenders/chaon-usb/actions/runs/41439339187), **SUCCESS**: native `-Werror` tests including activity/patch tests, policy and package tests, UBSan, original ESP-IDF v5.5.3 ESP32-S3 build, candidate verification/upload. Local native/UBSan and all 23 Python tests also passed.

Starting head `d306289f14f85a5dbb20a4c3006f5164c6841662` was confirmed exact-head green in [run 148](https://github.com/thetechbenders/chaon-usb/actions/runs/41438303973) **before firmware edits**. Subsequent handoff-only commits record results; they do not change firmware. PR workflows use a synthetic merge checkout; exact branch-head build provenance additionally requires a workflow-dispatch run on the branch. Refresh current PR metadata and verify CI on that exact head before any bench work.

Run 149 artifact: `13137118541`, `chaon-esp32s3-candidate`; outer Actions archive digest `sha256:979639e5c6ae775fe65316db0df3a5dd2acb233cee28f4a1fcd36889cfa19b5b` (not the inner firmware ZIP hash). Downloaded inner package independently passed verification against its actual synthetic merge source `991ce12992ed9d2d638d136edc2ff45cbace249a`; verification against the feature SHA correctly rejected the provenance mismatch. Run 149 is associated with feature head `b0a291d77dcdc71bf3f97d7cfb4c886df3b9a864`, but its binary is labeled with that merge SHA. A direct branch workflow-dispatch run is required for exact-head binary provenance. Build logs confirm the generated `chaon_tinyusb_msc.c` and application activity source compiled; downloaded ELF defines both MSC callbacks and activity event/drain symbols. This establishes integration/build evidence, **not physical USB/LED function**.

Rollback `v0.1.0-rc1` remains at tag object `6006cdffd8eeeb63d9410a769f1ad0374624b3a6`; no rollback image, release, tag or flash operation was changed/performed. No merge or release was performed.

**Before any bench session**: Refresh the PR head SHA and verify exact-head CI again. If changed, disregard the SHA above as an executable candidate. Keep the PR draft. **Never flash the older `2c08c401...` candidate / artifact `12645771310`**: independent review identified an auto-remount/formatting hazard in that older image. Preserve the existing `v0.1.0-rc1` rollback image, flash commands, provenance and host-side recovery access. Flashing requires Dan's explicit, separate permission.

## Exact branch-source validation result

Exact tested branch commit: `6028ef2917fce0d0df3f7dfb5b3aea48bcece321` (firmware implementation unchanged from `b0a291d77dcdc71bf3f97d7cfb4c886df3b9a864`). Direct branch [ChaoN CI run 151](https://github.com/thetechbenders/chaon-usb/actions/runs/41439930656), `workflow_dispatch`, **SUCCESS** for native `-Werror`/patch tests, UBSan, package/policy tests, ESP-IDF v5.5.3 original-board build and verified/uploaded candidate. The downloaded package independently verifies against **that same exact SHA**. PR integration run [41439937359](https://github.com/thetechbenders/chaon-usb/actions/runs/41439937359) also passed for that head.

- Exact-source artifact ID: `13139466009`.
- Outer Actions artifact digest: `sha256:d39cd5405438c905d444ff5cc17318eb9d3f09efa6c229cac0a5d52d630d9237`.
- Inner candidate ZIP SHA256: `2b92b396088997a8f8d1238eee3b9da6227c89919174b7ec7ceeafa7406ab45c`.
- This handoff result is recorded in a subsequent documentation-only commit; check its current PR head and CI separately.
- Hardware results: B01-B12 **NOT RUN**; B11 fault injection **BLOCKED** without fixture. No flash authorization, physical acceptance, storage-transfer success, or reliability claim follows.

## What currently exists and does not exist

- Original LILYGO T-Dongle-S3, 16 MB flash, no PSRAM. Not S3 Plus.
- MSC default identity: VID:PID `303A:4002`. HID-only identity: `303A:4004` after safely consuming the empty root `DUCKY` file on cold boot.
- MSC LCD is intended to show `USB STORAGE`, sanitized FAT volume label or `USB DRIVE`, used/total capacity, utilization bar, and `AT BOOT` legend. Capacity is a **cold-boot snapshot**, not live.
- APA102 now has an application-side attempted-data activity implementation: blue READ10, amber WRITE10, purple mixed, with bounded pulses over the dim idle/mounted heartbeat. **Hardware UNVALIDATED**; attempts include failures and deferred writes, not successful transfer or persistence.
- USB MSC setup explicitly configures `auto_mount_off = 1`, `do_not_format = true`, USB ownership, and blocks enumeration on backend startup failure. Source-policy CI confirms the configuration, not physical USB behavior.
- Pinned `esp_tinyusb` v2.0.0 has no supported transfer hook. An isolated hash-checked build patch now signals nonempty callback attempts through a lock-free atomic latch. See `pr23-msc-activity-design.md` for source provenance, semantics and limitations.
- Known upstream risk: synchronous MSC write errors may be acknowledged as success. Even a passing copy/eject/readback smoke test **does not establish reliable general-purpose storage**.

## Bench prerequisites and evidence

Use a sacrificial known-good FAT32 microSD containing no valuable data, an original T-Dongle-S3, an accessible Windows test host, tested USB data cable, serial logs, and a separately verified ROM recovery and rollback procedure. Save the card image/hash if its starting state matters. Capture the exact firmware commit SHA, CI URL and job outcomes, candidate archive + internal file hashes, flashing offsets (from verified package), board revision, host OS version, card model/capacity, photos/video of LCD+LED, Windows USB descriptors, serial log, and per-case results with time stamps.

**Go/no-go before flashing:** (1) named reviewer has checked the current source and pinned MSC implementation, including known write-error/format risk; (2) exact-head CI is green; (3) correct-board firmware bundle and hashes are verified; (4) Dan has explicitly authorized the test flash; (5) rollback/recovery verified. If any fails, stop.

## Supervised test sequence (only after approval)

| ID | Procedure | Expected / evidence |
| --- | --- | --- |
| B01 | Cold-connect valid FAT32 card with no `DUCKY` marker. | Windows sees only MSC `303A:4002`, no HID keyboard interface, no automatic keystrokes. Photograph `USB STORAGE`, volume label, usage bar, used/total and `AT BOOT`. |
| B02 | Observe LED idle for at least 10 seconds and while Windows enumerates. | Dim idle green with brief heartbeat when USB mounted. **Attempt pulses are implemented but unvalidated**; observe idle separately from host background scans. Film the sequence. |
| B03 | On disposable FAT32 card, write test files of several sizes, including a zero-length file and files crossing sector/cluster boundaries; safely eject. | Windows copy/eject reports, serial logs and LED video retained. Observe blue read / amber write / purple mixed attempt pulses and a visible gap during sustained traffic; correlate against independent host traces, not just file-copy UI. No app-side mount/format. Disconnect, read card independently and compare cryptographic hashes/lengths to sources. Note any failures; a green UI is insufficient. |
| B04 | Reconnect after host writes. | Capacity bar may change only after this cold boot; verify `AT BOOT` remains truthful. Compare against actual free space with rounding tolerance. |
| B05 | Create exactly empty root regular file `DUCKY` on host; flush/eject, disconnect, cold-connect. | Marker safely deleted before USB startup, Windows enumerates HID-only `303A:4004`, not MSC, no auto-type. A deliberate BOOT PICK/ARM/RUN must still be required. Record serial and descriptors. |
| B06 | Disconnect/reconnect with marker already consumed. | Default MSC-only returns; no persistent HID latch and no keystrokes. |
| B07 | Put nonempty file named `DUCKY`, directory instead of marker, or inaccessible media (separate cases). | Never switch to HID. Invalid marker remains nonauthorizing; failed FAT mount/unmount may block USB entirely. Capture exact failure mode and logs, without asserting host data safety beyond evidence. |
| B08 | Test FAT unsupported/corrupt media and absent media on sacrificial card/fixture. | Never format, remount or expose HID as a fallback. A blocked USB device is acceptable when ownership cannot be guaranteed. Preserve card before/after images where feasible. |
| B09 | Hold BOOT during USB insertion for ROM download/recovery. | ROM recovery remains available, no unexpected keyboard interface or automatic execution. Follow documented recovery procedure; do not overwrite rollback artifact. |
| B10 | Abort/replug/eject under controlled conditions on sacrificial card; test post-operation integrity. | No app-side media owner starts in MSC mode. Capture disconnect behavior, error logs and post-reconnect hashes. **Do not intentionally interrupt active writes on valuable data.** |
| B11 | Attempt to trigger SD read/write errors with a fault-injection fixture, if available. | Record exact host-visible result versus card readback. This is currently an unresolved P1 and **BLOCKED**, not PASS, without the fixture. |
| B12 | In intentional HID mode, run previously accepted PICK -> ARM -> RUN and cancellation smoke cases. | No automatic script execution; selected-script safety and the read-only SD worker remain functional. No MSC backend coexists. |

## Activity implementation and remaining validation

The attempted-data hook is implemented as described in `pr23-msc-activity-design.md`. Physical behavior is **NOT VALIDATED**. It signals nonempty READ10/WRITE10 attempts, including errors and writes awaiting deferred execution. The application polls an always-lock-free atomic bit latch; the hook does no GPIO, LCD, logging, allocation, clock reads or blocking operations. Attempts coalesce; 75 ms pulses have at least 125 ms gaps in application time. This is not operation counting or a successful-transfer indicator.

**Remaining gates (all pending):** independent source/patch review; exact-head CI refresh; verified correct-board bundle/hash/offsets; separately authorized supervised flash; B01-B12 acceptance with host traces/video, exclusive descriptors/no-auto-type, FAT/no-format/ownership observations, independent eject/reconnect readback hashes, LCD snapshot, idle heartbeat and activity pulse timing, and ROM recovery/rollback. B11 fault injection remains **BLOCKED** without an appropriate fixture. Known deferred-write error reporting is unchanged. No general-purpose storage reliability claim is justified.

## Verdict template

- Tested commit and flashed image SHA:
- CI run / job URLs:
- Board, SD, Windows details:
- B01-B12 per-case PASS / FAIL / BLOCKED / N/A with evidence:
- LCD text and volume snapshot accuracy:
- Idle LED observation:
- **Attempted-data activity LED: IMPLEMENTED, HARDWARE NOT VALIDATED; no successful-transfer claim**:
- Descriptor captures and no-auto-type:
- File hashes before/after eject and reconnect:
- FAT/no-format/ownership observations:
- Recovery and rollback result:
- Remaining faults and explicit risk disposition:
- Reviewer sign-off / Dan release decision:

**No merge, release or production reliability claim follows automatically from this plan or a green CI run.**
