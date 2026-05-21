# GPS Bring-Up Template

Use this template during first GPS UART/NMEA checks.

## Session

- Date / time (UTC):
- Operator:
- Environment:
- Antenna/path present:
- Artifact manifest:

## Commands Executed

- [ ] `pio run -e stm32wb55_hardware_gates`
- [ ] `pio run -e stm32wb55_hardware_gates -t upload`
- [ ] `pio device monitor`
- [ ] `sensors`
- [ ] `status`
- [ ] `telemetry`

## Golden Assertions

- [ ] `sensors` shows `gps compile=yes configured=yes`
- [ ] `parser=on`
- [ ] UART parser reports byte progress with live module (if powered and pinned)
- [ ] `gpsFix=no` at startup is accepted
- [ ] `gpsFix` transitions to `yes` in a valid RF environment
- [ ] `gps_satellites` rises above zero before sustained lock claim
- [ ] `fixAgeMs` values move from `none` to finite after valid fix

## Failure Triggers

- [ ] No UART progress while power and antenna are present
- [ ] `gpsSamples` remain `0/0` long after verified module power
- [ ] Fix remains `none` without expected improvements after a planned outdoor check

## Evidence to Capture

- Raw `sensors` snapshots at 1, 5, and 10 minutes
- Fix and satellite trend notes
- Any observed baud/line anomalies from `sensors`/serial

## Pass / Fail

- Pass criteria:
  - [ ] met
  - [ ] not met
- Stage transition decision:
  - [ ] Proceed to DX-LR03 modem smoke
  - [ ] Hold for UART/power review
