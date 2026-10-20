# Phase 2A follow-on: bounded read-only FAT root preview (draft)

## Status
Hardware smoke-tested development candidate in [draft PR #12](https://github.com/thetechbenders/chaon-usb/pull/12), built on [PR #9](https://github.com/thetechbenders/chaon-usb/pull/9).
**Code tests and the applicable original-board/Windows/FAT32 hardware smoke tests passed on 2026-10-20.** This does not establish production readiness, FAT12/FAT16 behavior on physical media, or full-directory browsing.

This is a deliberately limited root-directory *metadata preview*, not a
general file manager. No file data are opened, selected, or executed.

## Filesystem reader
- A pure C library accepts **only a sector-read callback**. No write,
  erase, format, VFS mount, FatFs write API or disk-output descriptor is
  present. The embedded adapter calls only `sdmmc_read_sectors`.
- Supports DOS FAT12/FAT16/FAT32 root 8.3 names on FAT superfloppies or
  primary MBR FAT partitions. The FAT type is determined from data cluster
  count, not untrusted "FAT" ASCII type markers.
- Rejects unsupported/GPT/exFAT/invalid sector sizes and inconsistent BPB
  geometry. All LBAs are checked against the reported physical card size.
- Lists at most **8 root entries**, scanning no more than **64 directory
  sectors**. Long filenames, Unicode, nesting, recursion, file-content reads,
  and directory-selection actions are unsupported. A directory is displayed
  only by its short name: it is not traversed.
- Names are short-name fallbacks with unprintable characters sanitized.
  A partial listing is explicitly **not** an exhaustive inventory.
- The observer publishes only an immutable bounded catalogue snapshot.
  Main UI owns all LCD I/O. While READY, names rotate every 3 seconds in
  the pre-existing 160x8 footer; in ARM/RUN/FAULT the original SD status
  footer is retained. The BOOT/HID controller and payload remain unchanged.
- Status: `SD DIR` on a successfully parsed root, `SD FS N` for an
  unsupported layout, `SD FS E` for invalid/IO errors. Card absence still
  reports `SD ERR C` and is not confused with an unsupported FAT type.
  `SD DIR` does not prove an exhaustive list or integrity of file contents.

## Hardware workflow
Never attempt to remove the card while this USB-A dongle is plugged in.
Hot-swap is **N/A** to the physical installed-card/cold-change workflow.

Before any flash: review source/CI, manifest and factory backup/rollback hashes
and explicitly obtain owner authorization. Use a disposable backed-up test
card with a known mix of 8.3 files and directories, confirm names on the LCD
and unchanged BOOT/HID behavior, then power off to change to empty/unsupported
or malformed media. Do not infer full-card immutability without an external
reader and a before/after whole-media hash comparison.

Completed gates: native C tests with -Werror, UBSan, ESP-IDF v5.5.3 ESP32-S3 build, in-scope source review, FAT32 file-display/rotation/8+ behavior on the real board, physical BOOT/HID regression, cold card restoration and ROM recovery. Independent second-party review remains optional. Other filesystem families and hosts, resource/electrical measurements and whole-card immutability remain unverified. This is NOT production firmware and does not enable SD script execution.

## Review hardening

- A valid MBR can coincidentally contain `0x0200` at offset 11; the
  superfloppy path requires both a FAT jump opcode and 512-byte BPB sector
  size before deciding not to inspect the primary MBR.
- For FAT32 with FAT mirroring disabled, the active FAT number from BPB
  extended flags is used; invalid indices fail closed.
- A truncated scan is never labelled `SD EMPTY`. If no names are available
  but the bound is reached, the footer shows `SD PARTIAL`; when a preview
  contains names but is incomplete, the displayed count has a `+` marker.

## Hardware smoke-test record — 2026-10-20

### Provenance and evidence type

- **Tested source:** `3c297b0b8038523c911a1f1cce7703dab8af6b06`, draft PR #12.
  [CI 39427011419](https://github.com/thetechbenders/chaon-usb/actions/runs/39427011419) passed native C -Werror tests, UBSan, and ESP-IDF v5.5.3 ESP32-S3 with zero reported warnings.
- **Flash artifact:** `chaon-esp32s3-candidate`, ID `12265813676`;
  ZIP SHA-256 `14be735a14fd47b2a28dcd50c2ee1f055bda2b800dafdd8e14eaea6bab6161c9`.
  The build's PR-merge Git tree matches the reviewed source tree.
- **Device:** original LILYGO T-Dongle-S3, ESP32-S3 v0.2,
  MAC `CC:BA:97:3A:4E:CC`, 16 MB flash. Recovery serial port was
  **COM3 during this session**; do not hardcode a COM port in procedures.
- **Flashing:** Dan authorized this candidate. Manifest-specific bootloader
  (`0x0`), partition table (`0x8000`) and app (`0x10000`) were flashed;
  an independent verification matched all three image regions. No full-chip
  erase, eFuse/security change or other flash operation was reported.
- **Media:** Windows identified a 125,829,120-byte (**120 MiB**) MBR/FAT32
  card, FAT32 volume size 121,571,328 bytes. The source's FAT12/FAT16 parser
  paths have synthetic tests, but **were not physically tested**.

Results below combine tool-reported provenance with Dan's recorded physical
observations. The session's raw logs and full Markdown report remain on
Dan's Windows computer, not in this repository; this section was prepared
from the canonical ChaoN project evidence record (Mem v27), not by inspecting
those local report files directly.

| Applicable test | Disposition | Evidence |
| --- | --- | --- |
| Normal boot / HID and LCD | **PASS** | Green READY, intact branding/BOOT hints, normal Windows HID enumeration, no unsolicited typing, freeze or reset |
| Read-only FAT32 root preview | **PASS** | Eight short names rotated about every 3 s; tilde, dot, underscore, hyphen and directory label appeared legibly; `8+` marked truncation and ninth entry was not displayed |
| Physical BOOT/HID | **PASS** | Arm, approximately 30 s expiry without typing, one exact fixed demo, active cancellation/key release with no remaining text or accidental rearm; primary state display took precedence |
| Empty slot / cold restoration | **PASS** | Unplugged removal yielded `SD ERR C` with normal HID; unplugged restoration returned the same root preview and normal demo |
| ROM recovery / final normal boot | **PASS** | BOOT-held ROM startup and read-only no-stub identity/security check; final boot had green READY, filename rotation and healthy HID |
| Source read-only boundary | **PASS (static inspection)** | `sdmmc_read_sectors` only, no SD write/format/mount, no card-sourced HID commands; 8-entry/64-directory-sector bounds and small synchronized snapshot |
| Powered hot-swap | **NOT APPLICABLE** | Card slot inaccessible while plugged in; all card changes made unpowered |

The observed first eight names were `SYSTEM~1`, `OLD.BIN`, `TOOLS`,
`HELLO.TXT`, `TEST.BIN`, `FILE~1.TXT`, `A_B.TXT` and `A-B.TXT`.
The additional `EXTRA.TXT` was not displayed; the footer indicated `8+`.
No card file was opened or executed as part of the dongle's firmware.

**Not tested:** whole-card before/after hash (raw-media read was
permission-denied on Windows); post-test per-file immutability; actual
CPU/stack usage, electrical/current/timing measurements, endurance, other
hosts, FAT12/FAT16 physical media, and physical malformed/unsupported/empty
filesystem cases. Static read-only inspection is not a byte-for-byte media
immutability proof. No UART traces or SD CID measurements were captured.

**Disposition:** the FAT32 root metadata preview passed the agreed
original-board/Windows hardware smoke-test scope. This is **not a production
release qualification** or permission to merge. Factory recovery contents
were located on a different PC and not rehashed in this FAT-preview session;
a known-good firmware rollback package was separately verified.

Local source-evidence locations (not checked into Git):

- `C:\Users\danie\Documents\ChaoN-USB-Recovery\2026-10-20-092125-fat-preview\hardware-validation-report.md`
- `C:\Users\danie\Documents\ChaoN-USB-Recovery\2026-10-20-092125-fat-preview\documentation-update-proposal.md`

See [the fifth physical session](bringup.md#fifth-physical-session--phase-2a-fat32-root-preview-2026-10-20)
for the chronological validation history.
