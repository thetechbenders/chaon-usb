# Phase 2A: read-only microSD probe and LCD diagnostics (draft)

**Flashed after authorization; partial physical validation recorded below.**
Dan has no UART adapter. The advisory LCD footer provided diagnostic visibility
for the disposable, operator-reported 128 MB card without changing USB HID.
PR #9 stays draft/unmerged. Powered removal/reinsertion remains NOT TESTED
because the card is inaccessible while this dongle is plugged into USB.

## Read-only scope

Original T-Dongle-S3 candidate pins: CLK12, CMD16, D0=14 (1-bit, 10 MHz).
D1=17/D2=21/D3=18 are not used by this probe. No verified card-detect or
write-protect GPIO; pullup adequacy remains unproven. No PSRAM is assumed.

A separate low-priority task initializes the host/card, checks the 512-byte
sector size, reads sector zero once, then polls card status every 3 seconds.
After errors it cleans up and retries after 5 seconds. There is no sector
write, formatting, mount, file operation, directory browsing or card-derived
HID instruction. MBR/FAT/UNKNOWN classification remains a log-only hint, not
filesystem validation. The observer and existing fixed HID demo stay separate.

## LCD diagnostic contract

The existing branding, primary READY/ARMED/RUNNING/CANCELLED/FAULT display,
ARMED countdown and BOOT hint are preserved. The new bottom line occupies
rows 72–79 below the BOOT hint; SD colors never change the APA102 LED policy.

| Footer | Meaning | What it does NOT prove |
| --- | --- | --- |
| SD INIT | Initialization/retry in progress; yellow | Card presence; this state can be brief |
| SD OK | Card initialized, 512-byte sector zero read succeeded; green | Filesystem integrity, capacity, mounted files or byte-for-byte immutability |
| SD ERR H | SDMMC host initialization failed; red | Missing card |
| SD ERR B | Bus/slot setup failed; red | Correct physical pin routing/pullups |
| SD ERR C | Card initialization failed or card unavailable; red | Absence versus unsupported/unreadable/electrically faulty media |
| SD ERR Z | Unsupported sector size; red | A safe sector-zero read (it is skipped) |
| SD ERR R | Sector-zero read failed; red | Filesystem corruption |
| SD IO S | Card status poll failed after an earlier successful read; red | Physical removal versus a general I/O error |
| SD ERR D | Host cleanup failed; red | Original cause of the preceding error |
| SD ERR T | Probe task could not start; red | A card failure |
| SD ERR U | Unknown diagnostic value; red | A specific media diagnosis |

Codes identify **stages**, not exact ESP-IDF error numbers. Detailed error
names and sector-zero hints remain in logs when a UART is available.
No distinct "NO CARD" claim is made. An empty slot and a card-init failure can
both produce C. SD IO S is loss/status-error evidence, not a card-detect event.
Error codes remain during the retry wait, then INIT may appear before a new
result. A cleanup failure can replace an earlier error with D.

## Task/UI separation and redraw cost

The SD task publishes only a small enum under an ESP-IDF SMP critical section.
The UI takes a coherent snapshot under the same lock. No SD, log, rendering
or SPI operation occurs while locked; the SD task never touches the LCD.

The main loop retains ownership of the framebuffer and LCD. Primary state or
ARMED-second changes use the existing full-frame draw. An SD-only change,
including during READY, repaints/transfers only the 160×8 bottom strip:
2,560 bytes instead of 25,600 bytes. Unchanged snapshots do not redraw.
At the configured 16 MHz SPI clock, strip payload time is nominally 1.28 ms
plus command/render overhead; this is arithmetic, not a measured button-latency
guarantee. No additional delays are added to BOOT polling. While the fixed
demo runs synchronously, new SD observations are displayed when its execution
returns; card I/O never blocks that path. Rapid intermediate snapshots may
coalesce; this footer is current status, not an event-history recorder.

LCD failure keeps the existing advisory failure behavior and never arms HID.

## UART-free hardware procedure (only after review and flash authorization)

1. Verify factory-backup and known-good rollback digests, exact CI candidate
   provenance/package/manifest, and dynamically identify the expected ESP32-S3
   MAC/revision/16 MB flash in ROM mode. No full-chip erase or eFuse changes.
2. With the disposable 128 MB card installed, flash only verified manifest
   images and separately verify them. Boot with BOOT released and an ordinary,
   non-elevated Windows text editor focused. Confirm healthy HID, green READY,
   READY / BOOT TO ARM, no typing, unclipped readable footer and intact hints.
3. Observe INIT followed by **SD OK**, or record the exact ERR/IO code and
   timing. OK proves successful sector-zero read, even when the logged sector
   hint would be UNKNOWN. Do not infer card capacity, SDSC compatibility or a
   filesystem type from this compact footer. If it never reaches OK, stop and
   diagnose the code before hot-removal testing; record the failed gate.
4. Verify arm, near-30-second countdown and expiry without typing. Execute the
   fixed demo into the editor: exact ChaoN USB HID smoke test, cyan/RUNNING,
   then green/READY. Cancel a repeat only after characters appear; record the
   exact partial output, red/CANCELLED, key release, and no arm on release.
   Repeat ordinary deliberate button gestures around countdown/footer changes;
   record missed presses, delays, clipping, freezes or resets.
5. Check mechanical access before planning powered removal. On Dan's original
   dongle, the card is obstructed while plugged into USB: **do not attempt
   powered removal/reinsertion in this setup**. Record that test NOT TESTED.
   A future separately arranged fixture must permit safe access while powered
   before testing status-poll errors, retry and same-runtime recovery. A general
   IO S/error still does not prove a dedicated physical card-detect event.
6. For the available setup, unplug before changing the card. Cold empty-slot
   and restored-card boots below test separate initialization paths; they do
   not establish hot-swap or same-runtime host reinitialization. Stop on
   crashes, watchdog loops or unresponsiveness; do not repeatedly reflash.
7. Boot with an empty slot. Record the actual error/retry sequence; verify
   inert READY, working HID/countdown and no freeze/reset loop. No claim that
   memory use or runaway logging was measured without instrumentation.
8. Restore the card, confirm OK and existing UI/HID behavior. Verify BOOT-held
   ROM recovery with read-only identification, no extra flash. Reconnect
   normally; final state must be green READY, healthy HID, no unwanted typing
   or stuck keys, and footer accurately reflecting the observed SD outcome.
9. Report PASS/FAIL/NOT TESTED per gate, exact codes/text and timings. If an
   external card reader becomes available, compare whole-card hashes before
   and after. Without that comparison, report static read-only inspection and
   observed behavior separately; never claim byte-for-byte immutability.

The LCD does not expose detailed card capacity, sector-zero format hints, raw
error names or crash logs. A UART/debugger is still useful for unexplained
failures. No aggressive timing/electrical fault injection or destructive test.

## Automated verification and remaining limits

Native tests cover every diagnostic label (including invalid values), distinct
rendering of each code, guarded framebuffer bounds, preservation of the
primary UI/countdown/hints, stale-text clearing on partial transitions,
READY error/recovery redraws, and countdown/state redraw priority.
Existing parser/VM/BOOT/LED/sector-classifier tests remain in the suite,
including UBSan. ESP-IDF v5.5.3 ESP32-S3 CI is required on the exact candidate.
These tests do not establish actual panel readability, task scheduling,
SDMMC/SDSC compatibility, hot removal, pullups or physical input latency.

Future filesystem design and bounded directory browsing require separate
scope/review; generic FatFs mounting is not proof of read-only media.
No SD-sourced script execution, USB CDC, HID descriptor change, release or tag.

## Observed hardware results — 2026-10-19

Firmware f22f18e4e95c77aa1eae086944f4886793a05cef was flashed after Dan's
explicit authorization from artifact 12232863952, successful CI 39352112253
(native -Werror/UBSan and ESP-IDF v5.5.3 ESP32-S3, zero warnings).
Separate flash verification passed for all three manifest images.

- Installed disposable card: operator reported SD OK, legible/unclipped footer,
  normal green READY, inert insertion, no resets/freezes. This establishes the
  successful initialization/512-byte sector-zero-read path on this card.
- Arm/countdown/expiry, exact fixed demo, and mid-stream cancel passed by
  operator report. Full text: `ChaoN USB HID smoke test`; cancelled prefix:
  `ChaoN USB HI`. Requested LED/UI/key-release/no-rearm checks affirmed.
- Empty-slot normal boot: exact `SD ERR C`, normal READY and functioning HID.
  The error briefly changed to unreadable yellow text; INIT is source-expected
  but was not legibly observed. No exact retry timing or resource use measured.
- Card restored only while unplugged: SD OK returned; arm expiry remained
  inert. This is cold restoration, not same-runtime SD host reinitialization.
- ROM recovery: rediscovered COM4/expected MAC; read-only no-stub identification
  succeeded without reflashing. Final normal boot SD OK/green READY/inert HID
  and no stuck keys/reset/freeze confirmed; Windows HID/keyboard status OK.
- Powered removal/reinsert: **NOT TESTED**, card inaccessible while plugged in.
  Other ERR/IO states were not fault-injected. Whole-card immutability hash,
  actual capacity/SDSC/CID/sector hint, precise latency, CPU/memory/log volume,
  electrical/pullup behavior and endurance remain unverified. No UART logs.

See [the fourth physical session](bringup.md#fourth-physical-session--phase-2a-lcd-sd-probe-2026-10-19)
for package/image hashes, tool evidence and A–F dispositions. This is partial
physical validation; keep PR #9 draft/unmerged and stop before filesystem work.
