# Phase 2B: single-button read-only file selection

**Development candidate; not hardware validated, merged or flashed.**

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

## Validation gates before merge

Portable tests must demonstrate short/long ambiguity does not execute
unexpectedly, normal demo expiry/cancellation remain correct, browser
cannot EXECUTE, USB disconnect and card invalidation fail closed,
bounds/guarded pixel rendering, and BOOT-held reconnect suppression.
Run native -Werror and UBSan plus ESP-IDF v5.5.3 esp32s3 CI.

Before any physical flash: obtain separate owner approval and verify
known-good rollback and artifact provenance; then test with the
disposable FAT32 card. Never attempt powered SD hot-swap: the card is
physically inaccessible when the USB-A dongle is inserted.
