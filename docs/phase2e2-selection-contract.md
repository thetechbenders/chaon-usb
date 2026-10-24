# Phase 2E.2: selected-program ownership contract

Status: implementation in progress, **not yet executable**. This branch is
stacked on the hardware-accepted Phase 2E.1 draft PR #17 head `ba999824`.

## Product interaction

- In BROWSE, `PICK` requests validation and selects **what** is loaded.
- After successful source revalidation and immutable copy, return to READY and
  show the selected 8.3 filename, not merely a rotating SD catalogue preview.
- READY shows either `DEMO` or the explicitly selected valid `.CHN` program.
- A first fresh BOOT tap `ARM` grants one 30-second run window for **that**
  selection. A second fresh BOOT tap `RUN` consumes the authorization once.
- After completion or cancellation, return to READY unarmed.
- Invalid PICK never silently falls back to the demo during ARM/RUN; show an
  explicit invalid-selection outcome and require deliberate selection.
- The embedded demo remains available only as a clearly labeled explicit
  choice. Do not silently execute it when a selected file becomes unavailable.

## Required execution safety boundaries

Selection is not execution authority. An execution token must bind the exact
validated raw 8.3 FAT identity, source-byte fingerprint, staged generation, USB
connection epoch, and immutable owned program. A directory identity and file
size do not detect same-size modification. Avoid treating the existing summary
snapshot as freshness evidence; re-read and hash exact source bytes.

Changes to USB connection, SD presence, catalogue, generation, selected source,
fingerprint, or controller state revoke ARM and stale programs. A reset starts
unselected/unarmed. No automatic resume, retry, typing on insertion, API
trigger or keyboard report while browsing.

A separate physical ARM and RUN remain mandatory. VM runs only after all
preconditions are checked and the immutable program has been atomically
transferred into its execution-owned slot. On cancellation, attempt all-keys
release; release failure is a FAULT. Preserve BOOT-held ROM recovery. Execute
bounded ChaoN Script v0 only; no DuckyScript or arbitrary command expansion.

## Current branch slice

`chaon_selection.c` provides static, bounded ownership and fail-closed
invalidations on freshness/USB epoch changes. It is intentionally **not**
connected to app_main, source re-reader, BOOT state machine, VM or TinyUSB.
An API caller must supply a fully validated program and a truthful fingerprint
from exact source bytes. Tests cover basic token and revocation invariants.

Next: implement exact-source fingerprinting and SD task revalidation; create
atomic one-shot execution ownership; wire READY selection presentation; replace
the separate hold-to-confirm gesture with normal ARM/RUN only when tests and
review demonstrate no aliasing or accidental authorization. The current
hardware-accepted Phase 2E.1 firmware remains the fallback.

No flash, merge, tag or release is authorized by this document.


## Added one-shot execution-owned slot

A separate statically allocated `chaon_execution_slot_t` now copies a selected,
validated program into a one-shot owner only if the caller provides an explicit
authorization predicate and a matching generation, USB epoch, and full SHA-256
source fingerprint. `take` consumes the slot and clears the source buffer.
This does **not** implement physical authorization; the predicate is currently
a parameter, not a certified controller output. No application code calls the
slot or sends its contents to TinyUSB.

The SD task still needs to independently read and hash exact source bytes,
revalidate them before ARM/RUN, and atomically bind the owned snapshot under
the correct lock. A staged program alone is not execution authority.

### CI note

The existing GitHub Actions CI pull-request trigger is limited to the `main`,
`feat/esp-idf-foundation`, and `feat/sd-readonly-probe` base branches.
Because PR #18 is stacked on draft PR #17, no automatic exact-head PR CI
is expected until it is explicitly dispatched or separately validated.
Do not label it software-validated on the basis of earlier Phase 2E.1 CI.


## Independent Claude review remediation, slice 1

- **S1:** `chaon_source_verify` is a bounded read-only FAT re-enumeration,
  file re-read, SHA-256 recomputation, and exact digest comparator. It wipes
  both scratch buffers. A host regression changes source bytes without
  changing size, cluster or filename and requires verification to fail.
  The helper must still be invoked from the SD task immediately before an
  execution transaction; there is **no** such live transaction yet.
- **S2:** A staged selection may issue a one-use grant on a caller-asserted
  physical RUN edge. `prepare` consumes the grant, and `take` rechecks
  live selection generation, USB epoch, SHA digest and a caller-provided
  independent freshness result. Clearing selection makes a prepared slot
  invalid and wipes it. Tests now assert this failure. **The physical
  controller is NOT yet wired as the grant's sole issuer**, so this remains
  an inert API, not a finished security boundary.
- **M1:** Replaced `diagnostic_lock` spinlock with a task-context FreeRTOS
  static mutex in the SD service, so multi-kilobyte stage copy/clear/publish
  no longer disable interrupts. Heavy disk I/O still runs outside the mutex,
  and the host harness asserts reads occur outside the lock.
  Mutex initialization is in `chaon_sd_probe_start`. The target build
  and contention behavior must be verified before accepting this change.

No SD-to-HID VM routing, active selection UI or physical RUN integration has
been added. The last-moment freshness check and physical token issuance
remain **integration gates**. Do not flash the accepted 2E.1 device with this
draft code or merge it as a completed execution bridge.
