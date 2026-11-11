# Contributing

ChaoN is a **public, pre-release** engineering project licensed under the [GNU Affero General Public License v3.0 only](LICENSE) (SPDX: `AGPL-3.0-only`). Before submitting changes, coordinate scope and applicable permissions with the repository owners. Contributors must have the rights necessary to submit their changes. **Source-code and documentation contributions** intended for inclusion must be distributable under AGPL-3.0-only. **New artwork contributions require explicit separate rights/licensing approval** before any license grant is asserted; the three PNG assets identified in [the branding inventory](docs/branding/README.md#asset-inventory) are separately [CC BY-NC 4.0 licensed](docs/branding/LICENSE-ARTWORK.md) for the rights their contributor may grant. New artwork is **not automatically covered** by that existing file-specific grant. Do not copy third-party code or assets into the repository without reviewing attribution and license compatibility.

## Working agreements

- Prefer reviewable feature branches and PRs. Do not push directly to protected branches.
- Firmware is C11 + ESP-IDF, not Arduino. The USB stack is Espressif's TinyUSB component.
- Host-side parser, encoding and execution state transitions need deterministic tests.
- No auto-execution on insertion, hidden background execution, or bypass of the deliberate arm/execute user interaction.
- Keep hardware pins, component limits, SDK versions and USB behavior traceable to reference docs or physical tests; clearly distinguish evidence from assumptions.
- Treat failed tests, compiler warnings and unverified hardware as blockers.
- Do not include real credentials, customer data, private payloads, or secrets.
- Restrict tests to owned systems or systems with explicit authorization. Never use test fixtures that run commands, change settings, or exfiltrate data without scope review.
- A green CI build is not a substitute for on-board USB enumeration/recovery checks.

## PR checklist

- [ ] Scope and behavior documented
- [ ] Native host tests and ESP-IDF build run on the exact commit
- [ ] Hardware unverified claims labeled accurately
- [ ] Test and recovery instructions updated
- [ ] No unknown scripts can run merely on USB insertion
- [ ] Changes do not add credentials or proprietary material
- [ ] No release, tag or merge without separate approval

