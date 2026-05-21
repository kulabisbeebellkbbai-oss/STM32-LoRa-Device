# Testing Plan

## No-Hardware Checks

No-hardware checks validate build pipeline health before any wiring change reaches
PCB. Run these in priority order:

1. `scripts/host-build-smoke.sh` clean build, which covers:
   - `env:stm32wb55_bringup`
   - `env:stm32wb55_hardware_gates`
2. `scripts/build-artifact-manifest.sh --build` to capture non-flash artifact evidence for
   both environments before hardware stages.
3. `scripts/host-unit-tests.sh` parser + config invariants validation against
  `tests/usb-command-cases.tsv` without hardware. This also validates `apps` and
  `app [mode]` command parsing and conservative profile selection metadata.
4. Static pin-review against `docs/pin-plan.md` and first-pass GUI behavior in
   `docs/gui-design.md` before module soldering.
5. Confirm `docs/flashing-prep.md` checklist is complete.
6. Confirm regional/caveats review in `docs/lora-regional-caveats.md`.

## Golden Boot Assertions

For Stage A/Stage 1 validation, treat the following as hard acceptance checks:

- Boot line appears within 10 s of reset:
  - `LoRaWB LoRa Endpoint boot`
  - `Firmware version: <version>`
  - `Firmware: firmware_name=LoRaWB LoRa Endpoint version=<version> ... hardware=STM32WB55 serial_console=1`
  - `Hardware drivers are compile-gated; RF transmit remains guarded.`
  - `Type 'help' for serial commands.`
- `help` returns command table containing all mandatory entries:
  `help`, `status`, `telemetry`, `display`, `radio`, `sensors`, `i2cscan`, `pins`,
  `version`
- Baseline `status` includes `mode=`, `telemetry_seq=`, and `uptime_ms=`.
- No boot-loop marker such as repeated `Resetting...` without progress.
- `help` and `status` are each reachable on first connect.

Use [`tests/templates/bringup-usb-template.md`](../tests/templates/bringup-usb-template.md)
as the acceptance form for this section.

## Hardware Bring-Up Matrix

Use the staged matrix and stage-level pass/fail lock gates in
[`docs/hardware-bringup-acceptance-matrix.md`](hardware-bringup-acceptance-matrix.md).
Templates for each stage are in [`tests/templates`](../tests/templates/).

## Hardware Smoke Tests (Bring-Up Stages)

Smoke tests only run after that stage is fully wired and visually inspected.
The current firmware is a flashable first version with serial-backed diagnostics
and compile-gated hardware paths. The conservative bring-up target keeps services
stubbed; `env:stm32wb55_hardware_gates` enables active display, sensor, and
radio init paths for staged bench validation.

### Stage A — USB + Baseline MCU

1. Boot banner from serial (`pio device monitor`).
2. Re-attach and cycle USB connection cleanly.
3. Confirm no boot-loop warnings.
4. Run the serial `config` command after flashing to confirm the Canada/Ontario
   profile reports `txLocked=true`, `signedOff=false`, and `txAllowed=false`.

### Stage B — Display + Modem Coexistence

Status: partially executable; full visual and RF validation remains blocked on real ST7789 and DX-LR03-900 hardware.

1. Verify display bring-up diagnostics via the serial `display` command:
   - `enabled`, `stubOnly`, `renodeSimulation`, `renderStub`, `st7789CompileEnabled`, `pinsConfigured`, `frameCounter`, `lastRenderMs`, `lastView`
   - ST7789 counters now tracked when compile-gated path is active:
     `st7789Initialized`, `st7789SpiInitialized`, `st7789Commands`, `st7789DataBytes`, `st7789SpiWrites`,
     `st7789BootFrames`, `st7789StatusFrames`
   - confirm firmware reports display control pins (CS/DC/RST/BKLT) configured
     before hardware validation checks the electrical levels
   - `LORA_DISPLAY_ST7789` enables compile-time ST7789 command/write scaffolding.
     When not running in stub/renode mode, boot/status paths now emit minimal frame
     writes using ST7789 command/data primitives.
2. ST7789 boot/status pixel-block visibility and command/data counters.
3. Verify modem serial command-response path (AT `AT`/`AT+PARAM?`-style smoke checks, matching module capabilities).
4. Toggle radio enable switch and verify state changes are visible on screen.
5. Validate no simultaneous module-setup and display bursts on power/boot transitions.
6. Cross-check screen states and visual criteria with
   [gui-design.md](gui-design.md).

### Stage C — Sensor Subsystem

Status: partially executable. The firmware exposes `sensors` diagnostics and
compile-gated BMP280, ADXL345, DHT11, GPS, and `i2cscan` paths. Use
`env:stm32wb55_hardware_gates` when modules are wired; the conservative
`env:stm32wb55_bringup` target keeps sensor reads in stub mode.

1. Verify serial `sensors` diagnostics before module tests:
   - `stubOnly`, sensor compile/active states, planned pins, I2C addresses,
     GPS parser state, sample/failure counters, and I2C scan counters.
2. I2C bus scan for BMP280 and ADXL345 addresses.
   - Compile-time switch: `LORA_SENSOR_I2C_SCAN` (enabled in
     `env:stm32wb55_hardware_gates`, off in the default conservative bring-up env).
   - Serial command: `i2cscan`; it reports disabled/default state unless the
     compile-time switch is enabled and only marks `ran=yes` after an enabled
     scan path executes.
   - Run only when this switch is enabled and keep in mind it is optional for debug use.
3. BMP280 pressure/temp read every 5 s.
   - BMP280 compile-gate: `LORA_SENSOR_BMP280`; diagnostics counters track
     sample/fail and address detection.
4. ADXL345 ID and accel read.
   - ADXL345 compile-gate: `LORA_SENSOR_ADXL345`; diagnostics counters track
     sample/fail.
5. DHT11 stable frame cadence and reasonable range filtering.
   - DHT11 compile-gate: `LORA_SENSOR_DHT11`; cadence should be limited to the
     sensor-safe interval and frame decode should validate checksum plus temperature
     and humidity range.
6. Sensor timeout fallback behavior (one dropped frame then recover).

### Stage D — GPS

Status: partially executable. The firmware contains a compile-gated non-blocking
NMEA parser and exposes state through `sensors`; live validation is blocked until
the GPS module is wired.

1. NMEA stream decode (GPRMC/GPGGA) at 9600 or module default baud.
   - GPS compile-gate: `LORA_SENSOR_GPS` using non-blocking serial parse on
     `Serial1` pins (`PB7` RX, `PB6` TX). The USB diagnostics console uses
     Arduino STM32 USB CDC generic `Serial`, enabled by build flags, so it does
     not consume USART1.
2. Parse and expose minimal state (`lat`, `lon`, `satellites`, `fix`) from RMC/GGA.
3. UTC/time fields parse, valid fix status reported.
3. No unbounded UART buffer growth during no-fix state.

### Stage E — RF Smoke Window

Status: planned, blocked on real DX-LR03-900 transmit/receive path and regional
frequency approval.

1. Verify serial `radio` diagnostics before RF hardware tests:
   - `pinsConfigured`, `dxLr03CompileEnabled`, `serialInitialized`,
     `moduleInitAttempted`, `moduleProbePassed`, `serialBaud`, `transmitGuarded`
   - `moduleProbePassed=no` is expected in the current firmware. The `radio at`
     command is intentionally deferred and should report `no AT write queued`
     until the DX-LR03 command-mode probe is explicitly enabled after hardware
     bring-up.
   - `radio(stub) simulated_tx` is a no-RF firmware diagnostic emitted by the
     mock path; it is not an over-the-air transmission.
   - discovery-only query support: `radio help` and `radio config` are currently
     guarded/deferred. Do not treat them as module communication until the
     command-mode probe is enabled in firmware.
   - confirm firmware reports UART modem pins and control pins before hardware
     validation checks electrical levels.
2. Point-to-point loopback with nearest lab-range peers only.
3. Keep power and duty cycle in the lowest safe test profile.
4. Confirm packet counter and retries in the local UI.
5. Stop after first successful end-to-end exchange.

### Stage F — Controls

Status: partially executable after flashing. The current firmware samples the
planned active-low inputs and reports edge/transitions, encoder detent/delta
diagnostics, and last transition time through the `pins` command via the new
`InputService` state output.

1. Encoder, back, and home buttons report through the serial `pins` command.
2. Radio-enable and profile toggles latch at boot and while running.
3. Keypad route reports `keypad_configured=no` until an I2C expander or direct
   matrix pin plan is selected; after that decision, all keys must report once.

### Stage G — Power Confidence

1. Final smoke: run one complete sensor/UI/radio loop for 30 minutes on USB.
2. Validate no repeated brownout resets.
3. Defer battery stress until enclosure thermal and charging paths are verified.

## Test Assets

- [assets/testing-no-hardware.mmd](../assets/testing-no-hardware.mmd)
- [assets/testing-hardware-smoke.mmd](../assets/testing-hardware-smoke.mmd)

## Regional and Legal RF Caveats

- Validate the frequency plan and max EIRP against local law before any over-the-air
  LoRa tests.
- Start with no-air-time and low-power bench checks; document approvals before field
  tests.
- Keep the board on a non-conductive bench for first RF tests.

Any change from this plan should be reflected here before hardware execution.

For staged hardware sequence details, hardware-required checkpoints, and hard-stop conditions,
continue to [physical-bringup-runbook.md](physical-bringup-runbook.md).
