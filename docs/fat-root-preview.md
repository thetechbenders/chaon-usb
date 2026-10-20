# Phase 2A follow-on: bounded read-only FAT root preview (draft)

## Status
Implementation proposed in draft PR #12, stacked on firmware PR #9.
**Not flashed or physically validated.** CI and independent review required.

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

Open gates: CI and native negative tests, independent bounds review,
UI readability, real FAT type and card geometry confirmation, cross-platform
and resource measurements. This is NOT production firmware and does not
enable SD script execution.

## Review hardening

- A valid MBR can coincidentally contain `0x0200` at offset 11; the
  superfloppy path requires both a FAT jump opcode and 512-byte BPB sector
  size before deciding not to inspect the primary MBR.
- For FAT32 with FAT mirroring disabled, the active FAT number from BPB
  extended flags is used; invalid indices fail closed.
- A truncated scan is never labelled `SD EMPTY`. If no names are available
  but the bound is reached, the footer shows `SD PARTIAL`; when a preview
  contains names but is incomplete, the displayed count has a `+` marker.
