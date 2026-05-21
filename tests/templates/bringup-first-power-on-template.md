# First Power-On Bring-Up Template

Use this template at first successful upload and any major reset/re-flash sequence.

## Session Header

- Date / time (UTC):
- Operator:
- Board / fixture id:
- Board rev:
- Repo path:
- Firmware path:
- Git branch:
- Git commit:
- Build manifest file:

## Evidence Assets

- Power-on log file:
- USB monitor command transcript:
- `pio run` output:
- `pio run -t upload` output:

## Build and Environment

- Environment used:
- Build directory:
- Build timestamp:
- Build status:

## Golden Power-On Assertions

- [ ] `LoRaWB LoRa Endpoint boot` appears within 10 s of reset
- [ ] `Firmware version:` present and expected
- [ ] `Firmware:` metadata includes `firmware_name=LoRaWB LoRa Endpoint`, `hardware=STM32WB55`, and `serial_console=1`
- [ ] `Hardware drivers are compile-gated; RF transmit remains guarded.` appears
- [ ] `Type 'help' for serial commands.` appears
- [ ] `help` command returns at least:
  - `help`, `status`, `telemetry`, `version`, `pins`, `display`, `sensors`, `i2cscan`, `radio`
- [ ] `status` shows `mode=` and `uptime_ms=` and no boot-loop chatter

## USB Health Check

- [ ] USB device enumerates after upload
- [ ] No immediate disconnect loop after power-up
- [ ] `version` command returns semantic version and branch/commit context if supported

## Pass / Fail

- Pass criteria met:
  - [ ] Yes
  - [ ] No
- Blocked conditions:
  - [ ] USB transport failure
  - [ ] Hard reset loop
- Deviations / notes:

## Hand-off

- Stage gate status for [`docs/testing-plan.md`](../../docs/testing-plan.md):
  - [ ] Stage A/Stage 1 can proceed
  - [ ] Hold and troubleshoot
