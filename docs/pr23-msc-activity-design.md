# PR #23 MSC activity integration (hardware unvalidated)

Inspected registry esp_tinyusb **2.0.0**, upstream esp-usb commit
`b8c36cdd1e748ad6f7b4b2b556b6b028a0baa61d`, `device/esp_tinyusb`.
Source: https://components.espressif.com/components/espressif/esp_tinyusb/versions/2.0.0
Registry archive object: `9ad074c0-affe-454c-a74c-1b1547eb0bbb`.
`tinyusb_msc.c` SHA256:
`5888e22cbaf948ef74676111c6f0e7a78187a5fac9879ec0b7f34b8deb0a3051`.

## Inspection and semantics

- `tinyusb_task.c:tinyusb_device_task` calls `tud_task()` in task context;
  data callbacks run there, not in the USB interrupt handler.
- `tinyusb_msc.c:906` READ10 calls `msc_storage_read_sector` synchronously;
  an error sets sense and returns -1.
- `tinyusb_msc.c:920` WRITE10 validates buffer size, copies to the backend
  buffer and queues `tusb_write_func` via `usbd_defer_func(..., false)`.
  The callback returns bytes accepted; deferred write failure only logs.
  This patch does not fix or conceal that known reliability gap.
- Existing public MSC events concern media ownership/mount/format, not each
  READ10/WRITE10. No supported transfer hook exists in this pinned version.

The isolated build patch inserts one signal at each callback entry, only for
nonzero data length. It reports **attempted READ10/WRITE10 data callbacks**,
including failures and writes not yet persisted. It does not report file-copy
completion, byte counts, host flush, successful transfer, or durability. Host
metadata scans may trigger it. INQUIRY, TEST UNIT READY and enumeration alone
are not signals. There are no new SD accesses or ownership changes.

## Integration boundary

`tools/msc_activity_hook.cmake` runs a Python patcher after IDF project setup.
It checks the full source SHA and two unique callback anchors, writes a build-local
copy retaining upstream Apache-2.0 notices, and replaces exactly one compilation
unit in the managed component target. Managed files stay intact. A changed source
or target layout fails configuration for review. No component vendoring, duplicate
TinyUSB callbacks, SDMMC wrapping, new upstream API, or return-value changes.
Apache-2.0 upstream code remains under its existing license; repository-owned
integration code is AGPL-3.0-only. Generated code is not committed.

The hook performs a relaxed atomic OR into an unsigned-int bit latch. Compile-time
`ATOMIC_INT_LOCK_FREE == 2` requires lock-free operations on the target. It does
no GPIO, LCD, logging, allocation, clock access, waiting or queueing. The app
atomically exchanges the latch every 25 ms; an event concurrent with the exchange
is either consumed now or retained for the next poll. Repeated events deliberately
coalesce; there is no finite event counter to overflow. This is an indicator, not
telemetry. No storage backend callback has been moved to the app task.

The app renders read attempts blue, write attempts amber and mixed attempts purple.
A pulse lasts at most 75 ms of app time followed by at least 125 ms without activity
color; sparse attempts during that gap may be dropped instead of replayed. The
original dim green idle and mounted heartbeat remain between pulses. Unsigned
elapsed arithmetic handles the 32-bit millisecond wrap. Unmounted polling drains
the latch and resets the animation; no persistent backlog is displayed on reconnect.
Scheduling delays can lengthen physical pulse durations and require bench observation.

## Validation limits

Native tests cover latch coalescing/draining, direction bits, idle, mixed bursts,
sustained activity, cooldown, disconnect/reconnect, dropped events and tick wrap.
Patch tests check only the two insertions change and mismatched source/anchors fail.
Target CI exercises the actual registry hash, CMake source replacement, linking and
lock-free requirement. None of these validates physical APA102/USB timing, Windows
behavior, error reporting, media integrity, recovery, or general-purpose storage.
Repeat supervised handoff B01-B12, including independent readback and fault injection.
