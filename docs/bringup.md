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
