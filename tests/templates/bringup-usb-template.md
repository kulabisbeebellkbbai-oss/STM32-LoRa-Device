# USB Baseline Bring-Up Template

Use this template for `Stage 1` USB-only baseline from
[`docs/physical-bringup-runbook.md`](../../docs/physical-bringup-runbook.md).

## Session

- Date / time (UTC):
- Operator:
- Repo / branch / commit:
- Build environment:
- Monitor port:
- Build artifact manifest:

## Steps Run

- [ ] `pio run`
- [ ] `pio run -t upload` (or environment-specific upload command)
- [ ] `pio device monitor`
- [ ] Serial command set sent:
  - `help`
  - `status`
  - `telemetry`
  - `pins`
  - `version`

## Golden Assertions for USB Baseline

- [ ] `help` output lists core command set from baseline firmware
- [ ] `telemetry` cadence present (~5000 ms)
- [ ] `DISPLAY(STUB):` cadence present (~3000 ms)
- [ ] `status mode=` cadence present (~10000 ms)
- [ ] `radio(stub) simulated_tx` cadence present (~15000 ms) in baseline run
- [ ] No `assert` tokens in logs over first 3 full cycles
- [ ] No `No boot banner` within 10 s after upload

## Capture Checklist

- [ ] Initial 60-second boot log captured
- [ ] Command transcript attached
- [ ] Serial line rate confirmed at 115200
- [ ] Baseline status fields captured:
  - `radio_online`
  - `radio_initialized`
  - `radio_available`
  - `radio_mocked`
  - `telemetry_seq`

## Decisions

- Pass criteria:
  - [ ] met
  - [ ] not met
- Next stage unlocked:
  - [ ] Stage 2 (Display)
  - [ ] Hold
- Notes / blocking findings:
