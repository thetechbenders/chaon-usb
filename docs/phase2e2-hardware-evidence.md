# Phase 2E.2 hardware evidence ledger

Status: **HARDWARE ACCEPTED WITH DOCUMENTED PHYSICAL LIMITATIONS**. The exact
frozen candidate was flashed and independently read back; all feasible
original-board checks passed, while fault-injection cases requiring powered
media access or an interposer remain explicitly BLOCKED.

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
| Explicit approval text/time | `Flash it it's in rom`; recorded `2026-10-24T17:33:16.531-04:00` |
| Rediscovered ROM port | PASS — exact `303A:1001` parent and interface rediscovered on COM4 immediately before write |
| Pre-write image hash recheck | PASS — artifact, three candidate images, accepted rollback ZIP and both private factory backups matched their recorded SHA-256 values |
| Write command/log | PASS — manifest regions only, no full-chip erase, `--before no-reset --after no-reset --no-stub`; local `flash-write.log` |
| Separate verify-flash/log | PASS — all three regions independently verified; local `flash-verify.log` |
| Bootloader readback/hash | PASS — 20,832 bytes, byte-identical, `82c19ce825793da7df337ebe59e4cbef057ab76d112316552349094044bd05e2` |
| Partition readback/hash | PASS — 3,072 bytes, byte-identical, `7f00b6c042a89b15b0cac534f82ed988caf29278ff5700b0c511eb1b5bb7c820` |
| Application readback/hash | PASS — 325,008 bytes, byte-identical, `2ee7a7972b23783e394ace8aef65377781fca9cece434156e3b775533e6abba6` |
| Normal-mode enumeration | PASS — `303A:4004` / `CHN-S3-DEV`, healthy HID input and keyboard interfaces, no ROM device and no ChaoN COM port |
| Flash disposition | FLASH VERIFIED; final board state is healthy normal HID mode |

## Physical acceptance results

Use PASS only for a directly observed expected result. Use FAIL for a contrary
observation, BLOCKED when required equipment/timing is unavailable, and NOT
RUN when no attempt occurred.

| ID | Test | Setup / expected result | Observed result | Status | Evidence |
| ---: | --- | --- | --- | --- | --- |
| 1 | Clean boot / enumeration / inertness | BOOT released; `303A:4004` / `CHN-S3-DEV`, READY, normal LCD/LED, empty editor | Dan reported READY and no unsolicited typing; Windows independently confirmed healthy HID identity and no COM port | PASS | Local enumeration log; operator report |
| 2 | Embedded demo | No selection; two distinct short gestures; exact fixed demo once | First gesture armed without output; second typed exactly `ChaoN USB HID smoke test`; READY returned with no stuck key | PASS | Operator report |
| 3 | Browser and valid PICK | Navigate to `BENIGN.CHN`; valid PICK; no typing | Browsed to and selected `BENIGN.CHN`; output occurred only after later deliberate ARM/RUN | PASS | Operator report |
| 4 | Selected filename persistence | `BENIGN.CHN` remains visible in READY and ARMED; no catalogue overwrite | Filename remained through two catalogue intervals and in ARMED | PASS | Operator report |
| 5 | Separate ARM / RUN | First gesture arms only; full release; later gesture runs | Deliberate separate ARM then RUN confirmed | PASS | Operator report |
| 6 | Benign SD execution | Fresh verify; exact fixture text/TAB/ENTER once; no demo | Emitted exact `ChaoN 2E2 A<TAB>B` fixture output; no unsolicited run | PASS | Operator report; fixture hash |
| 7 | Same-size source mutation | Preserve raw identity, change A bytes to B; reject with no output | Live raw identity cannot be preserved while modifying the inaccessible powered card; no approved interposer | BLOCKED | Hardware geometry; native regression only |
| 8a | SD removal in READY | Revoke selection; no output/resume | Powered removal inaccessible. Cold substitute passed: expected SD error/no typing; restoration returned READY/catalogue inertly | NOT APPLICABLE (hot); COLD PASS | Operator report |
| 8b | SD removal in ARMED | Revoke grant; no output/resume | Powered slot inaccessible; USB-disconnect-while-ARMED revocation passed in test 10 | NOT APPLICABLE | Hardware geometry; test 10 |
| 8c | SD removal during verification | Catch active verification; fail closed | Powered slot inaccessible; no approved timing/interposer fixture | BLOCKED | Hardware geometry |
| 9a | BOOT during verification | Catch active verification; cancel with no output/fallback | Verification completes too quickly for reproducible unaided intervention | BLOCKED | Native regression only |
| 9b | BOOT during execution | Cancel selected execution; no continuation or held key | Cancelled `KEYREL.CHN` during delay; `END` suppressed, READY returned, manual typing normal | PASS | Operator report |
| 10 | USB epoch invalidation | Disconnect while authorized; reconnect inert and unselected | Disconnect while selected/ARMED returned unselected READY with no resume or typing | PASS | Operator report |
| 11 | 30-second ARM expiry | Wait at least 35 s; READY and no output | Returned READY after 35 s with no output | PASS | Operator report |
| 12a | Verification timeout | Hold read beyond five seconds; fail closed | No approved fault fixture can hold a live read beyond five seconds | BLOCKED | Native regression only |
| 12b | SD I/O failure | Inject read failure; fail closed | Cold loss passed, but live verification I/O failure cannot be injected | BLOCKED | Native regression; cold substitute |
| 13 | Invalid / empty / oversized / non-CHN | All reject; no selected ARM or HID | Malformed file: `SYNTAX LINE 2`; zero-byte and 2,048-byte files: intentional `TOO LARGE`; TXT: `NOT CHN`; no HID | PASS | Operator report; card hashes; native zero-size assertion |
| 14 | No demo fallback | Rejected selected RUN emits neither selected nor demo text | USB invalidation paths and rejected PICK paths emitted no selected continuation or demo; dedicated last-moment verification rejection cannot be injected | PARTIAL; STRONG CASE BLOCKED | Tests 10, 13, 16b; native regression |
| 15 | Single-use authority | No automatic repeat; one tap only arms; a new ARM/RUN may reuse persistent selection with new authority | Completed run did not repeat; one later tap only armed; deliberate disarm emitted nothing; separate two-gesture reruns succeeded | PASS | Operator report |
| 16a | BOOT cancel / key release | Cancel active `KEYREL.CHN`; no sticky modifier | Cancellation suppressed delayed `END`; READY returned; manual typing showed no stuck key | PASS | Operator report |
| 16b | Transport loss | Disconnect during active typing; no resume/held-key symptom | Unplug after `START` suppressed `END`; reconnect emitted nothing, returned READY and had no stuck key | PASS | Operator report; host enumeration |
| 17a | BOOT-held ROM recovery | Rediscover expected ROM identity after candidate tests | `303A:1001` / COM4; ESP32-S3 QFN56 rev v0.2, expected MAC and 16 MB; final reconnect returned healthy HID/READY inertly | PASS | Local ROM and enumeration logs; operator report |
| 17b | Accepted-firmware restoration | If separately authorized, restore/readback Phase 2E.1 and return inert READY | No rollback-triggering failure; accepted artifact remains hash-verified and local | NOT REQUIRED | Preflight rollback evidence |

## Evidence locations

Local package:
`C:\Users\Daniel\Documents\Coding\choan-usb-hardware-preflight-2026-10-21\phase2e2-ae70e489-2026-10-24`.
It contains the verified archive, extracted artifact, read-only ROM logs and
host/device inventory. Private factory images remain only in their existing
recovery directory.

## Current disposition

**HARDWARE ACCEPTED WITH DOCUMENTED PHYSICAL LIMITATIONS.**

The exact frozen candidate was flashed only at manifest offsets and independent
ROM readback matched all three approved source images byte-for-byte. Physical
counts are 14 PASS, 0 FAIL, 5 BLOCKED, 1 PARTIAL/strong-case BLOCKED, 2 NOT
APPLICABLE and 1 NOT REQUIRED, plus a passing cold-card substitute. Blocked
cases require powered media access or a controlled SD fault/interposer and
retain separate native evidence. Controlled fixture-only SD changes were
backed up and hash-verified. Final state is normal `303A:4004` /
`CHN-S3-DEV`, READY/catalogue, with no automatic typing. No full-chip erase,
eFuse change, rollback, merge, tag or release occurred; PR #18 remains draft.
