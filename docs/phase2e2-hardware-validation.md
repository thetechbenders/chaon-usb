# Phase 2E.2 hardware validation plan

Status: prepared on 2026-10-24; **candidate not flashed and physical tests not
run**. The firmware-under-test is frozen at
`ae70e489ccd7ca22e2dcbe466bc5f544940a55f7`. Any later documentation-only
commit does not replace that firmware revision or authorize a different
artifact.

## Safety boundary

- Test only the original LILYGO T-Dongle-S3: ESP32-S3, 16 MB flash, no PSRAM.
- Use the owned Windows test host, a plain text editor, and the disposable
  FAT32 card. Never focus a shell, terminal, password field, browser, or
  privileged application while HID output is possible.
- SD-derived HID requires the complete BROWSE -> PICK -> READY -> ARM -> RUN
  path. PICK, insertion, reconnect, catalogue changes, invalidation, and an
  expired grant must remain inert.
- Stop immediately for unsolicited output, the wrong script, execution after
  invalidation, failed cancellation, a held key, or failed ROM recovery.
- Do not erase the chip, write eFuses, change USB drivers, format media, or
  write any image outside the three manifest regions.
- The candidate flash and any later restoration of the accepted Phase 2E.1
  image are separate writes. Explicit approval must cover each intended write;
  otherwise pause in ROM mode before the unapproved operation.

## Frozen candidate and flash layout

| Item | Verified value |
| --- | --- |
| Source | `ae70e489ccd7ca22e2dcbe466bc5f544940a55f7` |
| CI | run `40131847526`; native and ESP-IDF v5.5.3 jobs passed |
| Artifact | `12573854980`, `chaon-esp32s3-candidate` |
| ZIP SHA-256 | `abe10ebffa2d2949de1fb860b41f701097f930e65c86d45b7eb0e946183d69c7` |
| Bootloader | `0x0`, 20,832 bytes, SHA-256 `82c19ce825793da7df337ebe59e4cbef057ab76d112316552349094044bd05e2` |
| Partition table | `0x8000`, 3,072 bytes, SHA-256 `7f00b6c042a89b15b0cac534f82ed988caf29278ff5700b0c511eb1b5bb7c820` |
| Application | `0x10000`, 325,008 bytes, SHA-256 `2ee7a7972b23783e394ace8aef65377781fca9cece434156e3b775533e6abba6` |
| Flash settings | ESP32-S3, DIO, 80 MHz, 16 MB |
| App fit | `0x4f590` in a `0x100000` app partition; `0xb0a70` bytes (69%) free |

The workflow event was the intentionally separate CI-only draft PR #19 against
`main`. GitHub built synthetic merge
`f500be1a1ae2cf13017847601676254a31f819a1`. Its Git tree
`43f06b0d8987afad8877377071c19ed0cc3b8fd9` exactly equals the frozen
source tree, so embedded app version `f500be1` has the authorized source
content. Product PR #18 remains the stacked draft on Phase 2E.1.

The artifact-generated command uses `--before default-reset --after
hard-reset`. Because the supervised procedure begins with a separately
identified board already in ROM and must retain ROM for verification, use the
established no-reset form below after rediscovering the port:

```powershell
python -m esptool --chip esp32s3 --port <ROM_PORT> --baud 460800 --before no-reset --after no-reset --no-stub write-flash --no-progress --flash-mode dio --flash-freq 80m --flash-size 16MB 0x0 bootloader\bootloader.bin 0x8000 partition_table\partition-table.bin 0x10000 chaon.bin
```

Do not add `erase-flash`. After the write, run a separate `verify-flash` with
the same chip, port, no-reset, no-stub and flash arguments. Then use
`read-flash` to capture exactly `0x5160` bytes at `0x0`, `0xc00` bytes at
`0x8000`, and `0x4f590` bytes at `0x10000`. SHA-256 each readback and require
an exact match to the three source-image hashes above before normal boot.

## Setup

1. Recheck PR #18 is draft and its firmware ancestor is the frozen source.
2. Recheck artifact ZIP and individual image hashes from the local candidate
   directory. Do not use an artifact from a later documentation build.
3. Recheck both 16 MB factory backups and the accepted Phase 2E.1 rollback ZIP
   without moving, modifying, or uploading them.
4. Rediscover `303A:1001` and the COM port. With no reset and no stub, require
   ESP32-S3 QFN56 revision v0.2, MAC `CC:BA:97:3A:4E:CC`, flash `ef/4018`
   and 16 MB. Stop on any mismatch.
5. Prepare an ordinary editor containing only a unique session header. Keep
   hands away from the keyboard during inertness checks.
6. Use a USB extension that leaves the SD slot safely accessible if powered
   removal tests are attempted. If the slot cannot be accessed without
   disturbing USB, mark those cases BLOCKED rather than improvising.
7. Prepare fixture batches from `tests/hardware/phase2e2/fixtures`. The
   catalogue exposes at most eight root entries; do not delete unrelated files
   or claim a fixture is visible when it is beyond that bound.

## Reproducible fixture batches

- Batch A: `BENIGN.CHN`, `SLOWRUN.CHN`, `KEYREL.CHN`.
- Batch B: `FRESHA.CHN` staged on the card under the chosen test name, with
  `FRESHB.CHN` retained off-card as the equal-size mutation payload.
- Batch C: `BADGRAM.CHN`, `EMPTY.CHN`, `OVER2048.CHN`, `CONTROL.TXT`.

Record card root metadata, file byte counts, and SHA-256 before power-up.
Whenever a card write is authorized, perform it in a reader, verify the copied
bytes, eject cleanly, and insert the card while the dongle is unpowered unless
the individual test explicitly requires powered removal.

## Acceptance sequence

Every result must include firmware SHA, date, operator, observed result,
PASS/FAIL/BLOCKED status, and the supporting log or screenshot name.

1. **Clean boot and inert enumeration.** Boot with BOOT released. Require
   normal `303A:4004` / `CHN-S3-DEV` HID enumeration, readable READY LCD,
   expected LED, responsive browser, no COM port, and no editor changes.
2. **Embedded demo regression.** Start from READY with no selected SD program.
   One short debounced press/release must enter ARMED without typing. A second,
   distinct short press/release must type exactly
   `ChaoN USB HID smoke test` once and return READY without a stuck key.
3. **Browser and valid PICK.** Hold BOOT for at least 900 ms but less than
   3,000 ms to enter BROWSE, navigate with short taps, and hold 900-2,999 ms
   on `BENIGN.CHN`. Require valid inspection and PICK completion with no HID.
4. **Selected-name truthfulness.** After PICK, observe READY through at least
   two catalogue rotation intervals and require `BENIGN.CHN` to remain shown.
   Short-tap ARM and require the same selected name throughout ARMED.
5. **Distinct ARM and RUN.** The first short gesture may only arm. Release BOOT
   fully and observe a stable released interval. Only a later second short
   gesture may request verification/run; a held transition or reused release
   must not execute.
6. **Benign selected execution.** With `BENIGN.CHN` selected, perform ARM then
   RUN. Require a fresh verification followed by exactly
   `ChaoN 2E2 A<TAB>B<ENTER>`, one time, with no demo text and no extra key.
7. **Same-size source change after PICK.** Preserve filename, size, first
   cluster and raw directory identity while replacing the selected bytes from
   the FRESHA payload with FRESHB. ARM/RUN must reject it with no selected or
   demo output. Card removal/catalogue loss is a valid fail-closed test but
   does **not** isolate the SHA-256 freshness check; record the stronger case
   BLOCKED unless identity preservation and the verification attempt are
   evidenced.
8. **SD removal.** In separate clean trials remove the accessible test card
   during READY, ARMED, and source verification. Require selection/grant
   revocation, no typing, no automatic resume after reinsertion, and a new
   deliberate PICK before any selected run. Verification is normally too
   brief for unaided timing; mark that subcase BLOCKED if not actually caught.
9. **BOOT cancellation.** Attempt BOOT cancellation during verification and,
   separately, after `SLOWRUN.CHN` has typed `START` and entered its delay.
   Require no continuation, no demo fallback, released keys and safe return.
   Do not claim the verification subcase if the press occurred only after it.
10. **USB epoch invalidation.** Disconnect while ARMED, reconnect with BOOT
    released, and require READY, no selected authority, no resume, no typing,
    and a fresh deliberate selection/ARM/RUN sequence.
11. **Thirty-second expiry.** ARM a valid selection, leave BOOT untouched for
    at least 35 seconds, and require automatic READY with no output. A later
    single short gesture may arm but must not consume the expired grant.
12. **Verification timeout and SD I/O failure.** Use an approved fault
    fixture capable of holding a read past five seconds and a separate read
    failure. Require fail-closed cancellation and no demo. Ordinary removal
    usually creates immediate failure rather than timeout; if no controlled
    fault source exists, record the timeout case BLOCKED and retain native
    evidence separately.
13. **Rejected sources.** Require `BADGRAM.CHN`, `EMPTY.CHN` and
    `OVER2048.CHN` to reject, and `CONTROL.TXT` to remain non-selectable.
    None may reach ARM as a selected program or produce HID.
14. **No demo fallback.** After any selected execution is rejected by source
    change, SD loss, verification failure or timeout, require that the same RUN
    transaction emits neither selected text nor fixed-demo text.
15. **Single-use authority.** After one successful `BENIGN.CHN` run, require
    no automatic repeat. A lone subsequent short gesture must not rerun it.
    Reselect and perform a fresh two-gesture transaction for a second selected
    run; record both outputs separately.
16. **Key release and transport loss.** Cancel `KEYREL.CHN` while it is
    typing, then manually enter lowercase text and verify no sticky modifier.
    In a separate trial disconnect USB during active typing, reconnect with
    BOOT released, and require no resumed output or held-key symptom. A
    firmware-side failed-release FAULT cannot be claimed physically observed
    while the transport is absent unless separate instrumentation captures it.
17. **ROM recovery and accepted restoration.** Stop normal tests, perform the
    established BOOT-held reconnect, rediscover `303A:1001` and recheck chip
    identity read-only. If restoration is explicitly authorized, write and
    independently verify the accepted Phase 2E.1 artifact
    `12542959189` / `ba999824f1189747cdb465be56aab98bea58add3`, then boot
    normally and require READY, `303A:4004` / `CHN-S3-DEV`, and no typing.

## Honest evidence limits

Normal operator timing cannot reliably land inside a millisecond-scale SD
verification, and ordinary card removal invalidates catalogue identity before
it proves an in-place same-size hash mismatch. Those cases need a safe storage
fault/interposer fixture or must remain BLOCKED. Native state-machine tests are
supporting evidence, not physical PASS substitutions. Photographs establish
display state but not hidden buffer ownership, source bytes, HID report
ordering, or exact debounce timing.

Successful preparation or flashing is not hardware acceptance. Hardware
acceptance requires every applicable result in the evidence ledger, with all
BLOCKED/NOT RUN limitations preserved.
