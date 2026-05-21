# RF Safety Bring-Up Template

Use this template before any RF transmit behavior or peer testing.

## Preconditions

- [ ] `docs/ontario-canada-rf-signoff.md` reviewed and applicable profile prepared
- [ ] `docs/lora-regional-caveats.md` reviewed for channel spacing / EIRP limits
- [ ] Legal/project approval marker is stored in test session notes
- [ ] Antenna orientation and power limits documented for planned distance test

## No-Air Baseline

- [ ] `radio` command diagnostics show transmit guard active (`transmitGuarded=yes`)
- [ ] `radio(stub) simulated_tx` is used for software-only smoke paths
- [ ] No explicit command path used to force RF packet emission until approval gate is cleared

## Commands / Checks

- [ ] `radio`
- [ ] `status`
- [ ] Visual review of RF test plan and duty cycle caps

## RF Guard Assertions

- [ ] `txAllowed` and `txLocked` in config profile are aligned with approval state
- [ ] No unplanned duty-cycle burst while RF safety gate is enabled
- [ ] `moduleProbePassed` only transitions based on AT probe response, not UART open event
- [ ] Test logs include explicit "no RF emitted yet" operator statement

## Hard Stop Conditions

- [ ] Unauthorized or unapproved command path could force on-air emission
- [ ] Guard status is `transmitGuarded=no` before final approval is recorded
- [ ] Module reports invalid region/power condition not yet reviewed

## Pass / Fail

- [ ] RF safety gate remains active and auditable
- [ ] RF transmit authorization required before Stage E field test: [ ] requested [ ] completed
- [ ] Evidence link / sign-off artifact:
