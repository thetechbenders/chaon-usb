# Phase 2C: bounded read-only script inspection (portable backend)

**Development scope: NOT hardware validated, NOT flashed, NOT merged.**
This is the first implementation slice, not the complete on-device
inspection workflow.

## Implemented in the draft backend

- Only regular FAT32 root **DOS 8.3** entries with raw extension `.CHN`.
  No subdirectory traversal, long filename reconstruction, mounting or writes.
- Reads exactly the selected file's directory-declared length when it is
  **1 to 2047 bytes**. Output has room for a separate NUL terminator.
  At most four 512-byte data sectors are copied. Cluster numbers, active FAT
  selection, capacity, partition and geometry bounds are validated. Cycles,
  premature end-of-chain, invalid clusters, malformed metadata, unexpected
  media I/O and stale root selection all reject without a partial result.
- A root-directory **rescan** before data reading checks the original
  selected entry's raw name, type, size and cluster. Display-sanitized
  filenames never authorize the file reader.
- The source validator accepts ASCII text, LF/CRLF, tabs and the existing
  ChaoN Script v0 grammar. It rejects embedded NUL, binary/control/high-bit
  bytes, oversized input and invalid scripts. The grammar itself is defined
  in [script-language.md](script-language.md): max 32 commands, max 80
  characters per TYPE, 1–10000 ms delay. The scratch parsed program is private,
  held off task stack, cleared immediately after inspection, and never exposed.
- A small diagnostic summary can report valid/invalid, first instruction
  category, count and first failing line. It contains **no script text,
  parsed instruction array, file address, executable callback or HID command**.

The portability boundary uses a **read-sector callback only**, with no
sector-write function, generic filesystem mount or script-file executor.
Tests use synthetic FAT32 sectors, injected read failures and malformed
metadata. Only FAT32 is supported for **file content reading** in this
initial backend even though Phase 2A enumerates FAT12/16/32 roots.
No physical FAT12/FAT16 support is implied.

## Explicitly not implemented

The backend is **not yet called by the firmware SD task**. There is no
on-device selected-file read request, no mailbox/result snapshot exposed
to the LCD, and no new UI inspection view. The Phase 2B browser still
only toggles a visual PICK indicator.

No script sourced from SD can be parsed by the running firmware or
reach the HID executor because the path is not connected. The existing
embedded fixed HID smoke test and BOOT physical safety controls remain
unchanged.

Completing the on-device Phase 2C workflow will require a separately
reviewed SD-task-only request path, stale-media handling, UI diagnostics
without source bytes, and physical smoke tests. SD-derived scripts must
remain permanently disconnected from the HID executor until a distinct,
authorized Phase 2D threat-model and approval.

## Testing and disposition

Host tests should cover valid 1- and 2-sector scripts, exact length,
FAT32 cluster traversal, invalid/missing FAT links, stale entry metadata,
oversized and unsupported files, injected read failures, malformed ASCII,
source limit, parse rejection and no read callback writes.

ESP-IDF v5.5.3 ESP32-S3 builds are required for the real target. No
new firmware should be flashed, merged or released merely because this
portable backend compiles. The original board continues running the
physically tested Phase 2B binary.
