# LED and LCD state indication (candidate, physical verification pending)

Target: **original** LILYGO T-Dongle-S3, not S3 Plus or C5. The previous known-good firmware at \`323c180\` remains the first physical bring-up baseline.

## Operator-visible contract

| State | APA102 LED | LCD (160×80 landscape) | Transition |
| --- | --- | --- | --- |
| No USB HID host | Dim green | USB OFFLINE / CONNECT USB | Auto when USB host not mounted |
| Ready (normal) | Steady ChaoN lime green (nominal #B4F66B) | READY / BOOT TO ARM | USB connection or successful execution |
| Armed | Flashing yellow, 250 ms ON / 250 ms OFF | ARMED / BOOT TO RUN + remaining seconds | First debounced press-release, expires after 30 s |
| Running | Steady cyan | RUNNING / BOOT TO STOP | Second distinct press-release |
| Cancelled | Solid red for 2 s | CANCELLED / KEYS RELEASED | BOOT press during execution; then returns to Ready |
| HID fault | Flashing red, 125 ms ON / 125 ms OFF | FAULT / CHECK LOGS | HID transport/runtime failure until USB disconnect |

These are requested/default **software behaviors**, not yet verified on LCD hardware. The APA102 may look less saturated or shifted because its global brightness is fixed at 4/31.

**USB insertion stays inert.** The display/LED are indicators, not safety controls. BOOT remains a ROM strap at insertion and the user must hold it for download-mode recovery. The cancelled screen means the firmware attempted release-all; it does not prove host confirmation.

## Original-board LCD interface

Vendor's original-board LCD example is the source for wiring and orientation **candidate**:

- ST7735, 160x80, ESP-IDF native SPI2, 16 MHz
- MOSI GPIO3, SCLK GPIO5, CS GPIO4, D/C GPIO2, RESET GPIO1
- Backlight GPIO38, active LOW: initialize OFF, switch ON only after first successfully written frame
- Initial landscape MADCTL=0xA8, inversion ON, column offset +1, row offset +26, RGB565
- 25,600-byte aligned static RGB565 framebuffer (no PSRAM); redraw on status/countdown changes rather than every tick
- No Arduino, PlatformIO, copied product firmware, Wi-Fi, or SD card management

The SPI initialization sequence and display orientation are preliminary. Prove visible output and readback-inaccessible failure handling with the actual board before declaring this compatible. LCD failure is logged as \`LCD_INIT_FAIL\`/\`LCD_WRITE_FAIL\` and **does not disable HID or bypass the physical arm**.

## Physical acceptance tests after explicit flash approval

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
