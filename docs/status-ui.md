# LED and LCD state indication (initial physical validation completed)

Target: **original** LILYGO T-Dongle-S3, not S3 Plus or C5. Baseline `323c180` passed the first HID bring-up. The native LCD/LED firmware at `4d8d9e0` passed exact-head CI and the agreed physical Windows smoke tests on 2026-10-19. The original factory backup and verified baseline rollback package are preserved. See [session 2 evidence](bringup.md#second-physical-session--lcd-and-led-validation-2026-10-19).

## Operator-visible contract

| State | APA102 LED | LCD (160×80 landscape) | Transition |
| --- | --- | --- | --- |
| No USB HID host | Dim green | USB OFFLINE / CONNECT USB | Auto when USB host not mounted |
| Ready (normal) | Steady ChaoN lime green (nominal #B4F66B) | READY / BOOT TO ARM | USB connection or successful execution |
| Armed | Flashing yellow, 250 ms ON / 250 ms OFF | ARMED / BOOT TO RUN + remaining seconds | First debounced press-release, expires after 30 s |
| Running | Steady cyan | RUNNING / BOOT TO STOP | Second distinct press-release |
| Cancelled | Solid red for 2 s | CANCELLED / KEYS RELEASED | BOOT press during execution; then returns to Ready |
| HID fault | Flashing red, 125 ms ON / 125 ms OFF | FAULT / CHECK LOGS | HID transport/runtime failure until USB disconnect |

These are the software state definitions. Initial physical observations on Dan's board confirmed green READY, yellow/flashing ARMED and expiry, cyan/RUNNING, red/CANCELLED, and return to READY. Exact 250 ms blink and 2-second cancellation durations were not instrumented; colors were judged visually. FAULT blinking was not fault-injected. The APA102 global brightness is fixed at 4/31.

**USB insertion stays inert.** The display/LED are indicators, not safety controls. BOOT remains a ROM strap at insertion and the user must hold it for download-mode recovery. The cancelled screen means the firmware attempted release-all; it does not prove host confirmation.

## Original-board LCD interface

Vendor's original-board LCD example is the source for the wiring and initial orientation. These settings produced operator-accepted output on the tested board:

- ST7735, 160x80, ESP-IDF native SPI2, 16 MHz
- MOSI GPIO3, SCLK GPIO5, CS GPIO4, D/C GPIO2, RESET GPIO1
- Backlight GPIO38, active LOW: initialize OFF, switch ON only after first successfully written frame
- Initial landscape MADCTL=0xA8, inversion ON, column offset +1, row offset +26, RGB565
- 25,600-byte aligned static RGB565 framebuffer (no PSRAM); redraw on status/countdown changes rather than every tick
- No Arduino, PlatformIO, copied product firmware, Wi-Fi, or SD card management

Dan accepted the display as upright, correctly colored, readable and unclipped ("looks perfect"). The backlight and first visible frame worked; precise first-frame enable timing was not measured. No LCD-failure fault injection was performed. Source handling logs LCD failure as `LCD_INIT_FAIL`/`LCD_WRITE_FAIL` and **does not disable HID or bypass the physical arm**; that failure path is not physically validated.

## Physical acceptance procedure

1. Verify current factory recovery backup is still present outside Git and unmodified. Do not re-read or overwrite it casually.
2. Flash only the chosen CI-verified branch artifact by the validated manifest. No full-chip erase, no automatic flash from CI.
3. Confirm idle on USB connection: lime green LED, READY/BOOT TO ARM text upright and un-clipped.
4. Press-release once: yellow flashes twice per second, ARMED countdown from 30 s; expiry returns lime green READY, no typing.
5. Arm again and execute with owned editor focused: cyan RUNNING; exact benign text; return green READY, no sticky keys.
6. Cancel mid-script: solid red CANCELLED for approximately 2 s; no unrequested arm on releasing the cancel button; return green READY.
7. Disconnect/reconnect while armed or running: reset to WAIT_USB then READY, never auto-run. Confirm fault red blink only for transport errors.
8. Verify LCD black-screen/reversed-polarity/orientation behavior, APA102 colors, RAM consumption, and whether LCD update causes missed BOOT interactions.
9. Do not assume passing CI verifies actual pixel ordering, display timing, or hardware.

This change does not implement SD script selection, keyboard-layout mapping beyond US, or signed/remote payloads.

## Physical results — 2026-10-19

- Artifact `12227240519`, ZIP SHA-256 `124bcbc2e35b0ba415c2b16cea056e8c2fa2047563e41999c07bd54bb4eeb97e`, firmware `4d8d9e0`, ESP-IDF v5.5.3. Flash and separate image verification passed on the rediscovered COM4 / MAC `cc:ba:97:3a:4e:cc`.
- READY: display accepted as correct; steady green, no automatic typing. ARMED: screen/countdown and flashing indication worked; recheck of near-30-second countdown/expiry accepted. Dan explicitly observed `20S` decrementing to `19S`; the seconds suffix initially resembled a 5.
- Execution: exact `ChaoN USB HID smoke test`, cyan/RUNNING, return green/READY, no sticky keys/modifiers.
- Cancellation: **empty output prefix** in the actual trial, solid red/CANCELLED, return green; requested hold-then-release behavior accepted with no accidental arm reported and no sticking.
- Disconnect while armed returned to READY / BOOT TO ARM with steady green and an empty editor. BOOT-held insertion restored ROM connectivity without reflashing; final normal reconnect confirmed READY/green/no typing. Windows HID enumeration remained healthy.
- No confirmed defect or firmware correction. FAULT/LCD-failure behavior, precise timings, calibrated colors, other OS hosts, power/current and endurance remain unvalidated. Full evidence and exact commands are in [bring-up](bringup.md). Both PRs stay draft/unmerged; no microSD development.
