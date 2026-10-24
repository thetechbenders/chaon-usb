# Execution authority design and history

**Current disposition (2026-10-24):** Phase 2E.2 is merged into `main` and hardware accepted with documented physical limitations. The operative behavior is described in [the Phase 2E.2 selection contract](phase2e2-selection-contract.md). The 2E.1 material below is preserved as **historical design**, not currently active UI or control logic. See [physical evidence](phase2e2-hardware-evidence.md); no production release has been approved.

## Historical Phase 2E.1 separation

Selection, validation, staging, authorization and execution are distinct
states. PICK may request validation and produce a staged program, but it must
never arm HID. A future execution controller must issue a new event type and
bind a one-shot authorization to the exact staged generation, raw directory
identity, source-change fingerprint and current USB connection epoch. The
existing fixed-demo `EXECUTE` event must continue to select only the embedded
demo.

## Historical Phase 2E.1 one-button confirmation (superseded in Phase 2E.2)

One button makes timing aliases dangerous, so entry into a future SD execution
confirmation state must itself be non-executing. A practical candidate is:

1. while BROWSE shows a valid staged PICK, hold BOOT for at least five seconds
   and release to enter an `EXEC CONFIRM` screen;
2. consume that release and require at least 500 ms of stable released state;
3. within a ten-second confirmation window, require a new deliberate
   1.5–3-second press and release to authorize exactly one run;
4. treat a short tap, overlong hold, timeout, state change or any validation
   mismatch as cancellation, never as another gesture.

The exact thresholds require native boundary tests and original-board physical
testing before adoption. Display failure must make the feature unavailable.
The transition release, cancellation release and completion release must all
be suppressed so a BOOT release cannot arm or execute anything accidentally.

Authorization expires after one use or ten seconds, whichever comes first.
It is cancelled by USB disconnect, catalogue/source change, new selection,
browser exit, SD error, controller reset/fault, or BOOT cancellation. It must
not survive reconnect or reboot and must never be granted by insertion,
automatic timers, a network request or validation success alone.

## Historical source-freshness requirement

Raw name/cluster/size identity does not prove that same-size sectors were not
modified after staging. Before entering `EXEC CONFIRM`, a future implementation
must re-read and revalidate the selected file and bind authorization to a
fingerprint of the exact validated bytes. Immediately before execution it must
verify the generation, identity and fingerprint again, or atomically transfer
the freshly validated program into a one-shot execution-owned buffer. Any
change fails closed and returns to non-armed browsing.

## Shared execution, cancellation and recovery requirements

- Execution owns one immutable bounded program copy; SD replacement cannot
  mutate it. No automatic retry or resume is allowed.
- BOOT press, USB loss or a detected fault requests cancellation. Existing
  short delay slices keep cancellation responsive.
- Every exit path attempts an all-keys release. A failed release is a FAULT,
  not successful cancellation; reconnect begins unarmed and sends no program.
- Fault recovery requires a released button and a fresh USB/controller state.
  A severe or uncertain fault requires power-cycle/recovery, not automatic
  continuation.
- BOOT-held power-on remains the ESP32-S3 ROM download path. Runtime gesture
  handling cannot run before ROM strap sampling and must not impair recovery.

## Historical fixed-demo-only compatibility requirement

Outside BROWSE/confirmation, the existing two short press-release gestures,
30-second arm expiry, cancellation and embedded demo remain unchanged. The
future SD path must have a distinct controller state and event, and code review
must continue to show that the fixed demo is the only program reachable from
the legacy `EXECUTE` event.

Implementation, flashing and physical validation of this design require a new
explicitly authorized milestone.


## Phase 2E.1 inert physical confirmation prototype (historical baseline)

The initial Phase 2E development branch adds a **non-executing** confirmation
screen and a BOOT gesture classifier. This does not authorize or perform HID.
The existing fixed-demo two-tap path remains the only call to the VM.

- While BROWSE contains a PICK with a **VALID** current inspection summary,
  and the LCD is healthy, holding BOOT for 5 seconds and releasing may enter
  the `EXEC CONFIRM` screen. Without those conditions, a long hold exits
  browsing as before.
- The transition release is consumed. At least 500 ms of released BOOT is
  needed before another press. A distinct 1.5–3 second hold produces only
  an **inert acknowledgment** and returns to READY.
- Timeout at 10 seconds, USB loss, catalogue change, invalid inspection,
  or LCD failure cancels confirmation. No event reaches `chaon_run`.
- The 20 ms poll / 40 ms debounce controller continues to enforce button
  semantics. The BOOT-held power-on ROM behavior is unchanged.

The current VALID inspection summary is sufficient to display an *inert*
confirmation prototype but is **not** proof of source freshness or execution
authority. The one-shot `chaon_exec_auth` module remains separate from the
runtime gesture classifier until Phase 2E.2 implements fresh source re-read,
fingerprint, execution-owned program and USB epoch binding. No SD-program
execution is approved or connected in Phase 2E.1.


## Phase 2E.1 usability correction (historical baseline)

The original 5-second blind hold was unreliable in physical testing (2/10
successful attempts reported). In the revised draft, pressing BOOT while a
VALID file is PICKED renders a progress bar updated in 200 ms increments.
At five seconds it reads `RELEASE NOW`. The gesture still only enters an
**inert** execution confirmation screen, and produces no SD-derived HID.

Releasing after 900 ms but before five seconds **keeps the file selected**;
it no longer accidentally invokes the three-second browser exit. To leave
PICKED, make a short tap to unpick, then hold three seconds to exit browsing.
The LCD bottom hint changes between `TAP UNPICK` and `3S EXIT` accordingly.
Cancellation, source eligibility, USB disconnect, missing LCD, and ROM
recovery remain fail-closed. Do not flash the UX revision until its exact-head
native and target builds have passed, with an independent review.
