# Manager-Thread Workflow (LoRa Device)

Use this for future manager-thread sessions that coordinate multiple assistants.

## 1) Thread control defaults
- This is a manager workflow: when asked to work, create child Codex conversations for bounded implementation, review, or verification packages.
- Prompt the user only for blocking issues. Make reasonable assumptions for implementation details that are consistent with this repo and current standards.
- One child thread = one bounded objective at a time (scope, success criteria, and exit conditions are explicit before work starts).
- Assign ownership by component surface (`src/`, `include/`, `docs/`, `tests/`, `platformio.ini`) and never mix unrelated areas in one subagent handoff unless necessary.
- Use short check-ins every turn: what changed, what still open, next action in 1 line.
- When hardware is not present, implement all hardware-facing firmware, docs, diagrams, gates, and simulation assets as far as possible before stopping for physical bring-up.

## 2) Subagent role/model routing
- Route by task, not by person:
  - **Coordinator (manager)**: sequencing, blockers, final synthesis.
  - **Implementer**: edits and minimal diffs in assigned files.
  - **Verifier**: runs build/smoke checks and summarizes outputs.
  - **Reviewer**: risk scan and integration sanity checks.
- Required model split when available:
  - Coders/implementers: GPT-5.3-Codex-Spark.
  - Code review: GPT-5.4.
  - Integration and final checks: GPT-5.5.
- Escalate only when ownership conflicts or repeated failures appear.

## 3) Bounded ownership rules
- Keep each subagent package to one clear deliverable and explicit files.
- Start-up rule: if a package is not fully scoped, make the narrowest reasonable assumptions; ask only when the missing scope is a real blocker.
- Merge rule: no integration without a handoff note containing changed files and test status.
- Stop/start rule: pause work and wait when dependencies are missing (tooling/hardware/serial logs).

## 4) Wait / skip logic
- **Wait now** when:
  - a required test is queued but blocked by hardware/tooling.
  - verifier reports compile conflict/reproducibility issues.
  - integration checks are incomplete.
- **Skip with reason** when the user says `skip testing` or `skip reporting`:
  - Add the skipped item to a durable in-thread skip queue.
  - Include category (`testing` or `reporting`), skipped item, why skipped, unblock condition, and resume command.
  - Remember multiple skipped items in order; do not collapse them into one generic skipped task.
  - When the user says `continue with testing` or `continue with reporting`, summarize queued skipped items in that category, then resume at the oldest queued item.
  - When the user says `skip`, suggest the next functional step unless no functional steps remain.
- A functional run can complete while testing/reporting items remain skipped, but the final handoff must list the skip queue and must not claim skipped work is done.

## 5) Review + integration checks
- Minimum integration check on every code-changing manager task:
  - `pio run` (or equivalent env-specific build) run by verifier.
  - Relevant docs updated for any workflow, pin, test-plan, or behavior change.
  - No unrelated files reverted.
- Optional checks (run when feasible): `scripts/host-build-smoke.sh`, `scripts/renode-smoke.sh`.
- Process-only guidance changes do not need firmware builds unless they touch build/test assets; run a markdown/content review instead.

## 6) Hardware vs no-hardware
- Hardware-gated changes (display/radio/sensors/GPS/USB host paths): require explicit label:
  - **No-hardware state**: compile + simulation only; no behavior claims on physical I/O.
  - **Hardware-available state**: run bench tests and record observed values.
- Final handoff must state clearly which validation is compile-only and which is hardware-verified.
- If the next step requires hardware, state that explicitly and keep it out of no-hardware completion claims.

## 7) Required final next-step reporting
End each manager session with:
- Done in this session.
- Remaining risks + blockers.
- Skip queue (if any).
- Next-step category: functional work, testing work, or reporting work.
- Whether the next step requires hardware.
- Exact next commands/owners and expected outputs.
