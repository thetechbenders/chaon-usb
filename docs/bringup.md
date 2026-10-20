# Original T-Dongle-S3 bring-up

**Initial physical smoke tests completed (2026-10-19).** The evidence below distinguishes tool verification from Dan's physical observations. Results apply to the tested original board and Windows host.

## Setup

1. Inspect the actual PCB and compare with the [hardware register](hardware.md). Do not assume Plus pins/memory.
2. Install ESP-IDF v5.5.3, select ESP32-S3 and build; validate artifact size, partition table and dependency lockfile resolution.
3. Use a sacrificial/owned test computer and a normal text editor; ensure the editor is focused before any authorized typing.
4. Record USB enumeration descriptors, VID/PID, endpoint type and OS driver detection.
5. Power up with BOOT released. Confirm **no keystrokes on insertion**, even if the editor is focused.
6. Press and release BOOT once; confirm armed indicator. Wait >30s; confirm timeout disarms and no typing.
7. Repeat arm, then second deliberate press-release before timeout; confirm exact harmless test string and no residual Shift/key.
8. Verify unplug/reconnect, reset, fast double-click, button held before insertion, USB suspend, and mid-script disconnect.
9. Check status LED output, idle/active current, and whether HID tasks survive a malformed script.
10. Bring up LCD and SD separately, then add them only after proven.

### USB recovery

Vendor instructs: to enter ROM download, **hold BOOT while inserting the device**. This is distinct from holding/pressing BOOT during normal firmware operation. Native USB-OTG HID mode can require returning to ROM download manually for flashing.

LILYGO reference: https://github.com/Xinyuan-LilyGO/T-Dongle-S3/blob/main/docs/en/t-dongle-s3/REAMDE.MD

## Acceptance ledger

| Gate | Evidence | Result |
| --- | --- | --- |
| Host C tests | Run `39131941468`, head `323c180`, including UBSan / -Werror | Pass |
| ESP-IDF native build | Same CI run; v5.5.3 ESP32-S3 artifact verified below | Pass |
| USB enumeration | Windows HID keyboard `303A:4004`, `CHN-S3-DEV`, status OK | Pass on this Windows host |
| No automatic injection | Dan: no text after normal reconnect with editor focused | Pass on first normal boot |
| BOOT press/release + arm expiry | Dan: yellow/amber then blue after requested >=35 s wait, no typing | Pass |
| Normal HID execution | Dan: exact `ChaoN USB HID smoke test`, green LED, no apparent sticky keys | Pass |
| Cancel / release | Dan reports requested cancellation and manual lowercase check behaved as expected | Pass by operator report |
| ROM recovery after HID boot | BOOT-held insertion restores COM4; read-only ROM command succeeds with same identity | Pass |
| Normal reconnect / return from ROM | Dan reports requested blue/no-typing reconnect and return-to-HID test behaved as expected | Pass by operator report |
| LED idle | Dan observed blue after normal boot | Pass |
| LCD, SD | Unimplemented / deferred | Not tested |

A passing source build does not close any physical gate.

## First physical session — 2026-10-19

### Candidate provenance

At flash preflight, PR #1 was draft and unmerged on `feat/esp-idf-foundation`, head
`323c180fe7f767f1cb21293f5adf4f08f945b73c`. Both jobs in CI run
`39131941468` passed. The workflow checked out PR merge ref
`289997932f1f017bbfd4cd791c9636798d53d795`; its tree matches the feature
head exactly. This accounts for the application's embedded version `2899979`.

Artifact `12137904869` (`chaon-esp32s3-candidate`) was downloaded using
authenticated GitHub CLI. Its ZIP SHA-256 matches GitHub's artifact digest:
`18a32badd9e41840a0157f618946987aae25b69b6cfbb0a0562c871b2349bcd9`.
No local rebuild or binary substitution was used.

`esptool v5.4.0 image-info` reports ESP32-S3, ESP-IDF v5.5.3, valid image
checksum/hash, and application length 224,144 bytes (`0x36b90`).
Application file SHA-256:
`bfb693d5b09a65cfb8c43d6accc613f5af0a782b15dc844ac394f84deb2a0d1d`.

The artifact's manifest specifies DIO / 80 MHz / 16 MB and only these images:

| Offset | Image | SHA-256 |
| --- | --- | --- |
| `0x0` | `bootloader/bootloader.bin` | `86fe794128763f51442cd2632ca75cd17478dc2f94266e47a7a2571f5ac0aa3c` |
| `0x8000` | `partition_table/partition-table.bin` | `7f00b6c042a89b15b0cac534f82ed988caf29278ff5700b0c511eb1b5bb7c820` |
| `0x10000` | `chaon.bin` | `bfb693d5b09a65cfb8c43d6accc613f5af0a782b15dc844ac394f84deb2a0d1d` |

### Hardware and recovery backup

Dan confirmed the original T-Dongle-S3. Controlled unplug/reconnect identified
COM4: it disappeared when only this dongle was unplugged and returned with the
same identity after BOOT-held insertion. COM25 and CH340 COM10 remained
connected and were not opened.

- Windows description: USB Serial Device (COM4), VID `303A`, PID `1001`.
- Parent instance: `USB\VID_303A&PID_1001\CC:BA:97:3A:4E:CC`.
- Interface instance: `USB\VID_303A&PID_1001&MI_00\9&11FBF42&0&0000`.
- ROM connection and RAM stub successful: ESP32-S3 QFN56 revision v0.2,
  MAC `cc:ba:97:3a:4e:cc`, 40 MHz crystal.
- Flash detected: 16 MB, manufacturer `ef`, device `4018`, quad data lines.
- Security report: flags zero, Secure Boot disabled, flash encryption disabled,
  `SPI_BOOT_CRYPT_CNT=0`.

The first full flash read completed successfully using esptool v5.4.0:

```powershell
esptool --chip esp32s3 --port COM4 --baud 460800 --before no-reset --after no-reset read-flash 0x0 0x1000000 'C:\Users\Daniel\Documents\ChaoN-USB-Recovery\2026-10-19-first-bringup\factory-ccba973a4ecc-16MB.bin'
```

Backup length: 16,777,216 bytes. SHA-256:
`19af13fad3becb094ac4422782b0f3c2a7a25851bcfaa0a69bb155b2ad542175`.
The backup and local command logs are outside the repository; factory flash
contents must never be committed or uploaded. A second independent full read
(`read-flash --no-progress 0x0 0x1000000`, output ending
`-second-read.bin`) completed with the same length and digest. The two complete
files were also compared byte for byte and matched.

### Flash and first enumeration

After the backup gate passed, these commands ran from the extracted artifact's
`build` directory, using esptool v5.4.0:

```powershell
esptool --chip esp32s3 --port COM4 --baud 460800 --before no-reset --after no-reset write-flash --no-progress --flash-mode dio --flash-freq 80m --flash-size 16MB 0x0 bootloader/bootloader.bin 0x10000 chaon.bin 0x8000 partition_table/partition-table.bin
esptool --chip esp32s3 --port COM4 --baud 460800 --before no-reset --after no-reset verify-flash --flash-mode dio --flash-freq 80m --flash-size 16MB 0x0 bootloader/bootloader.bin 0x10000 chaon.bin 0x8000 partition_table/partition-table.bin
```

Both commands exited successfully. Each image passed write-time digest
verification and the separate `verify-flash` check. The tool applied manifest
flash-header settings to the bootloader in memory; verification used identical
settings. No full-chip erase was performed. Only sectors covering the manifest
images were erased: `0x0–0x5fff`, `0x8000–0x8fff`, `0x10000–0x46fff`.

The device was deliberately left in download mode for verification, then Dan
was instructed to power-cycle with BOOT released per the vendor procedure.
Windows subsequently enumerated:

- `USB\VID_303A&PID_4004\CHN-S3-DEV`: USB Input Device, service `HidUsb`,
  bus-reported description `ChaoN USB HID`, status OK.
- `HID\VID_303A&PID_4004\9&20A0588E&0&0000`: HID Keyboard Device, status OK.
- The dongle's former COM4 interface is absent in HID mode, as expected.

Dan reported no automatic text and a blue LED on the first normal boot with
the ordinary editor focused. One BOOT press/release produced yellow/amber,
then blue after the requested >=35-second wait, with no typing. A fresh arm
and second press/release typed exactly `ChaoN USB HID smoke test`; Dan saw
green and reported no apparent sticky keys/modifiers. These are operator
observations, not instrumented USB report/timing captures.

After HID execution, BOOT-held insertion restored COM4 with the original
serial identity. The following read-only command succeeded directly against
ROM, with no stub upload, erase, or reflash, and reported the same revision,
MAC and security state:

```powershell
esptool --chip esp32s3 --port COM4 --before no-reset --after no-reset --no-stub get-security-info
```

Dan then followed the requested normal reconnect and cancellation sequence:
power-cycle with BOOT released into the focused ordinary editor, verify blue
and no automatic typing, arm, execute, immediately press/release a third time
during execution, and manually type lowercase letters to check key release.
His report was: "Done, everything works as expected." This closes those
smoke-test gates by operator report; the exact cancelled text prefix and LED
transition timings were not separately recorded. Final Windows enumeration
was checked again for the ChaoN HID keyboard.

No unexpected typing, enumeration fault, sticky keys, or other defect was
reported. No firmware change or repeat flash was needed. LCD and SD remain
unimplemented. Tests apply to this one Windows host; they do not establish
other OS behavior, current draw, USB suspend behavior, precise debounce
timing, or production readiness.

The flashed code and successful CI remain at head `323c180` (PR merge build
`2899979`). Subsequent first-bring-up documentation changes do not alter the
firmware and do not warrant a new firmware build. PR #1 remains draft and
unmerged; no release, tag, or publication was performed.


## Second physical session — LCD and LED validation, 2026-10-19

The first-session record above is historical evidence for baseline `323c180`.
This session tested the new LCD/LED image on the same original board and
Windows host. Dan explicitly authorized flashing after recovery/preflight gates.

### Provenance and recovery safeguards

- PR #6: `feat/status-led-lcd`, based on `feat/esp-idf-foundation`; both PR #6
  and foundation PR #1 remain draft/open/unmerged.
- Firmware source: `4d8d9e046d917baa9814d74cc24c8b346d55046d`.
  Native C tests (including visual tests/UBSan) and ESP-IDF v5.5.3 ESP32-S3
  build passed in run `39336336585`; no compiler warnings were found.
- CI merge build `7d502cf7d9927c188f6cf6bcecdb175e74bb83d8` has the same
  source tree as the firmware commit. The embedded app version is `7d502cf`.
- At preflight the branch head was `02db39223f81cd026f1ad2b8ceb213c1f88bb694`.
  Only README, roadmap and status-ui documentation differed from the validated
  firmware; no local rebuild or substituted image was used.
- Artifact `12227240519`, `chaon-esp32s3-candidate`, was downloaded with
  authenticated GitHub CLI. ZIP SHA-256:
  `124bcbc2e35b0ba415c2b16cea056e8c2fa2047563e41999c07bd54bb4eeb97e`.
- The original factory backup at
  `C:\Users\Daniel\Documents\ChaoN-USB-Recovery\2026-10-19-first-bringup\factory-ccba973a4ecc-16MB.bin`
  was rechecked without modification: 16,777,216 bytes, SHA-256
  `19af13fad3becb094ac4422782b0f3c2a7a25851bcfaa0a69bb155b2ad542175`.
- Preserved baseline `candidate-323c180.zip` in that directory also passed
  its original ZIP digest check
  (`18a32badd9e41840a0157f618946987aae25b69b6cfbb0a0562c871b2349bcd9`),
  retaining a verified rollback package. Factory contents were not uploaded.
- Rediscovered ROM port **COM4**, parent
  `USB\VID_303A&PID_1001\CC:BA:97:3A:4E:CC`, after BOOT-held insertion.
  The first insertion remained HID; a careful retry entered ROM, before any
  serial operation or flash write. esptool v5.4.0 confirmed ESP32-S3 QFN56
  revision v0.2, MAC `cc:ba:97:3a:4e:cc`, flash `ef/4018`, 16 MB quad.
  Secure Boot and flash encryption remained disabled; security flags were zero.
  COM25 and CH340 COM10 were not opened.

### Images, flash and separate verification

The app was 261,184 bytes (`0x3fc40`), ESP32-S3 / ESP-IDF v5.5.3,
with valid image checksum/hash. The verified artifact manifest specified
DIO, 80 MHz, 16 MB and only these images:

| Offset | File | SHA-256 |
| --- | --- | --- |
| `0x0` | `bootloader/bootloader.bin` | `48620ccd49db38d3a574feaf94b116796052237030f8336d2dd650e5402aa513` |
| `0x8000` | `partition_table/partition-table.bin` | `7f00b6c042a89b15b0cac534f82ed988caf29278ff5700b0c511eb1b5bb7c820` |
| `0x10000` | `chaon.bin` | `302bc54c076562571fa07b937537568ce67c39902de492ae988e124558eec9be` |

Commands ran with esptool v5.4.0 from the extracted artifact's `build`
directory, inside
`C:\Users\Daniel\Documents\ChaoN-USB-Recovery\2026-10-19-lcd-led-bringup\candidate-4d8d9e0`:

```powershell
esptool --chip esp32s3 --port COM4 --baud 460800 --before no-reset --after no-reset write-flash --no-progress --flash-mode dio --flash-freq 80m --flash-size 16MB 0x0 bootloader/bootloader.bin 0x10000 chaon.bin 0x8000 partition_table/partition-table.bin
esptool --chip esp32s3 --port COM4 --baud 460800 --before no-reset --after no-reset verify-flash --flash-mode dio --flash-freq 80m --flash-size 16MB 0x0 bootloader/bootloader.bin 0x10000 chaon.bin 0x8000 partition_table/partition-table.bin
```

Both commands exited zero. All three images passed write-time and separate
digest verification. Manifest flash-header settings were applied to the
bootloader in memory, with identical verification settings. Only image-covering
sectors were erased: `0x0–0x5fff`, `0x8000–0x8fff`,
`0x10000–0x4ffff`. No full-chip erase, unrelated writes, or eFuse writes.

### Operator-confirmed physical results

| Test | Actual evidence | Result |
| --- | --- | --- |
| A: normal boot | Windows ChaoN HID interface and keyboard child status OK; display "looks perfect", green LED; editor stayed empty | Pass |
| B: arm / expire | ARMED and flashing light; requested yellow/near-30-second/expiry recheck affirmed; specifically observed 20S decrement to 19S; returns green/READY without typing | Pass by operator report |
| C: execute | Exact `ChaoN USB HID smoke test`, no sticking; cyan/RUNNING then green/READY separately confirmed | Pass |
| D: cancel | Actual output prefix empty (zero characters); solid red/CANCELLED, return green; no sticking; overall requested hold/release-without-rearm sequence affirmed | Pass by operator report |
| E: armed disconnect | Reconnected to READY / BOOT TO ARM, steady green, empty editor | Pass |
| E: ROM recovery | BOOT-held insertion restored COM4 / original serial; direct read-only ROM command below succeeded with same chip identity | Pass |
| E: return to normal | Dan confirmed READY / BOOT TO ARM, steady green and no automatic typing after BOOT-released reconnect | Pass by operator report |

The initial ambiguous countdown description "205" was clarified by Dan as
`20S` changing to `19S`. The renderer uses two numeric digits plus an `S`
seconds suffix; no timer defect was established.

Test A's requested upright orientation, colors, readability and absence of
clipping were accepted by Dan's "looks perfect" report. This validates the
candidate MADCTL `0xA8`, inversion ON, x+1/y+26 address offsets, RGB565 output,
and active-low GPIO38 backlight configuration on this board qualitatively.
No photograph, calibrated color measurement, electrical trace, or first-frame
backlight timing capture was taken. Nominal yellow 250 ms on/off and red
2-second timing were not instrumented.

The cancellation test held BOOT until return to READY before releasing it.
No output characters were emitted in that trial; release did not produce a
reported accidental arm. Manual lowercase checks found no sticking. This is
host/operator evidence, not a capture of every HID release report.

ROM recovery used no stub and no reflash:

```powershell
esptool --chip esp32s3 --port COM4 --before no-reset --after no-reset --no-stub get-security-info
```

Normal USB identity remained `303A:4004` / `CHN-S3-DEV`, bus description
`ChaoN USB HID`, driver `HidUsb`, with a healthy HID Keyboard Device child.
COM4 disappears in HID mode as expected.

### Supplemental mid-stream cancellation observation — 2026-10-19

After the recorded cancellation trial that emitted zero characters, Dan supplied
a **separate manual mid-typing cancellation observation**. The focused editor
contained the exact partial output `ChaoN USB HI`. Dan confirmed this text was
produced while deliberately stopping the script **after typing had begun**.
This demonstrates that cancellation can interrupt an actively transmitting
script, not merely stop it before the first character. The rest of the fixed
demo string was not observed in that output.

This is operator-provided text evidence, not a raw USB HID report capture or
latency measurement. The earlier zero-character trial is retained above as a
distinct first cancellation test. No firmware change, new CI run, or reflash
is implied by this supplemental observation.

### Closeout and limits

No confirmed in-scope defect, firmware correction, or repeat flash was needed.
The firmware/last successful code CI remains `4d8d9e0` / run `39336336585`;
subsequent hardware-result documentation is metadata only and skips CI.
The first-session factory backup and baseline package remain preserved.
Local session logs and image digests are in
`C:\Users\Daniel\Documents\ChaoN-USB-Recovery\2026-10-19-lcd-led-bringup`.

Only this original board and Windows host were tested. FAULT blinking and
LCD-failure fallback were not fault-injected; other OS hosts, USB suspend,
current draw, endurance, and instrumented timing remain unvalidated.
microSD scripting remains unimplemented. Both PRs remain draft/unmerged.
No merge, tag, release, publication, extra feature, or unrelated cleanup.

## Third physical session — post-merge hardware regression, 2026-10-19

This section tests the merged BOOT-controller firmware. The first- and
second-session records above remain historical evidence for their original
firmware versions and at-the-time PR states. PRs #1, #6 and licensing PR #7
were already merged before this session.

### Identity, authorization and provenance

- Live repository: public, AGPL-3.0-only, default branch `main`, head
  `679ded5b2f88e685099e24040fe727c8911a8868`.
- Firmware source: `fb85ce8596c25c9749120c8678cf3da21e70f411`.
  Comparing this commit to main found only LICENSE, README, CONTRIBUTING,
  branding and roadmap changes; no runtime source or build-setting changes.
- CI run `39343230416`: native C tests/UBSan and ESP-IDF v5.5.3 ESP32-S3
  jobs successful. No compiler warnings found in the reviewed log.
- CI checked out merge ref `78e9bf62467e4d9a39914585580bd8efb2fb5402`,
  whose tree `7bd14681b3618f859d123be2bf2e926ac11795e6` matches the firmware
  source tree. Embedded version `78e9bf6` is therefore expected.
- Artifact `12230365264`, `chaon-esp32s3-candidate`, downloaded using
  authenticated gh after Dan's explicit "Approve flashing" response.
  ZIP SHA-256 verified:
  `c11a3729b3d05e396e030e717998ab947a7b7907b9f1d6f71384dfb623732561`.
- Rediscovered **COM4**, USB `303A:1001`, parent
  `USB\VID_303A&PID_1001\CC:BA:97:3A:4E:CC`. esptool v5.4.0 confirmed
  ESP32-S3 QFN56 v0.2, MAC `cc:ba:97:3a:4e:cc`, flash `ef/4018`,
  actual 16 MB quad. Secure Boot and flash encryption disabled, flags zero.
  Original-board/no-PSRAM identity follows the established board
  specification; no new physical PSRAM measurement was performed.
  COM25 and CH340 COM10 were not opened.
- Factory backup at
  `C:\Users\Daniel\Documents\ChaoN-USB-Recovery\2026-10-19-first-bringup\factory-ccba973a4ecc-16MB.bin`
  rechecked: 16,777,216 bytes; SHA-256
  `19af13fad3becb094ac4422782b0f3c2a7a25851bcfaa0a69bb155b2ad542175`.
  It was not overwritten, moved or uploaded.
- Working rollback package `4d8d9e0` remains available at
  `C:\Users\Daniel\Documents\ChaoN-USB-Recovery\2026-10-19-lcd-led-bringup\candidate-4d8d9e0.zip`;
  its ZIP SHA-256 `124bcbc2e35b0ba415c2b16cea056e8c2fa2047563e41999c07bd54bb4eeb97e`
  matched. Local Git checkout was clean on `feat/esp-idf-foundation` at
  `e755932`; no operator changes or local source edits.

### Flash evidence

The verified artifact targets ESP32-S3 / ESP-IDF v5.5.3. Its application is
261,200 bytes (`0x3fc50`), with valid internal checksum/image hash.
The manifest specifies DIO, 80 MHz, 16 MB and only these files:

| Offset | Image | File SHA-256 |
| --- | --- | --- |
| `0x0` | `bootloader/bootloader.bin` | `461f2c06f44991a2afd9a411c3c641a3182be0da31ad78a34afb204055bcbd1c` |
| `0x8000` | `partition_table/partition-table.bin` | `7f00b6c042a89b15b0cac534f82ed988caf29278ff5700b0c511eb1b5bb7c820` |
| `0x10000` | `chaon.bin` | `39bbb75fedb29c6cde26b9aff76eb58a0c131056ac59e8fc90cc74f852f28c71` |

Commands executed from the extracted `candidate-fb85ce8/build` directory
with esptool v5.4.0:

```powershell
esptool --chip esp32s3 --port COM4 --baud 460800 --before no-reset --after no-reset write-flash --no-progress --flash-mode dio --flash-freq 80m --flash-size 16MB 0x0 bootloader/bootloader.bin 0x10000 chaon.bin 0x8000 partition_table/partition-table.bin
esptool --chip esp32s3 --port COM4 --baud 460800 --before no-reset --after no-reset verify-flash --flash-mode dio --flash-freq 80m --flash-size 16MB 0x0 bootloader/bootloader.bin 0x10000 chaon.bin 0x8000 partition_table/partition-table.bin
```

Both exited zero; all three images passed write-time and separate digest
verification. Bootloader header settings were applied in memory with identical
verification options. Only image-covering sectors were erased:
`0x0–0x5fff`, `0x8000–0x8fff`, `0x10000–0x4ffff`.
No full-chip erase, unrelated port access, eFuse/security change, local rebuild,
older artifact substitution or repeat flash.

### Interactive tests A–H

These observations apply to this board, Windows host, and firmware `fb85ce8`.
A short "Passed" reply affirms the specified expected checks; it is not an
instrumented measurement.

| Test | Result | Expected | Operator's reply | Actual evidence |
| --- | --- | --- | --- | --- |
| A | PASS | READY / BOOT TO ARM, steady green, editor empty | Passed | Operator affirmed requested observations |
| B | PASS | ARMED, flashing yellow, near-30S countdown, no typing; after >=35s green READY and empty editor | Passed | Operator affirmed requested observations |
| C | PASS | Exact sentence once; cyan/RUNNING then green/READY; no duplicate characters or stuck modifiers; manual lowercase check | ChaoN USB HID smoke test  /  / Passed | Exact sentence supplied and requested checks affirmed |
| D | PASS | Stop while characters actively appear, red/CANCELLED then green READY, held cancellation release does not rearm, no stuck keys | ChaoN USB HID smok /  / Passed. I stopped it just in time | Exact nonempty strict prefix supplied; all requested checks affirmed |
| E | PASS | BOOT-held power-on enters ROM without unexpected typing; cancellation release not an arm gesture | Done | Operator reported completion of requested no-text check; host shows COM4 303A:1001 / expected MAC serial, no dongle HID interface. Cancellation-release check separately affirmed in D. |
| F | PASS | Disconnect while ARMED; reconnect BOOT released -> READY / BOOT TO ARM, steady green, no typing, arm cleared | passed | Operator affirmed requested observations |
| G | PASS | 3 ordinary manual trials during countdown refresh; each press executes once, exact sentence, no missed press or unusual delay | Passed | Operator affirmed all three requested trials; no missed/delayed press or incorrect output reported |
| H | PASS | Read-only ROM identity check without reflash, then normal green READY/inert boot with no stuck keys | recovery connected; final normal boot: Passed | Direct ROM command exit 0, same MAC/revision; operator affirmed final normal-state checks |

Timing and evidence limits:

- **A:** Approximately 10-second wait requested; no instrumented timing.
- **B:** 35-second wait requested; exact countdown/blink timing not instrument-measured.
- **C:** Execution indicator observation qualitative.
- **D:** Manual timing, no latency measurement.
- **E:** Held for requested 2 seconds; no instrumented hold timing.
- **F:** Approximately 10-second post-reconnect wait requested.
- **G:** Qualitative manual check only; no button waveform, latency or worst-case timing measurement.
- **H:** Manual recovery and requested 10-second final wait; no instrumented timing.

The exact mid-stream cancellation output on this firmware was
`ChaoN USB HID smok`, a nonempty strict prefix of
`ChaoN USB HID smoke test`. This trial stopped active typing before completion;
neither the older firmware's observations nor the earlier zero-character
cancellation trial was used to claim this pass. The requested held cancellation
button release did not produce a reported extra arm.

Test G consisted of three ordinary manual trials around countdown digit
updates. No missed press, unusual delay, duplicate execution, or incorrect
output was reported. It does not establish a worst-case latency or minimum
button-pulse guarantee.

Test E's BOOT-held power-on produced the expected ROM recovery USB identity,
not normal HID execution. Test H independently verified direct read-only ROM
communication using:

```powershell
esptool --chip esp32s3 --port COM4 --before no-reset --after no-reset --no-stub get-security-info
```

It returned ESP32-S3 QFN56 v0.2, MAC `cc:ba:97:3a:4e:cc`, unchanged security
flags, and exit zero without uploading a stub or reflashing. After normal
reconnection, Windows again reported `ChaoN USB HID`, `303A:4004`,
`CHN-S3-DEV`, a healthy HID interface and keyboard child; Dan affirmed green
READY / BOOT TO ARM, no typing and no stuck keys.

### Disposition

- **CI-verified:** the exact artifact's native C/UBSan and ESP32-S3 builds.
- **Tool-verified:** GitHub provenance, ZIP/image digests, chip/flash identity,
  separate flash verification, USB enumeration, backup integrity and ROM access.
- **Operator-observed:** the A–H behavior and exact cancellation prefix above.
- **Instrument-measured:** none of the physical button/LED/HID timing.
- **Not tested in this session:** electrical timing, calibrated colors/current,
  FAULT/LCD-failure injection, USB suspend, endurance and other OS hosts.

No confirmed defect, code correction or additional flash was required.
Factory backup digest was unchanged at closeout. The local checkout remained
clean; only this new documentation session is proposed through a separate
reviewable branch/PR. The first two sessions above were preserved intact.

Local artifact, image digests, commands, flash/ROM logs and observation record:
`C:\Users\Daniel\Documents\ChaoN-USB-Recovery\2026-10-19-post-merge-regression`.

**Recommendation:** ready for the next separately scoped development phase on
this tested board/Windows baseline; this is not a production-readiness or
cross-platform claim. Stop after validation/documentation. No microSD work,
new scripting, USB behavior change, merge, tag or production release.


## Fourth physical session — Phase 2A LCD SD probe, 2026-10-19

This session supersedes the earlier UART diagnostic blocker for the updated
candidate only. Dan has no UART adapter; the ST7735 footer supplied stage
visibility. The original 891b21b candidate was not flashed. Earlier sessions
above remain intact.

### Provenance, authorization and flash

- Tested firmware source: **f22f18e4e95c77aa1eae086944f4886793a05cef**,
  draft/open PR #9, feat/sd-readonly-probe; main baseline
  467e354fc2c2bfe09cd92bfc470d915ed51d05b8.
- [CI 39352112253](https://github.com/thetechbenders/chaon-usb/actions/runs/39352112253)
  passed native -Werror tests, UBSan and ESP-IDF v5.5.3 / ESP32-S3.
  Reviewed logs/annotations had zero compiler/Actions warnings or failures.
- CI merge build 76a259b79204f5efd08812fd50ee4634e2d7d770 shares tree
  e590b5d442e75cbd7b0d69259f1e096ee927654f with source; embedded version
  76a259b is expected.
- Artifact **12232863952**, chaon-esp32s3-candidate, ZIP SHA-256:
  `9ce9a822f134becd3fd68837d7b086039141ff96adde3255b7fecca51cd582c1`.
  App 294640 bytes, SHA-256
  `09bcc0b18603d36f5e16395da521bc536bfeae1a292c03c0bbf6f0f858eb1d14`.
  Manifest/extracted images byte-equal to verified ZIP; no local rebuild.
- Dan explicitly authorized this updated artifact: "Flash it".
  Rediscovered ROM COM4 / USB 303A:1001 / MAC cc:ba:97:3a:4e:cc.
  esptool v5.4.0 confirmed ESP32-S3 QFN56 v0.2, flash ef/4018 / 16 MB,
  security flags zero, Secure Boot/encryption disabled. Other ports untouched.
- Factory backup reverified unchanged: 16777216 bytes, SHA-256
  `19af13fad3becb094ac4422782b0f3c2a7a25851bcfaa0a69bb155b2ad542175`.
  Working fb85ce8 rollback ZIP reverified:
  `c11a3729b3d05e396e030e717998ab947a7b7907b9f1d6f71384dfb623732561`.
  No backup contents uploaded or replaced.
- Only manifest images written, DIO/80MHz/16MB: 0x0 bootloader,
  0x8000 partition table, 0x10000 app. Write and separate verify-flash
  exited zero; all three digests matched. Only image-covering sectors
  0x0–0x5fff, 0x8000–0x8fff, 0x10000–0x57fff erased.
  No full-chip erase, eFuse/security change or additional flash.

Bootloader SHA-256:
`4fac6f12f2b8e10a5f09e0d8b93872370db660dce29697ab0dddd9fdc1569758`.
Partition SHA-256:
`7f00b6c042a89b15b0cac534f82ed988caf29278ff5700b0c511eb1b5bb7c820`.

Commands from the extracted candidate's build directory:

```powershell
esptool --chip esp32s3 --port COM4 --baud 460800 --before no-reset --after no-reset write-flash --no-progress --flash-mode dio --flash-freq 80m --flash-size 16MB 0x0 bootloader/bootloader.bin 0x10000 chaon.bin 0x8000 partition_table/partition-table.bin
esptool --chip esp32s3 --port COM4 --baud 460800 --before no-reset --after no-reset verify-flash --flash-mode dio --flash-freq 80m --flash-size 16MB 0x0 bootloader/bootloader.bin 0x10000 chaon.bin 0x8000 partition_table/partition-table.bin
```

### Physical test results and evidence boundaries

| Test | Result | Actual evidence |
| --- | --- | --- |
| A: inserted-card boot/read | PASS for initialization/read and normal UI; card identity details NOT TESTED | Dan reported "SD Okay", then affirmed green READY / BOOT TO ARM, empty editor, readable unclipped footer and no resets/freezes. Windows ChaoN HID interface/keyboard 303A:4004 / CHN-S3-DEV status OK. |
| B: read-only guarantee | PASS static inspection; byte-for-byte immutability NOT TESTED | Probe/call-site inspection found no sector writes, mount, formatting, file changes or card-derived HID. No external reader or before/after whole-card hash comparison. |
| C: HID under SD observer | PASS by operator report | Arm/countdown/expiry without typing; cyan/RUNNING -> green READY/SD OK; exact full text ChaoN USB HID smoke test. Mid-stream cancellation left exact prefix ChaoN USB HI; red/CANCELLED -> READY, no rearm on release, stuck keys, missed input, unusual delay or freezes reported. |
| D: powered removal/reinsertion | NOT APPLICABLE (mechanically inaccessible) | Dan reported the card is physically inaccessible while USB is plugged in. No powered card change was attempted or required for this physical setup. Cold boots do not prove hot-swap or same-runtime host reinitialization; those would require a separately approved access fixture. |
| E: empty-slot boot/HID and cold restore | PASS for observed behavior; resource measurements NOT TESTED | "SD ERR C everything else is normal". Empty-slot arm/countdown/execution typed exact ChaoN USB HID smoke test; everything worked as expected. C briefly flashed to unreadable yellow text; INIT is expected by source but its text was not observed. Unpowered card restore returned "SD Ok", with requested normal-state checks affirmed; restored-card arm expiry without typing separately confirmed at closeout. |
| F: ROM recovery and final normal boot | PASS | Rediscovered COM4 / expected 303A:1001 MAC serial; direct ROM --no-stub get-security-info exited zero, ESP32-S3 QFN56 v0.2, same MAC and unchanged security. No reflash. Dan confirmed final SD OK, green READY / BOOT TO ARM, no typing/stuck keys or freeze/reset; Windows HID/keyboard status OK. |

The full output and nonempty cancellation prefix were supplied directly by
Dan. Requested checks affirmed by "Passed" remain operator observations,
not USB report traces or latency measurements. Near-30-second countdown,
>=35-second expiry and observation waits were requested, not instrument-timed.

SD OK means the source path initialized the card, accepted a 512-byte sector
size and successfully read sector zero. Actual capacity, SDSC type, card CID,
filesystem/sector classification and raw ESP-IDF errors were not visible.
"128 MB" is operator-reported; no capacity or filesystem claim follows.
SD ERR C deliberately combines unavailable/card-initialization failure;
empty-slot observation does not create a general absence/failure distinction.
No SD IO S or other injected error code was physically exercised.

No CPU/memory/log-volume instrumentation, card-bus capture, full-media hash,
pullup measurement, electrical timing, long-duration endurance, other-OS,
FAULT/LCD-failure injection. No UART logs were captured. Powered hot-swap is
**NOT APPLICABLE** to the installed-card/power-off card-change workflow because
the card slot is inaccessible while the dongle is plugged into USB. It is not
a failed test or a blocker for this hardware workflow. A separately approved
fixture could investigate powered removal if that ever becomes a requirement.
No confirmed defect or firmware correction was required by observed tests.
**Validation disposition:** the SDMMC probe passed the applicable inserted-card,
empty-slot, cold restoration, HID and ROM-recovery hardware smoke tests. The
rest of Phase 2A (filesystem and browsing) has not yet been implemented;
whole-card hashing, other-host testing and instrumented timings remain open.

Local packages, image digests, flash/verification/recovery logs and operator
observations are retained outside Git in
`C:\Users\Daniel\Documents\ChaoN-USB-Recovery\2026-10-19-sd-lcd-diagnostics`.
Factory contents are excluded. Documentation follow-up does not change tested
firmware f22f18e or warrant a new build. PR #9 stays draft/unmerged; no
filesystem browsing, SD execution, USB changes, tag or release.
