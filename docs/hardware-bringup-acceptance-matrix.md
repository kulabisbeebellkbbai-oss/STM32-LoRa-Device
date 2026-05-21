# Hardware Bring-Up Acceptance Matrix

Use this matrix to keep staged bring-up evidence consistent. Each staged test writes to
the relevant template in `tests/templates/` and records pass/fail notes back in
[`docs/testing-plan.md`](testing-plan.md) before advancing.

## Stage Matrix

| Stage | Subsystem | Precondition | Primary assertions | Stop conditions | Evidence template |
|---|---|---|---|---|---|
| 0 | First power-on and transport | `platformio` build artifacts from either `stm32wb55_bringup` or `stm32wb55_hardware_gates`; board USB + ST-Link access | Boot banner, `help` response, and command path availability within 10 s of upload | No boot banner, repeated USB disconnects, repeated flash lock errors after clean re-plugs, missing USB enumerate | [`tests/templates/bringup-first-power-on-template.md`](../tests/templates/bringup-first-power-on-template.md) |
| 1 | USB baseline | No external peripherals required; baseline firmware (`pio run`) | Stable serial command response cadence, no reset loops, `status`/`version` valid | Boot timeout, `assert` in monitor log, monitor command stalls | [`tests/templates/bringup-usb-template.md`](../tests/templates/bringup-usb-template.md) |
| 2 | ST7789 display | Display module attached, pins per `docs/pin-plan.md` | `display` reports ST7789 enabled path and counters increment, no obvious frame corruption | `st7789SpiInitialized=no`, no frame counter movement, visible backlight/polarity faults | [`tests/templates/bringup-display-template.md`](../tests/templates/bringup-display-template.md) |
| 3 | Inputs/controls | encoder + buttons/toggles connected | `pins` counters change deterministically after manual interaction | Stuck toggles, invalid encoder transitions increasing without actuation | [`tests/templates/bringup-controls-template.md`](../tests/templates/bringup-controls-template.md) |
| 4 | Sensor bus (BMP280/ADXL345/DHT11 + I2C scan) | Sensor rails stable and I2C wiring to configured pins | `sensors` and `i2cscan` show expected `active` and sample counters | Scan hangs/floods, sensors active with no samples over windows | [`tests/templates/bringup-sensors-template.md`](../tests/templates/bringup-sensors-template.md) |
| 5 | GPS UART/NMEA | GPS module on `PB7/PB6`, 9600 baud baseline | `sensors` reports parser state and eventually `fix=yes` in field signal | Parser shows no UART progress while power and antenna are present | [`tests/templates/bringup-gps-template.md`](../tests/templates/bringup-gps-template.md) |
| 6 | DX-LR03 modem boot + safety gate | DX-LR03 power and UART wiring present, TX profile/region approved for bench mode | Modem pins configured and guarded TX path (`transmitGuarded=yes`) before any TX command | Brownout after modem power-up, `serialInitialized=no` with expected wiring, `pinsConfigured=no` | [`tests/templates/bringup-dx-lr03-template.md`](../tests/templates/bringup-dx-lr03-template.md) |
| 7 | RF safety | Regional documentation and approvals present; no active over-air intent | `radio.at` only used for diagnostics; no field transmit until explicit RF approval; `transmitGuarded` remains true during smoke | Any external transmit command used without RF sign-off, uncontrolled antenna exposure | [`tests/templates/bringup-rf-safety-template.md`](../tests/templates/bringup-rf-safety-template.md) |
