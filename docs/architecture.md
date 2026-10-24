# Architecture and trust boundaries

**Current status:** Phase 2E.2 merged, hardware accepted on the original T-Dongle-S3 with documented physical limitations. Not production-released.

```text
BOOT / GPIO0 → debounced one-button controller
                   │
    FAT32 root browser → PICK / async inspection
                   │
       SD task: bounded read, Script v0 parse
                   │
       immutable selected program + SHA-256
                   │
      READY → ARM → RUN (separate gestures)
                   │
        production app decision authority
                   │
      one-use grant, USB epoch, generation
                   │
      SD task: independent source re-read + hash
                   │
      execution slot (one-time prepare / take)
                   │
        ChaoN VM → US HID map → TinyUSB
                   │
                  Host
```

The embedded benign demo uses the same VM/HID transport, but only when no selected/blocked SD execution context exists. An invalidated selected run cannot fall back to the demo.

## Component ownership

ChaoN owns GPIO, the browser, SD inspection and selection, scripts, execution decisions, safety guards, HID report semantics and product identity. ESP-IDF/FreeRTOS and TinyUSB provide RTOS/driver/USB transport services. This is a standalone C11 firmware project, not Dragon-family firmware.

The SD worker performs read-only FAT access and hashing outside the SD state mutex, then publishes bounded results. The main task owns selected execution authority and the immutable execution copy. Display state communicates the selected program in READY and ARMED. No network-based control or USB-host-command execution exists.

## Safety invariants

- USB insertion, enumeration, reboot and reconnect do not trigger execution.
- PICK never authorizes HID by itself; ARM and RUN require separate debounced gestures within 30 seconds.
- Each selected execution consumes a single-use grant bound to its selection/source digest and USB epoch.
- A new SD re-read and SHA-256 match are required immediately before selected execution; stale, changed or unavailable sources deny output.
- An invalidated selected RUN does not fall back to the embedded demo in the
  same transaction.
- BOOT press and USB loss request cancellation; the VM attempts an all-keys release. If USB remains mounted, a failed release is a FAULT; disconnect instead returns the controller to its unmounted state.
- No automatic resume/retry; controller state and source authority do not survive reconnection.
- A safe editor and explicit authorization are necessary: the device cannot know the host application's focus.

See [the current selection contract](phase2e2-selection-contract.md), [security](security.md) and [physical evidence](phase2e2-hardware-evidence.md). Earlier bring-up and architecture sections should be interpreted as historical stages rather than active design proposals.

## Scope and deferred work

Native tests and physical acceptance do not establish a production security certification. Remaining specialized fault injection is documented in the evidence ledger. USB VID/PID distribution policy, release packaging and provenance remain open in [release issue #5](https://github.com/thetechbenders/chaon-usb/issues/5). Wi-Fi management, OTA, BLE HID and remote execution are not implemented commitments.
