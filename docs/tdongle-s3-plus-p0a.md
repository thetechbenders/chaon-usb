# P0-A: explicit T-Dongle-S3 Plus build identity (draft)

**Status:** implementation design started, not a completed board port or an authorized Plus firmware image.

## Baseline and isolation

- Baseline source: `ec6536809ab51504d22214211d95d99022f321f4` on `main` at intake.
- Preserve original T-Dongle-S3 behavior, 16 MB flash, no PSRAM, and current Script v0 limits.
- Plus must be an explicitly selected **separate** firmware target with 8 MB OPI PSRAM, independently validated GPIO/peripherals and ROM recovery.
- Do not merge the active MSC/DUCKY draft PR #23 here. It is a separate original-board workstream.
- No Plus binary should be labeled validated or flashable until resolved Kconfig, linker memory placement, target build, artifact board identity and physical review exist.

## Incremental implementation gates

1. [x] Add an explicit Kconfig board choice, defaulting to original S3. Choice currently records intent only; it **does not** switch hardware setup yet.
2. [ ] Verify the exact Plus board/module revision and compare original/Plus schematics signal by signal, especially LCD, SD, BOOT, native USB, PSRAM, microphone, IR and optional secure element.
3. [ ] Introduce an audited board profile consumed by peripheral drivers; reject mismatched target configurations rather than silently falling back to original pins.
4. [ ] Produce resolved ESP-IDF v5.5.3 Plus `sdkconfig` with OPI PSRAM initialization and explicit failure behavior. Prove internal memory retains authorization/control/VM-owned execution snapshot.
5. [ ] Add separate original/Plus CI builds and manifest identity enforcement; reject cross-board artifacts.
6. [ ] Measure `sizeof` values and linker map, run original regression tests, verify build head and get independent review.
7. [ ] Separate P0-B supervised Plus hardware acceptance: USB enumeration, LCD, LED, SD, button, ROM recovery, current and safe error behavior.

## Non-goals

No larger scripts, text pools, program-data migration into PSRAM, new USB features, external-RAM execution authority, OTA, release, flash, or changes to the existing original-board acceptance record.

## Stop condition

Keep this PR in **draft**. Do not merge until board facts, resolved build configuration, CI, packaging and independent review are complete; hardware qualification is a separately recorded event.
