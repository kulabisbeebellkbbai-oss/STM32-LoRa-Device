# DX-LR03 Bring-Up Template

Use this template for modem init and guarded-path validation.

## Session

- Date / time (UTC):
- Operator:
- Environment:
- Module revision:
- Artifact manifest:

## Commands Executed

- [ ] `pio run -e stm32wb55_hardware_gates`
- [ ] `pio run -e stm32wb55_hardware_gates -t upload`
- [ ] `pio device monitor`
- [ ] `radio`
- [ ] `status`
- [ ] `radio at` confirms deferred/no-write behavior in the current firmware

## Golden Assertions

- [ ] `radio` shows `dxLr03CompileEnabled=yes`
- [ ] `serialInitialized=yes` after boot
- [ ] `pinsConfigured=yes` for `uart_tx`, `uart_rx`, `m0`, `m1`, `aux`
- [ ] `moduleInitAttempted=yes` once command path is invoked
- [ ] `transmitGuarded=yes` during bring-up and no forced RF activity
- [ ] `moduleProbePassed=no` before and after `radio at` in the current firmware
- [ ] `radio at` reports `no AT write queued`
- [ ] `status` shows coherent `radio_initialized`, `radio_available`, and `radio_mocked` fields

## Failure Triggers

- [ ] Immediate brownout on module power-up
- [ ] `radio pinsConfigured=no` after upload
- [ ] `serialInitialized=no` with expected correct wiring
- [ ] Any AT write is queued before the documented command-mode probe is enabled
- [ ] Unexpected or continuous `AUX` state not matching module docs

## Evidence to Capture

- `radio` command output at first boot
- `radio at` command output showing deferred/no-write behavior
- `status` snapshots before/after module probe attempt
- Current draw trend during modem init

## Pass / Fail

- Pass criteria:
  - [ ] met
- [ ] not met
- Hold reason:
