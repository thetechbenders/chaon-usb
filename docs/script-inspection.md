# Phase 2C: bounded read-only script inspection (portable backend)

**Development candidate: connected to the optional SD task and browser LCD,
NOT hardware validated, NOT flashed, NOT merged.** The existing HID demo
remains the only executable path. A successful build alone is not hardware
validation.

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

## On-device inspection path and trust boundary

1. Enter the existing Phase 2B browser with a long BOOT hold/release. Short
   taps browse through the bounded root entries.
2. Hold ~1.2 s and release to mark the current root entry PICKED. Only
   this selection requests a **read-only** inspection from the SD task.
   Holding again clears PICK; a tap moves to the next name and clears
   any old request; a ~3.5 s hold exits the browser.
3. The main/UI task submits just an index and bounded raw root identity.
   A one-slot lock-guarded mailbox accepts it only when the current
   catalogue identity still matches. The SD task exclusively executes
   sector reads and parser checks. Its static 2048-byte source scratch
   is zeroed afterward; no source text, decoded program or callback is
   shared with the UI or controller.
4. The LCD renders `CHECKING`, `OK <count> <first-op>`,
   `SYNTAX LINE <n>`, or a bounded error category (`NOT CHN`,
   `TOO LARGE`, `STALE FILE`, `SD READ ERROR`, etc.).
   The screen does not show raw script payloads and never authorizes HID.
5. Superseding selection, browser exit, USB loss and a lost/reinitialized
   card invalidate the mailbox generation. The SD task discards late
   results from earlier generations. An SD read I/O error causes the
   SD observation task to fail closed and reinitialize.

The 20 ms button controller and its fixed HID demo remain unchanged. Even
a *valid* file has **no executable authority**. Parsed instructions live
only briefly in private static inspector scratch; `app_main`'s existing
`chaon_run(&demo, &io)` continues to use the original embedded fixed demo,
never an SD program.

The Phase 2C implementation intentionally does **not** display complete
script lines or provide script execution, launch, trust grants, USB output
from card data, a write API, or a privileged host-control path. These
would require separate authorization, design and safety review.

## Testing and disposition

Host tests cover valid one-to-four-sector scripts, exact length,
FAT32 cluster traversal, invalid/missing FAT links, stale entry metadata,
oversized and unsupported files, injected read failures, malformed ASCII,
source limit, parse rejection and no read callback writes.

Additional host tests exercise mailbox replacement, cancellation, stale
result suppression, identity mismatches, pending/complete snapshots,
and the summary-only LCD framebuffer bounds. The FreeRTOS task and GPIO/SD
timing still require original-board physical smoke tests.

ESP-IDF v5.5.3 ESP32-S3 builds are required for the real target. No
new firmware should be flashed, merged or released solely because
CI passes. The original board continues running the physically tested
Phase 2B firmware until Dan separately authorizes a new flash.
