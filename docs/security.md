# Safety and authorization

ChaoN is a USB HID engineering instrument, intended for **owned or explicitly authorized** computers. HID devices send key events to whichever application has focus; the device cannot identify the foreground window or the human intent behind it.

## Deliberate execution policy

- Power-on / USB insertion alone must not type.
- The v0 demo requires two discrete physical button press-and-release gestures: arm, then execute.
- The arm state expires after 30 seconds.
- Physical button press while a script runs requests cancellation.
- An interrupted/invalid script must attempt to release all HID keys.
- No network-triggered script execution exists in the initial scope.
- The current fixed demo contains only plain printable text and **no Enter** to avoid accidental execution if a terminal has focus. Still, execute only with a safe editor focused.

## Security limitations

- This is an engineering prototype, not a tamper-resistant security product.
- USB HID enumeration can still cause endpoint-security warnings; policies vary.
- Nothing in firmware can guarantee focus or prohibit misinterpretation by the host.
- USB unplug or brownout while a key is held is a host-visible interruption; the host normally releases keys on disconnect, but that behavior is outside our control.
- GPIO0 is a boot strap; cannot be used to override the ROM download behavior when power is applied.
- Readable scripts / unsigned firmware / unverified OTA would be a threat if introduced later. Do not ship those as secure by assertion.

No credential collection, persistence, evasion, or unattended keystroke injection is part of the initial architecture.

Before release: decide authentication/trust for any script ingestion, signed firmware policy, update/recovery flow, USB VID/PID assignment, and a threat model suitable for a USB interface that host computers inherently trust as input.


## Phase 2C read-only inspection

The development inspector only validates at most 2047 bytes from one
FAT32 root `.CHN` file after rechecking the raw directory-entry identity
and bounded cluster chain. It returns a small diagnostic summary to the
LCD and wipes private source/parsed temporary memory after validation.
It does not publish script text or commands to `app_main`, the BOOT
controller, USB descriptors, HID mapping or the VM. A validated file is
**not trusted or executable**. The only HID demo is the preexisting fixed
string requiring two distinct short physical presses outside browsing.
No card writes or FAT filesystem mounts are performed.

This firmware extension is a draft and has not passed physical Phase 2C
testing on the original board. It is not a production safety claim.
