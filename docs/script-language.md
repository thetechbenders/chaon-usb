# ChaoN Script v0

A small original script grammar implemented in **C11** for the built-in HID demo and bounded FAT32-root `.CHN` files on the original T-Dongle-S3. **Not DuckyScript-compatible.** SD programs require physical PICK → ARM → RUN and a passing last-moment freshness check.

## Grammar

- One instruction per line; CRLF or LF accepted.
- Blank lines, leading/trailing ASCII whitespace and lines beginning with `#` are ignored.
- `TYPE "text"` sends printable US-ASCII characters through the US HID usage map. Supported escapes inside quoted strings are `\\"` and `\\\\` only.
- `DELAY N` waits `N` milliseconds, 1–10000.
- `ENTER` presses and releases Enter.
- `TAB` presses and releases Tab.

Example:

```text
# Use only on an owned test workstation
DELAY 500
TYPE "ChaoN test"
TAB
TYPE "ready"
```

## Bounds

For SD files, the reader accepts **1–2047 source bytes**, keeping separate room for a terminating NUL. The parser has a 2048-byte source buffer, at most 32 executable instructions, and at most 80 characters per TYPE. Invalid syntax or unsupported characters reject the **entire** program. No partial execution is authorized.

Only FAT32 root files with supported short DOS 8.3 names and a raw `.CHN` extension can supply selected scripts. FAT12/FAT16 metadata preview does not imply that those files are executable.

No loops, conditions, key chords, file writes, shell execution API, network command handling, layout detection or Unicode typing are implemented. A non-US host layout may render unexpected characters.

See [selection contract](phase2e2-selection-contract.md), [inspection details](script-inspection.md) and [security](security.md).
