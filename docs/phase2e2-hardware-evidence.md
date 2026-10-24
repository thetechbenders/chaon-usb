# Phase 2E.2 hardware evidence ledger

Status: **PREFLIGHT COMPLETE; AWAITING EXPLICIT FLASH APPROVAL**. No candidate
flash, SD-card write, or physical acceptance test was performed while creating
this ledger.

## Session identity

| Field | Value |
| --- | --- |
| Prepared | 2026-10-24, America/New_York |
| Coordinator | Codex on Dan's Windows gaming rig |
| Physical operator | Dan (required for visual, BOOT, card and host observations) |
| Product PR | #18, draft, stacked on `feat/phase2e-execution-authority-gate` |
| Validation-package branch state | Documentation/fixture-only descendant of the frozen firmware source |
| Frozen firmware source | `ae70e489ccd7ca22e2dcbe466bc5f544940a55f7` |
| Accepted baseline | `ba999824f1189747cdb465be56aab98bea58add3` |
| CI evidence | run `40131847526`, both jobs successful |
| Artifact | `12573854980`, `chaon-esp32s3-candidate`, 1,988,175-byte ZIP |
| Artifact SHA-256 | `abe10ebffa2d2949de1fb860b41f701097f930e65c86d45b7eb0e946183d69c7` |
| Test plan | `docs/phase2e2-hardware-validation.md` |
| Fixtures | `tests/hardware/phase2e2/fixtures` |

CI run `40131847526` belongs to the CI-only draft PR #19 against `main`.
Its synthetic merge `f500be1a1ae2cf13017847601676254a31f819a1` has Git tree
`43f06b0d8987afad8877377071c19ed0cc3b8fd9`, exactly equal to frozen
source `ae70e489...`. The artifact is therefore accepted as a build of the
frozen source content; no later PR artifact may be substituted.

## Preflight evidence

| Gate | Observed evidence | Status |
| --- | --- | --- |
| Branch | Worktree began clean at `ae70e489...`; the later package commit changes only docs, fixtures and their byte-preservation attribute | PASS |
| Product PR | #18 remains OPEN/DRAFT on Phase 2E.1; its frozen firmware ancestor is `ae70e489...` | PASS |
| CI | Native and ESP-IDF v5.5.3 ESP32-S3 jobs completed successfully | PASS |
| Artifact API | ID, name, run, head SHA, size and published digest match the handoff | PASS |
| Independent ZIP hash | Downloaded archive SHA-256 equals `abe10ebf...` | PASS |
| Manifest | ESP32-S3, DIO, 80 MHz, 16 MB; `0x0` / `0x8000` / `0x10000` | PASS |
| Image integrity | esptool image-info reports valid ESP32-S3 checksums/hashes and ESP-IDF v5.5.3 | PASS |
| Partition fit | App `0x4f590`; 1 MiB app partition; 69% free | PASS |
| Accepted rollback | Local artifact `12542959189` rehashed to published `450ae893...` | PASS |
| Factory backups | Two local 16,777,216-byte reads both rehash to `19af13fa...` | PASS |
| ROM USB identity | `USB\VID_303A&PID_1001\CC:BA:97:3A:4E:CC`; COM4 rediscovered | PASS |
| Chip identity | ESP32-S3 QFN56 rev v0.2, MAC `CC:BA:97:3A:4E:CC`, 40 MHz crystal | PASS |
| Flash identity | `ef/4018`, 16 MB, quad, 3.3 V | PASS |
| Security state | Flags zero; Secure Boot and flash encryption disabled | PASS |
| Port availability | Three no-reset/no-stub read-only esptool sessions opened COM4 and exited zero | PASS |
| Fixture parser check | Production parser, GCC 15.2, `-Werror` and UBSan; all nine fixture cases passed | PASS |

### Candidate files

| File | Bytes | SHA-256 | Status |
| --- | ---: | --- | --- |
| Artifact ZIP | 1,988,175 | `abe10ebffa2d2949de1fb860b41f701097f930e65c86d45b7eb0e946183d69c7` | PASS |
| `bootloader/bootloader.bin` | 20,832 | `82c19ce825793da7df337ebe59e4cbef057ab76d112316552349094044bd05e2` | PASS |
| `partition_table/partition-table.bin` | 3,072 | `7f00b6c042a89b15b0cac534f82ed988caf29278ff5700b0c511eb1b5bb7c820` | PASS |
| `chaon.bin` | 325,008 | `2ee7a7972b23783e394ace8aef65377781fca9cece434156e3b775533e6abba6` | PASS |
| `chaon.elf` | 5,041,224 | `000c5316f43ddc74c7007e6e62f821d0a52d82490100c95455db5bac51b68b2e` | PASS |
| `flash_args` | 143 | `894b9d8fb1046ffdd1e163eb0e82935d9d8fbb31ab74aac284426138dc9456a2` | PASS |
| `flasher_args.json` | 903 | `82365565ecb2a079c0304e5512abeb5f2356788348b7dc110080aea359bc06bd` | PASS |
| `dependencies.lock` | 1,088 | `736e48ae469767afdb881fa5ed6443d2275cd9350751e2f65ff44af373340afa` | PASS |

### Recovery material

| Material | Bytes | SHA-256 | Status |
| --- | ---: | --- | --- |
| Accepted Phase 2E.1 artifact ZIP `12542959189` | 1,967,598 | `450ae8934a6e19ca27e316c690cb14cba98c83cb234d1da7c17bc9a2f7d0e7eb` | PASS |
| Accepted bootloader | 20,832 | `6b3614fa0cee8e455552ca3f5fc3f070061ba90790b1fbb31b7eeb9cfc38e508` | PASS |
| Accepted partition table | 3,072 | `7f00b6c042a89b15b0cac534f82ed988caf29278ff5700b0c511eb1b5bb7c820` | PASS |
| Accepted application | 320,336 | `a659debfbef68d09b6af585b254f328b3b3ef380e5488c6e312b3f0ba0fe1dbc` | PASS |
| Factory backup, first read | 16,777,216 | `19af13fad3becb094ac4422782b0f3c2a7a25851bcfaa0a69bb155b2ad542175` | PASS |
| Factory backup, second read | 16,777,216 | `19af13fad3becb094ac4422782b0f3c2a7a25851bcfaa0a69bb155b2ad542175` | PASS |

Factory backup paths and contents remain local and are not committed or
uploaded.

## Flash-session record

| Field | Observation |
| --- | --- |
| Explicit approval text/time | NOT YET RECEIVED |
| Rediscovered ROM port | COM4 during preparation; must rediscover again |
| Pre-write image hash recheck | NOT RUN for flash session |
| Write command/log | NOT RUN |
| Separate verify-flash/log | NOT RUN |
| Bootloader readback/hash | NOT RUN |
| Partition readback/hash | NOT RUN |
| Application readback/hash | NOT RUN |
| Normal-mode enumeration | NOT RUN |
| Flash disposition | NOT FLASHED |

## Physical acceptance results

Use PASS only for a directly observed expected result. Use FAIL for a contrary
observation, BLOCKED when required equipment/timing is unavailable, and NOT
RUN when no attempt occurred.

| ID | Test | Setup / expected result | Observed result | Status | Evidence |
| ---: | --- | --- | --- | --- | --- |
| 1 | Clean boot / enumeration / inertness | BOOT released; `303A:4004` / `CHN-S3-DEV`, READY, normal LCD/LED, empty editor | Not run | NOT RUN | — |
| 2 | Embedded demo | No selection; two distinct short gestures; exact fixed demo once | Not run | NOT RUN | — |
| 3 | Browser and valid PICK | Navigate to `BENIGN.CHN`; valid PICK; no typing | Not run | NOT RUN | — |
| 4 | Selected filename persistence | `BENIGN.CHN` remains visible in READY and ARMED; no catalogue overwrite | Not run | NOT RUN | — |
| 5 | Separate ARM / RUN | First gesture arms only; full release; later gesture runs | Not run | NOT RUN | — |
| 6 | Benign SD execution | Fresh verify; exact fixture text/TAB/ENTER once; no demo | Not run | NOT RUN | — |
| 7 | Same-size source mutation | Preserve raw identity, change A bytes to B; reject with no output | Not run; specialized access may be required | NOT RUN | — |
| 8a | SD removal in READY | Revoke selection; no output/resume | Not run | NOT RUN | — |
| 8b | SD removal in ARMED | Revoke grant; no output/resume | Not run | NOT RUN | — |
| 8c | SD removal during verification | Catch active verification; fail closed | Not run; short timing window | NOT RUN | — |
| 9a | BOOT during verification | Catch active verification; cancel with no output/fallback | Not run; short timing window | NOT RUN | — |
| 9b | BOOT during execution | Cancel `SLOWRUN.CHN`; no continuation or held key | Not run | NOT RUN | — |
| 10 | USB epoch invalidation | Disconnect while authorized; reconnect inert and unselected | Not run | NOT RUN | — |
| 11 | 30-second ARM expiry | Wait at least 35 s; READY and no output | Not run | NOT RUN | — |
| 12a | Verification timeout | Hold read beyond five seconds; fail closed | Not run; fault fixture required | NOT RUN | — |
| 12b | SD I/O failure | Inject read failure; fail closed | Not run; fault fixture required | NOT RUN | — |
| 13 | Invalid / empty / oversized / non-CHN | All reject; no selected ARM or HID | Not run | NOT RUN | — |
| 14 | No demo fallback | Rejected selected RUN emits neither selected nor demo text | Not run | NOT RUN | — |
| 15 | Single-use authority | No automatic repeat; reselection required for selected rerun | Not run | NOT RUN | — |
| 16a | BOOT cancel / key release | Cancel active `KEYREL.CHN`; no sticky modifier | Not run | NOT RUN | — |
| 16b | Transport loss | Disconnect during active typing; no resume/held-key symptom | Not run | NOT RUN | — |
| 17a | BOOT-held ROM recovery | Rediscover expected ROM identity after candidate tests | Not run | NOT RUN | — |
| 17b | Accepted-firmware restoration | If separately authorized, restore/readback Phase 2E.1 and return inert READY | Not run | NOT RUN | — |

## Evidence locations

Local package:
`C:\Users\Daniel\Documents\Coding\choan-usb-hardware-preflight-2026-10-21\phase2e2-ae70e489-2026-10-24`.
It contains the verified archive, extracted artifact, read-only ROM logs and
host/device inventory. Private factory images remain only in their existing
recovery directory.

## Current disposition

**PREFLIGHT COMPLETE; AWAITING EXPLICIT FLASH APPROVAL.**

The device remains in ROM download mode after read-only checks. No flash write,
erase, eFuse operation, SD-card change, normal-mode execution, rollback, merge,
tag, or release occurred. Physical tests remain NOT RUN.
