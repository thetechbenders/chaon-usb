# Architecture Decision Records

ADR documents record important architectural choices and their rationale. A proposed ADR is not an implementation authorization; it becomes normative when its documentation PR is reviewed and merged.

| ADR | Status | Subject |
| --- | --- | --- |
| [ADR-002 Rev 3](ADR-002-web-editor-storage-authority.md) | **Proposed** | v0.2.0 browser editor, exclusive SD ownership, physical execution authority, conservative saves and recovery |

## Review and change rules

- Decisions carry stable IDs (for example `ADR002-D04`) so requirements and tests can reference them.
- Subsequent firmware/UI PRs should cite the applicable decisions, requirements, and acceptance tests.
- Changes to accepted architectural decisions require a reviewed amendment or superseding ADR; issues and chats do not silently change the contract.
- The 2026-11-03 ADR-002 Rev 2 report was a **proposed analysis document**, not an accepted repository ADR. Rev 3 records the agreed narrower direction and explicitly retains unresolved gates.
- The v0.1.x candidate under PR #26 is independently qualified for build provenance only; this architecture work does not alter that firmware or authorize a flash/merge.
