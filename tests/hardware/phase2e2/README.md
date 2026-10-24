# Phase 2E.2 hardware fixtures

These DOS 8.3-compatible fixtures are for controlled physical validation of
firmware source `ae70e489ccd7ca22e2dcbe466bc5f544940a55f7`. They contain
only ChaoN Script v0 `TYPE`, `DELAY`, `TAB`, and `ENTER`
instructions, comments, or deliberate parser/bounds errors. Use an owned
disposable card and a plain text editor. Do not focus a shell, terminal,
password field, browser, or privileged application.

## Files

| File | Bytes | SHA-256 | Purpose |
| --- | ---: | --- | --- |
| `BENIGN.CHN` | 86 | `db18258b1d9c744fa7f8582b81d56a18e23bfdbf610f8570d0ab714a0b54aae0` | Successful selected execution; visible text, TAB and ENTER |
| `SLOWRUN.CHN` | 51 | `f658f962b280f5297ea964ce7da9e2c39d8da00edb10e4c5cad364eb89282c0a` | BOOT cancellation during a three-second delay |
| `KEYREL.CHN` | 109 | `b10831ad8aae2a8a579b70da4a67c1dfd688ffc2b6b0253f2eaf4fd2f9ee3f9e` | Active typing/disconnect and key-release observation |
| `FRESHA.CHN` | 31 | `311ac62eabfd02c2f98a18587a538e7b3f6f865297f15393e0c53efa5e16d348` | Initial same-length freshness payload |
| `FRESHB.CHN` | 31 | `1bac9dc4d746e71e52a3fbcabcfdac7550329bd036bdafda989c71c01b631268` | Mutated same-length freshness payload |
| `BADGRAM.CHN` | 29 | `198657c36e2e4a3db9da417d6127acdc2f836f5235927740d2ffb15485f58137` | Syntax rejection on line 2 |
| `EMPTY.CHN` | 0 | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` | Empty-source rejection |
| `OVER2048.CHN` | 2,048 | `73b5f3ab2f561073532806be2ed55bac8bff29ac10123404fd8b6892d32d5e08` | Source-ceiling rejection |
| `CONTROL.TXT` | 31 | `c74eb0f8a416cdba0d10be039668c8c77f34e68b2698c566ad63cb660f5ffa6b` | Non-.CHN selection rejection |

`FRESHA.CHN` and `FRESHB.CHN` have exactly the same byte count and
differ only in the intended typed marker. A physical freshness test must
preserve the selected file's raw FAT identity while changing its bytes.
Merely removing the card and causing catalogue invalidation proves fail-closed
invalidation, not the SHA-256 reread check in isolation. Do not claim the
stronger result without an observed execution-verification attempt against
preserved raw identity.

The firmware exposes at most eight root entries. Split the fixtures into cold
batches rather than deleting unrelated files or overflowing the catalogue.
Copy and eject the card only under the approved test procedure; no preparation
step in this directory writes removable media.

## Host verification

From the repository root, compile `fixture_check.c` together with the
production parser and inspection sources using the same warnings-as-errors and
sanitizer policy as the native suite. The test must report every expected
state, confirm the equal-size freshness pair, and end with:

    Phase 2E.2 fixture validation: PASS

This verifies fixture bytes against the production parser. It does not replace
FAT selection, SD reread, USB HID, BOOT timing, or physical cancellation tests.
