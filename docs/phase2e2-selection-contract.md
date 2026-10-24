# Phase 2E.2: selected-program ownership contract

Status: pre-flash remediation candidate, **not physically tested or hardware
accepted**. Draft PR #18 remains stacked on the hardware-accepted Phase 2E.1
PR #17 head `ba999824f1189747cdb465be56aab98bea58add3`.

## Product interaction

- BROWSE and PICK choose a file; neither action grants HID authority.
- PICK completes only after the asynchronous inspection is valid and the
  application owns a bounded immutable program copy plus the exact source
  SHA-256, raw FAT identity, staging generation and USB epoch.
- READY and ARMED show that selected `.CHN` filename. Catalogue rotation is
  used only when no SD program is selected.
- A fresh short BOOT press/release arms for 30 seconds. A second, distinct
  short press/release requests one run. The former Phase 2E.1 five-second
  confirmation state and screen are not part of Phase 2E.2.
- With no SD selection, the same two gestures retain the embedded fixed-demo
  behavior. A failed PICK or invalidated selected run is blocked; the same RUN
  event never falls through to the demo.

## Execution boundary

The production decision layer is the only adapter between debounced controller
events and execution choice. For an SD selection it requires, in order:

1. an exact selected-program snapshot;
2. distinct controller `ARM` then `EXECUTE` events inside 30 seconds;
3. a single-use grant bound to generation, USB epoch and SHA-256;
4. a new SD-task FAT enumeration and exact source reread;
5. an actual passing freshness result within five seconds; and
6. preparation and immediate take of the bounded execution-owned program.

The actual freshness result is passed to both execution-slot operations. It is
not replaced by a constant or inferred from the old staged hash. The grant,
slot and verification ticket are consumed or cleared on every terminal path.

USB disconnect, SD/catalogue/identity loss, LCD loss, browser replacement,
BOOT cancellation, verification failure or timeout revokes unconsumed
authority. A selected-source failure enters a blocked state and clears the
selection, so it cannot silently run the embedded demo. Re-entering BROWSE or
starting a new USB epoch establishes a new deliberate context.

During VM execution, BOOT or USB loss still requests cancellation. Every VM
exit attempts all-keys release; release failure remains a FAULT. BOOT-held
power-on remains the ESP32-S3 ROM recovery path.

## Ownership and memory

Selection, grant, verification request and execution slot have distinct
owners. All program objects are fixed-capacity static storage, not app-task
stack allocations. SD I/O and hashing remain on the SD task outside the
mutex; only bounded metadata/program publication uses the task-context mutex.
The SD service is read-only.

## Validation gates (completed for the accepted candidate)

Host tests exercise the production controller-to-decision path for PICK,
ARM/RUN, successful freshness, failed freshness, missing grants, USB loss, SD
loss, cancellation, timeout and demo-fallback prevention. Full native
warnings-as-errors plus UBSan/ASan and an exact ESP-IDF v5.5.3 ESP32-S3 build
must pass on the final commit. Independent review remains a gate before any
controlled physical test.

The accepted candidate was flashed and verified under explicit operator approval, and PR #18 subsequently merged. This document does **not** authorize any subsequent flash, tag, or firmware release.
