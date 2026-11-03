# ADR-002 Rev 3: v0.2.0 Web Editor, Storage Ownership and Execution Authority

- **Date:** 2026-11-03
- **Status:** Proposed for acceptance in this documentation PR; **not yet merged, implemented or hardware-qualified**
- **Scope:** Original LILYGO T-Dongle-S3, ESP-IDF v5.5.3, 16 MB flash, no PSRAM
- **Provenance:** Supersedes the *proposed* ADR-002 Rev 2 report (2026-11-03) after review and Dan's approval of the Rev 3 design direction. The Rev 2 report and ADR-001 A–E decisions remain historical inputs.
- **Independent hardware gate:** PR #26 frozen commit `765831e803ab95a20f69e5cd88c02f940a28600c` remains unflashed and distinct; v0.1.x testing is not authorization for v0.2.0.

## Decision

ChaoN v0.2.0 will expose a lightweight browser editor and **remote PICK** while retaining **physical-only ARM and RUN**. Editing, storage, verification, USB mode, and execution share an explicit firmware-owned state/authority model. The browser cannot synthesize physical gestures or directly generate HID reports.

These decisions are the architectural baseline, *not* a complete recovery algorithm or permission to implement before the corresponding design gates close.

| ID | Accepted direction for Rev 3 | Enforcement / open qualification |
| --- | --- | --- |
| ADR002-D01 | Start with **HID-only WPA2 SoftAP** and bounded authenticated HTTP sessions; do not activate browser editing in MSC mode. | Measure heap, timing, USB/Wi-Fi coexistence and attack surface on the actual board. |
| ADR002-D02 | **One SD owner** at a time; serialize HID storage access through one worker and prohibit writable FatFs overlap with raw-FAT inspection. | Verify SDMMC deinit/reinit lifecycle and cache coherence; make failed unmount fail closed. |
| ADR002-D03 | v0.2.0 **create and replace** only, for bounded root `.CHN` scripts; defer rename and deletion. | Respect 8.3 identity, current 2,047-byte executable source limit, eight-entry root catalogue and 64-sector scan bound; account for non-`.CHN` root entries. |
| ADR002-D04 | Browser may **PICK**, but only distinct **physical ARM and RUN** gestures can authorize execution; fresh script read/hash/identity check immediately precedes a one-shot grant's use. | Reject remote ARM/RUN, stale source generations, concurrent mutations and grants after USB epoch changes. |
| ADR002-D05 | **Disable implicit DEMO execution in managed v0.2.0 HID mode**, including pre-PICK, failed PICK, browser re-entry, USB loss and reset paths. | Explicitly selected diagnostic demo, if retained, requires separate specification; do not alter legacy v0.1.x behavior through this ADR. |
| ADR002-D06 | Saves use a **bounded verified backup + staged file + final verification**. Preserve the old copy until new copy verified. On ambiguous state, refuse to mutate instead of reconstructing speculatively. | Define exactly ordered sync/unmount/readback/rename/replace operations and crash-point tests **before coding**. FAT32 does not offer a multi-file atomic transaction. |
| ADR002-D07 | **Dirty, unreadable or inconsistent FAT32** media blocks automated managed mutations. Neither clean bit nor digest matching proves integrity or exclusive provenance. | Resolve dirty-bit semantics and interrupted-own-write paradox; first release may choose diagnostic/manual recovery instead of automatic repairs. Never silently format. |
| ADR002-D08 | Recovery evaluation precedes consumption of one-shot `DUCKY`. Ambiguous artifacts preserve evidence and block normal personality selection. | Design collision-safe metadata placement; treat externally edited target/journal as untrusted. |
| ADR002-D09 | **Recovery-only MSC** is an opt-in, distinct, **physically authorized** boot mode. No HID, no script run, no firmware writes after handoff. | Gate its entry gesture against real board and ROM download mode; if safe ownership cannot be established, block USB instead. |
| ADR002-D10 | Only one mutation operation at a time, conditional on current source revision/generation and controller state; firmware validates all API inputs. | Define API status codes, authentication/session expiry, CSRF/Origin policy and request bounds in the API contract. |
| ADR002-D11 | On physical RUN, **quiesce HTTP mutations with a bounded deadline**, verify source, consume one-use authority and execute with independent cancellation/key release. | Test synchronous `chaon_run()` timing, disconnected clients, task priority, USB loss and stuck requests; do not block cancellation on HTTP. |
| ADR002-D12 | Firmware diagnostics must truthfully distinguish storage owner, USB personality, selected source, authorization, fault and network status. | Static lightweight editor with local syntax feedback backed by authoritative firmware parser/validation; no heavyweight framework by default. |

## Rationale and tradeoffs

**SoftAP versus USB control:** SoftAP provides a practical local browser interface without adding another USB personality or CDC/WebUSB control transport while HID is active. It costs radio power, heap and additional security exposure, so hardware/resource qualification is required. STA and USB-browser transports are deferred, not prohibited forever.

**Physical authority:** Remote script selection is useful; remote grant creation or HID injection would expand ChaoN into an unattended programmable keyboard. The browser may request selection and observe state, but cannot arm, run, bypass fresh verification, or revive revoked grants.

**Storage:** A filesystem mutex alone cannot reconcile FatFs's cached metadata with a raw sector reader. Transition between exclusive, explicit phases and revoke stale selection before mutation. Host-owned MSC never shares the card with an app writer.

**Durability:** A small script does not justify a general-purpose transaction database. Keep the protocol deliberately narrow; do not claim power-loss atomicity, persistence after SD-controller acknowledgement, or automatic correction of arbitrary host edits.

## Essential state invariants

1. **ST-01:** Exactly one physical SD owner; `MSC_HOST`, `RAW_READ`, `FS_MUTATION` never overlap.
2. **ST-02:** Failed mount/unmount, unknown ownership, media faults, or ambiguous recovery blocks normal USB exposure and execution.
3. **ST-03:** Never autoformat; never overwrite a target whose identity/bytes differ from the expected old/new state.
4. **ST-04:** Revoke READY and pending verification *before* mutation, and invalidate generations/tickets when the storage phase changes.
5. **AU-01:** Remote PICK may replace READY while idle, but cannot authorize ARM/RUN or modify ARMED/VERIFYING/RUNNING/CANCELLING state.
6. **AU-02:** No implicit DEMO path can restore execution eligibility on reset/re-entry in managed mode.
7. **AU-03:** Every RUN requires physical gesture, nonexpired one-use grant, fresh independent verification, and valid USB/source/epoch state.
8. **AU-04:** Cancellation and HID key release remain available independently of Wi-Fi/HTTP and storage-worker responsiveness.
9. **REC-01:** Examine any interrupted-save artifacts before `DUCKY`; unrecognized or externally conflicting evidence is left intact.
10. **REC-02:** Recovery-only MSC is a separate explicit physical exception, not an automatic fallback.

## Must-resolve design gates before v0.2.0 firmware work

- **G1: Save protocol.** Specify exact create/replace operation ordering, backup/stage naming, sync boundaries, error actions, crash cut points, and manual-repair fallback. No journal engine is implicitly authorized by Rev 3.
- **G2: Dirty media.** Verify actual FatFs clean-shutdown handling and define admissible writes after an interrupted ChaoN save; if safety cannot be shown, stop at non-destructive refusal.
- **G3: Metadata/catalogue.** Prove 8-entry root catalogue behavior with directories and non-scripts; choose metadata location/collision rules.
- **G4: Physical recovery.** Select and test a physical gesture for recovery-only MSC that does not conflict with bootloader entry or HID selection.
- **G5: Ownership.** Inspect exact ESP-IDF SDMMC/FatFs lifecycle; measure phase transitions, USB task blocking, and failure handling.
- **G6: API/security.** Specify authenticated bounded sessions, origin/CSRF mitigation, concurrency, revision checks, denial-of-service handling; no remote ARM/RUN endpoint.
- **G7: Resource/cancellation.** Collect no-PSRAM heap/stack/latency/USB-Wi-Fi/brownout results and exercise independent cancel/key release.
- **G8: v0.1.x separation.** Complete its independent supervised hardware qualification before treating current MSC behavior as tested infrastructure.

## Traceability example

- `ADR002-D04` → `REQ-AUTH-007`: physical ARM creates a scoped, expiring, one-use authority token; remote PICK cannot create one.
- `REQ-AUTH-007` → `TEST-AUTH-014`: HTTP RUN attempt after remote PICK must not generate any HID report without physical ARM and RUN.

Detailed requirement numbering and implementation tests will be established in separate v0.2.0 specification PRs. No acceptance by assertion.

## Rev 2 → Rev 3 disposition

- **Retained:** ADR-001 A (FatFs writes + independent verification), B (remote PICK collision rule), C (HID-only SoftAP and HTTP quiescence), E (unique WPA2 credentials and expiring API sessions); single SD owner; physical execution boundary; recovery-before-marker ordering.
- **Narrowed:** ADR-001 D's bounded journal/recovery direction is **superseded for v0.2.0 implementation scope** by a minimal create/replace protocol and conservative refusal on ambiguity. No general-purpose journaling/rename/delete requirement.
- **Resolved policy direction:** No implicit managed-mode DEMO; recovery-only MSC must be physically authorized.
- **Not falsely resolved:** Dirty FAT32 semantics, exact crash-safe save algorithm, metadata placement, SD driver lifecycle, physical recovery gesture, runtime resource budget and hardware qualification remain open gates.

## Implementation governance

This ADR becomes the normative **v0.2.0 architecture direction only when its PR is reviewed and merged**. Until then it is proposed. The separate requirements/state machine/API/test-plan documents will be reviewed and versioned alongside code, with issue and test references to decision IDs. An architectural change requires a reviewed ADR amendment or superseding ADR; a GitHub issue by itself does not revise the contract.

**Not permitted by this ADR:** v0.1.x firmware edits, PR #26 merge, flashing, release publication, silent formatting, unsupervised script execution, or assumptions of physical acceptance.
