# Original T-Dongle-S3 bring-up

**Not yet performed.** These are acceptance steps for when the hardware arrives.

## Setup

1. Inspect the actual PCB and compare with the [hardware register](hardware.md). Do not assume Plus pins/memory.
2. Install ESP-IDF v5.5.3, select ESP32-S3 and build; validate artifact size, partition table and dependency lockfile resolution.
3. Use a sacrificial/owned test computer and a normal text editor; ensure the editor is focused before any authorized typing.
4. Record USB enumeration descriptors, VID/PID, endpoint type and OS driver detection.
5. Power up with BOOT released. Confirm **no keystrokes on insertion**, even if the editor is focused.
6. Press and release BOOT once; confirm armed indicator. Wait >30s; confirm timeout disarms and no typing.
7. Repeat arm, then second deliberate press-release before timeout; confirm exact harmless test string and no residual Shift/key.
8. Verify unplug/reconnect, reset, fast double-click, button held before insertion, USB suspend, and mid-script disconnect.
9. Check status LED output, idle/active current, and whether HID tasks survive a malformed script.
10. Bring up LCD and SD separately, then add them only after proven.

### USB recovery

Vendor instructs: to enter ROM download, **hold BOOT while inserting the device**. This is distinct from holding/pressing BOOT during normal firmware operation. Native USB-OTG HID mode can require returning to ROM download manually for flashing.

LILYGO reference: https://github.com/Xinyuan-LilyGO/T-Dongle-S3/blob/main/docs/en/t-dongle-s3/REAMDE.MD

## Acceptance ledger

| Gate | Evidence | Result |
| --- | --- | --- |
| Host C tests | Local or CI run with compiler flags and commit SHA | Pending |
| ESP-IDF native build | Exact commit + IDF version + artifacts | Pending |
| USB enumeration | Physical USB descriptor and host observation | Pending |
| No automatic injection | Physical controlled editor test | Pending |
| BOOT press/release + arm expiry | Physical timeline | Pending |
| Cancel / release | Controlled physical run | Pending |
| LED, LCD, SD | Board-level validation | Pending |

A passing source build does not close any physical gate.

