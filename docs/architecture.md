# Architecture and trust boundaries

    Original T-Dongle-S3 USB-A connector
                |
     ESP32-S3 USB OTG peripheral
                |
         esp_tinyusb / TinyUSB
                |
     ChaoN HID report adapter (C)
                |
      ChaoN script executor (C)
        |               |
    script parser    US HID map
        |
    trusted onboard test text (v0)

Firmware ownership: ChaoN owns GPIO, scripts, user interactions, HID report semantics, safety policy and product identity. ESP-IDF and TinyUSB own the transport, USB framing, RTOS and drivers. No Dragon-core dependency.

## Firmware slices

1. **v0 candidate:** USB HID keyboard and BOOT button, APA102 status, embedded benign script only. Demo should be inert until two physical press/release actions; arm expires after 30 s.
2. **Hardware bring-up:** LCD + SD read-only driver and robust button interaction, including physical measurements.
3. **User script library:** bounded text parser, verified files, preview, select/run/cancel. Script grammar and file format must be versioned.
4. **Optional management:** local read-only status first, then carefully authenticated update, if ever authorized. No remote auto-run by default.

## Invariants

- No script runs just because USB enumerates, reconnects, reboots, or receives a network request.
- A host cannot select/arm/run scripts through HID traffic.
- Report release on every normal key, failure and cancellation path.
- Input bounds are explicit; no unbounded dynamic memory in runtime script parsing.
- No automatic retries on partial execution.
- The host computer, not the USB device, decides where typed characters land. A benign payload can still be hazardous in a privileged terminal. Test only in a controlled text editor.
- USB disconnected means no output; a new connection starts disarmed after reset.
- A bad card, malformed script or unsupported keyboard layout is a visible failure, not interpreted as a valid payload.

## Notes

The USB transport callback is synchronous at the ChaoN adapter boundary; the TinyUSB stack itself schedules USB transactions. Timing and key-release acknowledgement need physical validation. No screen, SD, network or production boot/recovery policy is implemented in v0.

