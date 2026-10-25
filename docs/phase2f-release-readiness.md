# Phase 2F: experimental release readiness decision record

**Status:** planning only; no firmware release, tag, flashing, or distribution authorized.
**Baseline:** `main` at `8de9959b1b18d4e948b8c52e9914a0cfdb6c146c` (documentation merge). Accepted **firmware source**, not this branch: `ae70e489ccd7ca22e2dcbe466bc5f544940a55f7`.
**Owner decision tracker:** [issue #5](https://github.com/thetechbenders/chaon-usb/issues/5).

## Proposed experimental-release boundary (requires owner approval)

An explicitly labeled *publicly downloadable experimental* build for the **original LILYGO T-Dongle-S3 only**, manually flashed over ESP32-S3 ROM serial download, on owned/authorized hosts. USB HID keyboard + LCD/LED + read-only FAT32-root .CHN selection are the current features. No Wi-Fi, OTA, remote execution, encrypted/signed firmware verification, or support for T-Dongle-S3 Plus is claimed. No auto-run on USB connection. This scope must be approved before a release candidate is promoted.

## Audit: confirmed facts vs open decisions

| Area | Verified from repository / acceptance evidence | Pending owner decision or verification |
| --- | --- | --- |
| Licensing | `LICENSE` contains the GNU AGPL v3 text; README and CONTRIBUTING identify AGPL-3.0-only. | Confirm Techbenders has authority to license every shipped original source/artwork and whether dependency notices, source offer, credits and any binary redistribution duties are covered. License selection is **not** pending; distribution approval/compliance review is. |
| USB identity | Application sets `usb.descriptor.device = NULL` (TinyUSB component default); device strings are Techbenders / ChaoN USB HID / CHN-S3-DEV / ChaoN Keyboard. Accepted hardware enumerated `303A:4004`. | Espressif documentation permits its VID 0x303A with the TinyUSB default PID for standard USB device classes (https://docs.espressif.com/projects/esp-iot-solution/en/latest/usb/usb_overview/usb_vid_pid.html). Check that pinned esp_tinyusb 2.0.0 actually supplies default PID 0x4004 for ChaoN's HID configuration, and document the basis and string/serial policy for public distribution. **Do not claim ownership of 0x303A or USB certification**. A dedicated Espressif PID request remains an optional later choice. Any descriptor change creates a **new firmware candidate** requiring CI and relevant hardware enumeration testing. |
| Flashing/recovery | ESP32-S3, 16 MB, no PSRAM; known-good candidate images at 0x0 bootloader, 0x8000 partition table, 0x10000 app. BOOT-held connection enters ROM; accepted image and private factory backups verified. `sdkconfig.defaults` specifies 16 MB. | Approve manual ROM-flash-only instructions, minimum backup expectations, tool/host prerequisites, rollback steps, safeguards against wrong board/port, and whether user-facing packages include all three images and checksums. No OTA is implemented. |
| Source trust | FAT32 root `.CHN`, short filenames, bounded parser (1–2047 file bytes), PICK and separate ARM/RUN; one-use grant with last-moment source SHA-256 reread; denied SD execution cannot fall back to demo. Read-only SD access. | Approve disclosure that local scripts are user-controlled and **not authenticated or signed**, and that operator gestures cannot make an untrusted payload intrinsically safe. Future wireless/script ingestion is out of scope, not implicitly approved. |
| Build/provenance | Pinned ESP-IDF v5.5.3 build and espressif/esp_tinyusb 2.0.0; CI native tests and ESP32-S3 build; GitHub Actions outputs candidate app, bootloader, partition table, flash args, ELF and dependency lock for 30 days. Hardware-accepted artifact ID `12573854980` has ZIP SHA-256 `abe10ebffa2d2949de1fb860b41f701097f930e65c86d45b7eb0e946183d69c7`. | Decide whether experimental release requires cryptographic signing, independently reproducible binary builds, provenance attestations, SBOM/dependency notices, retention policy and immutable release manifest. Do not call builds reproducible or firmware signed without evidence. |
| Compatibility and accessibility | Original T-Dongle-S3 physically checked on a Windows host. USB uses a **US HID keyboard map**; non-US layouts may differ. BOOT is also ROM strap. | Define minimum supported host OS/layout and testing matrix; decide whether Windows-only verified is sufficient for an experimental release. Provide an accessible nonvisual instruction path and distinguish design support from tested support. |
| Limitations | Phase 2E.2 physical evidence: **14 PASS, 0 FAIL, 4 BLOCKED, 1 PARTIAL, 2 N/A, 1 NOT REQUIRED**. Blocked: same-identity source mutation, BOOT during brief verification, forced verify timeout, injected live SD read failure. | Decide whether these explicit limitations are acceptable for experimental labeling, or require additional fault-injection fixtures/testing first. The impossible powered-SD-removal manual test was removed and must not be reinstated. |
| Product readiness | PRs #17, #18, and #20 merged; release issue #5 remains open. | Approve release version/name, audience, support/bug-report process, final risk acceptance, and final publication action separately. No release has been authorized. |

## Work packages and acceptance criteria

### 2F-A: owner decisions (first dependency)

Record explicit decisions in issue #5 on: (1) experimental vs public production scope, (2) USB VID/PID/string ownership policy, (3) manual flashing/rollback model, (4) script trust wording, (5) mandatory artifact/provenance controls, (6) tested host/layout support claims, and (7) whether physical fault-injection limitations are accepted. **Do not infer approvals** from the existing AGPL license or hardware acceptance.

**Owner preference recorded:** Dan wants a public experimental firmware download, not source-only. This sets planning direction but does not authorize publication.\n\n**Exit:** every row above has a recorded owner choice, rationale, and where appropriate a new follow-up task. Release issue stays open until these decisions are actually completed.

### 2F-B: packaging/recovery implementation (after owner decisions)

Create a reviewable packaging workflow that produces a manifest binding source commit, Git tree, ESP-IDF and component versions, firmware filenames/sizes/SHA-256, flash offsets/parameters, license/dependency notices and acceptance/limitations pointer. Include a conservative flash/rollback guide for the *original board only*; never erase factory backups or enable secure boot/flash encryption/eFuses as an unapproved packaging side effect. Make generated assets independently verifiable; test malformed/missing/modified images and wrong-board guardrails.

**Exit:** artifact manifests and recovery steps validated from a clean checkout and independently checked against the packaged bytes; protected hardware remains unmodified.

### 2F-C: identity and compatibility (conditional firmware change)

First verify Espressif's documented default TinyUSB identifier permission against pinned runtime behavior. If identifiers or descriptors change, implement in a distinct firmware PR with descriptor tests, exact ESP-IDF CI, and supervised USB-enumeration/no-auto-type/ROM recovery regression on hardware. Otherwise document why the existing identifiers are legitimately distributable.

**Exit:** approved identifier policy and matching descriptor/host evidence. Existing Phase 2E.2 acceptance is **not** automatically transferable to changed firmware.

### 2F-D: security and documentation gate

Publish draft, not a release: supported/unsupported hardware and host layouts, trust model, operator safety, manual recovery, firmware integrity/provenance, and the four BLOCKED/one PARTIAL physical results. Define issue/security reporting and disclose lack of firmware signature, encrypted storage, OTA and unattended/remote execution. Conduct focused external review of exact candidate content and release materials.

**Exit:** review findings resolved or explicitly accepted with owner sign-off.

### 2F-E: explicit release decision (future separate authorization)

Freeze exact source and distribution artifact digest; run required native/ESP-IDF CI, validate distributable bits, obtain any necessary new physical test evidence, and produce a GO/NO-GO record. **Stop before tags, GitHub Releases, publication, flashing, or merge of firmware changes**, unless Dan expressly authorizes each consequential action.

## Public download package (proposed)\n\nDraft GitHub Release assets: bootloader, partition table, application BIN, flash arguments, human-readable SHA-256 manifest with accepted source and exact build identity, concise Windows-first ROM flashing and recovery guide, license/dependency notices, and benign Script v0 fixtures. Do **not** distribute private factory backups or claim host compatibility beyond the tested Windows setup. No automatic flashing executable is assumed. Publishing the GitHub Release requires a separate explicit approval.\n\n## Proposed sequence

1. **Now:** audit #5, publish this decision record in a draft documentation PR, and update #5 with evidence-backed choices.
2. **Next:** owner chooses USB identity and minimal packaging/provenance requirements, then decide what experimental limitations are acceptable.
3. **Then:** implement packaging and compatibility work in small separate PRs; no release side effects.
4. **Finally:** independent review, exact-artifact checks, supervised target testing only if firmware changes, and explicit release authorization.

## Non-goals

Do not add OTA, Wi-Fi/BLE, remote triggering, new scripting commands, authentication servers, or eFuse/security boot configuration in 2F. Do not fabricate additional physical PASS results or resurrect powered SD removal as a manual test.

## Stop condition

When this plan is captured and issue #5 is updated, stop. Do not implement packaging or USB descriptor changes, merge the planning PR, flash firmware, create a release/tag, or close #5 without fresh authorization.
