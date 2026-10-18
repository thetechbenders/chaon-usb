# ChaoN Script v0

This is a small **original C grammar** for a future standalone USB HID test tool. It is **not DuckyScript-compatible**.

## Grammar (v0 candidate)

- One instruction per line, CRLF or LF accepted.
- Blank lines, leading/trailing ASCII whitespace and lines beginning with `#` are ignored.
- `TYPE "text"` sends each printable US-ASCII character as a USB HID keyboard usage. Supported escapes within strings: `\\"` and `\\\\` only.
- `DELAY N` waits N milliseconds, with 1 <= N <= 10000.
- `ENTER` presses and releases Enter.
- `TAB` presses and releases Tab.

Example:

    # Run only on an owned test workstation
    DELAY 500
    TYPE "ChaoN test"
    TAB
    TYPE "ready"

Constraints: source <= 2048 bytes including NUL, <= 32 executable instructions, <= 80 chars per TYPE, ASCII printable only. Invalid syntax fails the **whole** program. There is no partial execution, key combination syntax, loops, conditionals, file/network access, command execution API, layout autodetection, or shell integration.

The US keyboard translator produces HID key usages and Shift modifiers; it does not type Unicode and does not guarantee the same visible character on a host with a non-US layout.

## Future possibilities (not implemented)

Formal bytecode representation, multi-layout maps, key combinations, file-based script selection, previews and signature/trust metadata. Each must preserve bounds and interactive arming.

