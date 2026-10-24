# Phase 2D: bounded SD program staging

**Historical Phase 2D milestone:** this page records the staging boundary as it
existed before selected execution authority. The Phase 2C/2D branches later
merged through PRs #14 and #15, and Phase 2E.2 subsequently added guarded
selected execution. See [the current selection contract](phase2e2-selection-contract.md).

The earlier `e34e621` firmware was physically accepted on the original
T-Dongle-S3 on 2026-10-23 (**33 PASS, 0 FAIL, 1 NOT TESTED,
2 NOT APPLICABLE**). The in-flight read supersession race remains physically
untested, and private staged bytes were not instrumented. The later F3
sector-scratch clearing fix and Phase 2E work do not retroactively turn that
earlier evidence into observation of internal staged bytes. At the Phase 2D
milestone, staging was not an SD execution feature and the embedded fixed text
demo remained the only program passed to `chaon_run`.

## Contract

PICK creates two coordinated, one-slot requests under the existing SD-service
lock: the summary-only Phase 2C inspection request and a program-stage request.
Only the SD task may take them. It then performs the FAT32 re-read and parsing
outside the critical section. Publication requires all of the following:

- the source is 1–2047 bytes and passes the existing FAT32 bounds, fresh root
  rescan, raw DOS 8.3 entry identity, cluster-chain and media checks;
- the existing ChaoN Script v0 parser accepts the complete source with at most
  32 instructions;
- the stage still has the same generation, entry index, raw name, type, size
  and first cluster after parsing;
- the parsed program has a bounded, internally valid instruction shape.

The SD task parses into a private static candidate. A successful publication
copies that candidate into one fixed stage and then wipes the candidate,
source buffer, parser workspace and shared 512-byte sector scratch. Invalid
grammar, partial parsing, an empty or oversized file, stale identity, I/O
failure, cancellation or supersession leaves no valid program and clears the
affected buffers. No allocation is dynamic.

`chaon_sd_probe_staged_program_copy` is an inert, bounded copy-out interface
for a future separately authorized control feature. It is not called by the
current application, it grants no execution authority, and it performs no VM
or HID operation. A successful copy also returns the exact stage generation.
A future caller must provide static/non-stack destination
storage and must add a separate authorization state machine. Merely obtaining
a valid staged copy must never be treated as human consent.

## Ownership and lifetime

| Object | Owner and lifetime | Crossing rule |
| --- | --- | --- |
| 512-byte sector scratch | SD task, static for firmware lifetime | Never published; explicitly wiped after each inspection attempt, then legitimately reused by later sector probing and catalogue operations |
| 2048-byte file source | SD task, static for firmware lifetime | Wiped after every attempt |
| 2048-byte parser workspace | SD task, static for firmware lifetime | Wiped by the compiler and again by the task |
| Candidate `chaon_program_t` | SD task, static, one inspection attempt | Published only by bounded copy after all checks; then wiped |
| One-slot staged program | SD service, static for firmware lifetime | Protected by the existing lock; valid only for its generation and exact identity |
| LCD inspection summary | Main/UI snapshot | Contains state/count/op/error line only; never program text or instructions |

No SD I/O or parsing occurs under the SMP critical section. Lock-protected
work is limited to state/identity operations and a bounded program
clear/publication/copy. The current firmware never performs the optional
program copy-out. The SD task stack remains 4096 bytes; the source, parser
workspace, candidate and staged program are all static rather than task-stack
objects.

Measured with local GCC on the host ABI, `chaon_program_t` is 2952 bytes and
the complete stage is 3000 bytes. An installed ESP32-S3 Xtensa compiler reports
2948 and 2996 bytes respectively from DWARF. Compile-time guards reject a
program over 3072 bytes or a complete stage over 3200 bytes. These figures do
not replace an ESP-IDF v5.5.3 target build or on-device high-water measurement.

## Invalidation

The staged slot is wiped and its generation changes on:

- a new or different PICK request, including replacement while a read is in
  flight;
- clearing PICK or navigating to another browser entry;
- browser exit or the existing 60-second browser inactivity timeout;
- catalogue invalidation/replacement, SD reinitialization or read failure;
- changed root-entry identity or failed source/parser validation;
- USB disconnect, controller transition out of the selected browser state, or
  controller completion/reset paths;
- power/reset, which reinitializes all static validity state as empty.

The SD task carries the generation and complete raw-entry identity captured
when it took the request. A completion that arrives after any invalidation or
replacement cannot publish into the newer slot. A failed publication cannot
clear or overwrite a newer request.

## Historical Phase 2D trust boundary

At that milestone there was no SD write callback, writable mount, subdirectory
traversal, long-filename reconstruction, network trigger, new USB interface,
or call that executed an SD program. `app_main` retained the single existing
`chaon_run(&demo, &io)` call whose input was the embedded harmless fixed demo.
LCD behavior was summary-only. Current Phase 2E.2 selected execution preserves
the read-only storage boundary but uses a separate, physically authorized and
freshness-verified execution path.
