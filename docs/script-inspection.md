# Phase 2C inspection and Phase 2D bounded staging

**Development candidate; both branches remain unmerged.** The earlier stacked
Phase 2D candidate `e34e621` containing this Phase 2C inspection path was
physically accepted on the original T-Dongle-S3 on 2026-10-23: **33 PASS,
0 FAIL, 1 NOT TESTED, 2 NOT APPLICABLE**. The in-flight supersession race was
not physically established, and internal staged bytes were deliberately not
instrumented; native adversarial tests provide that evidence. The later F3
sector-scratch clearing change has not been flashed or physically tested. No
SD-derived HID program was executed. The embedded fixed demo remains the only
executable path; Phase 2D staging grants no execution authority.

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
  characters per TYPE, 1–10000 ms delay. Parser workspace and candidate output
  are private to the SD task, held off its stack and cleared after every attempt.
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
   is zeroed afterward. Phase 2D may copy a fully validated candidate into one
   generation-bound internal stage; the UI still receives only the summary.
4. The LCD renders `CHECKING`, `OK <count> <first-op>`,
   `SYNTAX LINE <n>`, or a bounded error category (`NOT CHN`,
   `TOO LARGE`, `STALE FILE`, `SD READ ERROR`, etc.).
   The screen does not show raw script payloads and never authorizes HID.
5. Superseding selection, browser exit, USB loss and a lost/reinitialized
   card invalidate the mailbox generation. The SD task discards late
   results from earlier generations. An SD read I/O error causes the
   SD observation task to fail closed and reinitialize.

The 20 ms button controller and its fixed HID demo remain unchanged. Even
a *valid staged* file has **no executable authority**. `app_main` does not call
the program copy interface; its existing `chaon_run(&demo, &io)` continues to
use the original embedded fixed demo, never an SD program. See
[program-staging.md](program-staging.md) for ownership and invalidation.

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
result suppression, identity mismatches, pending/complete snapshots, stage
replacement, invalid/partial program rejection, buffer guards and the absence
of VM/HID linkage from staging, plus summary-only LCD framebuffer bounds.

Physical acceptance applies to the earlier stacked `e34e621` firmware and its
observable behavior. A Phase 2C-only binary was not separately flashed, the
in-flight read race was not reproduced physically, and staged program bytes
were not observed on-device. ESP-IDF v5.5.3 ESP32-S3 builds remain required for
the real target. A successful build is not hardware validation, and neither
draft branch has been merged or released.
