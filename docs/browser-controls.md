# Phase 2B: single-button read-only file selection

**Hardware smoke-tested development feature on the original T-Dongle-S3/Windows/FAT32 setup (2026-10-20).** PR #13 was tested from source commit `bb715cf8d2bca18b406cb10727a1697ebeb54c81`. This is not production qualification or permission to execute SD files.

The original T-Dongle-S3 has one available control: BOOT / GPIO0.
The button is also a ROM bootstrap pin, so BOOT-held power-on continues to
enter ROM download mode. Do not attempt to change that hardware behavior.

## Contract

The control policy is implemented by the single portable BOOT state machine,
not inferred by a display handler. USB/HID and SD cannot bypass it.

| Context | Gesture (debounced) | Effect |
| --- | --- | --- |
| READY | Tap (under 900 ms) | ARM the existing fixed benign HID demo |
| READY + supported catalogue | Hold for 900 ms or more, release | Enter BROWSE, without ARM |
| READY + no catalogue | Hold for 900 ms or more, release | No action; never ARM |
| ARMED | Short tap (under 900 ms) | Execute **only** the existing fixed benign demo |
| ARMED | Hold for 900 ms or more, release | Disarm without execution |
| BROWSE | Short tap | Next of at most 8 root entries; clear PICK marker |
| BROWSE | Hold 900–2999 ms, release | Toggle PICK marker on current entry |
| BROWSE | Hold 3000 ms or longer, release | Exit BROWSE without ARM |
| BROWSE | 60 s inactivity | Exit, consume any held release |
| BROWSE | SD catalogue unavailable/count changed, USB disconnect | Exit, consume held release |

The timing is measured from debounced stable press to debounced stable
release at the existing 20 ms poll cadence; it is not a physical timing
guarantee. If a gesture straddles a USB disconnect, timer expiry, or SD
invalidation, it must not be repurposed as ARM.

**PICK is display-only.** It does not open any file, read file contents,
parse scripts, allocate a dynamic string, generate HID input, or confer
execution authority. The only event that starts the VM remains two
normal, separate short taps while not browsing; the VM continues to run
the embedded fixed text demonstration.

The SD task remains read-only and publishes a bounded catalogue snapshot.
The main task alone renders the LCD. BOOT controls are always serviced
before display I/O; rendering failure cannot authorize USB/HID input.

## Display and limitations

When browsing, the full 160×80 screen displays the current short DOS 8.3
root filename, `FILE` or `DIR`, index/count, `+` if the listing is
incomplete, and a distinct PICK marker. There is no cursor movement on
disk, filesystem recursion, file preview, on-device file reading, nor
selection persistence across power loss or card errors.

No more than eight root entries are available because that is the
Phase 2A bounded snapshot. The ninth entry is not reachable. Physical
FAT12/FAT16 and malformed/unsupported media remain outside previously
completed FAT32 hardware smoke-test coverage.

## Verification and hardware results

[Exact source-head CI run 39440312608](https://github.com/thetechbenders/chaon-usb/actions/runs/39440312608) passed seven native C test groups with `-Werror` and UBSan and built with ESP-IDF v5.5.3 for ESP32-S3, with zero reported compiler/Actions warnings. Host tests cover the 880/900 ms and 2980/3000 ms gesture boundaries, BOOT/HID regression, card/catalogue loss, USB disconnect/held-button reconnect, fault/timeout transitions, and renderer bounds. No host test replaces the physical gesture and HID checks.

Dan explicitly authorized flashing the exact candidate. Artifact `12274550501` (`chaon-esp32s3-candidate`) ZIP SHA-256:
`e2f8c1f68a65d7ff25b10a2d05a4e8322ce563fa67affd09c32b3bee18376a34`.
An independent binary-stream download matched the GitHub digest. The CI merge checkout `015dc0686a15a4a3f3bf8d2e49ef9374de119de1` and source head `bb715cf8` share Git tree `0768d779924c5301f799108ed67125f97067440a`. Embedded `015dc06` version metadata therefore identifies the same tested source tree. The original ESP32-S3 v0.2, 16 MB board was identified through ROM COM3; image-specific flash and separate verify both exited zero for bootloader, partition table and application. The previously tested Phase 2A image package was rehashed as a rollback candidate; the factory full-flash backup was not present on this computer.

**Physical smoke tests A–F: PASS** on the original T-Dongle-S3, Windows host, and existing 120 MiB FAT32 card:

- READY startup, normal LED/LCD, automatic filename rotation, healthy USB HID and no unsolicited typing
- First-file view, FILE/DIR, index/count and `+` truncation, short-tap advance/wrap, PICKED toggle/clear, navigation clearing selection, long-hold exit, with no ARM or typing while browsing
- Existing fixed text HID demo: 30-second arm expiry, long-hold disarm without typing, two-short-tap intentional single exact `ChaoN USB HID smoke test`, active cancellation (reported partial text `ChaoN USB HID`), no stuck keys or spontaneous rearm
- Power-off card removal: `SD ERR C`, absent-catalogue long hold inert; power-off card restore returned the file preview/browser
- Approximately 65-second no-input wait exited browse to READY; BOOT-held ROM entry and read-only no-stub recovery probe succeeded; subsequent normal boot returned to a responsive, inert device

The pass disposition combines separate tool-verified flash/ROM enumeration and the operator's qualitative observations. **Not tested:** instrument-calibrated button/HID timings, HID traces, full-card before/after byte immutability, electrical/current/resource/endurance measurements, other hosts and filesystems. Powered card hot-swap is **NOT APPLICABLE** because the slot is inaccessible while USB is connected. Static read-only design does not prove byte-for-byte media immutability.

See the [sixth chronological bring-up session](bringup.md#sixth-physical-session--phase-2b-button-browser-2026-10-20). The complete original report and logs remain in the operator's local `ChaoN-USB-Recovery/2026-10-20-110849-phase2b-browser/` folder, not checked into this public repository. No code or firmware was changed during the physical test. No production release, tag or deployment is implied.


## Phase 2C selected-file diagnostics (draft, not physically validated)

Picking a file additionally requests **read-only inspection** of a
root-level FAT32 `.CHN` file in the optional SD task. The LCD shows
only validation status, instruction count/first opcode, or the first
error-line number. Picking a non-script/directory gives a bounded
diagnostic or no valid request. Navigating or exiting invalidates any
outstanding result. No source text or executable script crosses into
the browser, BOOT controller, HID VM or USB stack.

The timings and gesture meanings in the Phase 2B table are unchanged.
Successful validation confers no execution permission. See
[script-inspection.md](script-inspection.md) for the Phase 2C boundary.
