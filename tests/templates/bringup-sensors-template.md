# Sensor Bus Bring-Up Template

Use this template for sensor+I2C bring-up checks.

## Session

- Date / time (UTC):
- Operator:
- Environment:
- Sensor hardware attached:
  - BMP280
  - ADXL345
  - DHT11
- Artifact manifest:

## Commands Executed

- [ ] `pio run -e stm32wb55_hardware_gates`
- [ ] `pio run -e stm32wb55_hardware_gates -t upload`
- [ ] `pio device monitor`
- [ ] `sensors`
- [ ] `i2cscan`
- [ ] `status`
- [ ] `telemetry`

## Golden Assertions

- [ ] `sensors` output includes stable `ready=yes` and expected pin map
- [ ] `stubOnly` state matches intended hardware-gate staging
- [ ] `i2cScan compile=yes` for this stage
- [ ] `i2cScan requested=yes` and `ran=yes` after command
- [ ] `bmp280Samples` and/or `adxlSamples` increase when devices are present
- [ ] `dht11` shows bounded cadence and sane values when enabled
- [ ] Parser memory counters remain bounded in no-fix/no-sample states

## Evidence to Capture

- Addresses detected from `i2cscan`
- Per-sensor sample/fail counters at:
  - 1-minute mark
  - 5-minute mark
- `gps compile` state value when GPS module is absent

## Failure Triggers

- [ ] `i2cscan` hangs or floods output
- [ ] `active=yes` but sample counters do not change over repeated windows
- [ ] Bus reports valid pin config with repeated frame-level faults

## Pass / Fail

- Pass criteria:
  - [ ] met
  - [ ] not met
- Stage transition decision:
  - [ ] Stage 5 (GPS) can proceed
  - [ ] Rework I2C pull-ups / wiring
