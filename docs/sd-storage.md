# Phase 2A: read-only microSD discovery (draft)

Status: **first experimental increment only**. This code is not flashed or
physically validated. It is not a filesystem driver or script browser.

## Confirmed vs proposed

- Source-backed board pin candidate, original LILYGO T-Dongle-S3 vendor
  `examples/sd_card/sd_card.ino`: CMD GPIO16, CLK GPIO12, D0 GPIO14,
  D1 GPIO17, D2 GPIO21, D3 GPIO18. In the new code only D0/CMD/CLK
  are used in **1-bit SDMMC mode**, 10 MHz maximum.
- No hardware-tested card-detect (CD) or write-protect (WP) GPIO.
  The controller uses internal pullups for bring-up; board-level pullup
  adequacy and insertion/removal behavior remain unproven.
- No PSRAM. Static 512-byte, aligned transfer buffer; no unbounded
  directory buffers or dynamic script allocations.
- Card initialization, CMD13-style status and a single **read-only**
  sector-zero read run in a **separate low-priority task**, never in the
  HID/button control loop. Retry 5 s on failures; status poll every 3 s
  while the card is responsive.
- No `sdmmc_write_sectors`, no FatFs/VFS mounting, no file opens,
  formatting, formatting fallback, directory listing, script selection,
  script parsing, or script execution from the card.
- Sector-zero signature classification (MBR or FAT boot) is only a
  **hint**, not validation of filesystem integrity or safe directory
  traversal. Errors and card removal without a dedicated CD signal may
  be indistinguishable.
- Existing physical BOOT two-gesture HID demo remains the sole way
  to type the fixed benign text.

## First hardware gates (requires separate flash approval)

1. Confirm original board, pin references, 3.3 V SD wiring and pullup
   situation; use a sacrificial backed-up test card.
2. Check clean boot with **no card**. READY LED/LCD and HID operations
   must be unaffected; observe SD_UNAVAILABLE in logs.
3. Insert a preformatted test card before startup. Check SD_PRESENT
   and plausible sector-zero classification. **Do not assume presence
   from a sector signature alone.**
4. Remove and reinsert a test card while running. Check that status
   errors and retry/reinitialization recover without hanging USB HID.
   No assertion of hot-swap safety until this passes on the hardware.
5. Test unreadable/corrupt test media and repeat cycles; check that
   no files on the card change.
6. Repeat arm, expire, execute fixed text, cancel mid-stream and
   reconnect. Measure responsiveness while SDMMC probing if possible.
7. Capture serial logs and a test-card before/after content hash.
   Report separately: CI, flash verification, operator observation,
   instrument measurement, and untested paths.

## Remaining Phase 2A work (NOT implemented in this increment)

- Confirm FAT read-only mounting architecture; ESP-IDF v5.5.3 FatFs
  includes write support by default, so **do not call the general
  mount helper and claim a hardware-enforced read-only filesystem**.
  Evaluate whether SD read-only disk I/O wrappers or a deliberately
  limited file parser are preferable. Never accidentally write to media.
- Bounded directory enumeration with filtering and invalid-name/size
  tests; card filesystem validation before reading file contents.
- LCD card presence/error/file count and, eventually, a read-only file
  browser. Input scheme needs approval; BOOT remains the HID arm/execute
  control and must not have ambiguous meanings.
- Reliable absence/removal classification, bounded retry and fault
  recovery including testing a removal mid-transfer.
- No card-sourced scripts may become executable without a separate
  safety review and explicit project-owner approval.

No release, tag, flashing, or modifications to factory backup.
