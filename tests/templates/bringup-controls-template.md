# Controls Bring-Up Template

Use this template when encoder, button, and toggle wiring is present.

## Session Header

- Date / time (UTC):
- Operator:
- Board / fixture id:
- Board rev:
- Git branch:
- Git commit:
- Build manifest file:
- Firmware environment:

## Evidence Assets

- Serial transcript:
- Photos of connected controls:
- Pin-plan revision reviewed:

## Baseline

- [ ] `pio run -e stm32wb55_hardware_gates` passed
- [ ] Hardware-gated firmware uploaded
- [ ] `pins` command responds before any manual interaction
- [ ] Initial `pins` state recorded
- [ ] No control appears stuck before interaction

## Control Checks

Record the observed `pins` / `input` output before and after each action.

| Control | Action | Expected evidence | Observed evidence | Pass |
| --- | --- | --- | --- | --- |
| Encoder A/B | One detent clockwise | `enc_detents` or `enc_delta` changes in the expected direction | | |
| Encoder A/B | One detent counter-clockwise | `enc_detents` or `enc_delta` changes in the expected direction | | |
| Encoder switch | Press and release | `enc_sw` changes and returns to idle | | |
| Back button | Press and release | `back` state changes and returns to idle | | |
| Home button | Press and release, if connected | `home` state changes and returns to idle | | |
| Radio-enable toggle | Toggle both positions | `radio_en` and `toggle_changes` update | | |
| Profile toggle | Toggle both positions | `profile` and `toggle_changes` update | | |

## Stop Conditions

- [ ] Encoder transitions increment without actuation
- [ ] Any active-low control is stuck active
- [ ] Toggle state is inverted from documented wiring
- [ ] `pins` output stalls or USB disconnects during interaction

## Pass / Fail

- Pass criteria met:
  - [ ] Yes
  - [ ] No
- Deviations / notes:

## Hand-off

- Stage gate status for [`docs/testing-plan.md`](../../docs/testing-plan.md):
  - [ ] Stage 3 can proceed
  - [ ] Hold and troubleshoot
