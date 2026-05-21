# Physical Bring-Up Runbook

## Scope and assumptions

This runbook is a staged hardware validation path for the STM32WB55 LoRa endpoint. It is written for a host with `pio run`, `pio run -t upload`, and `pio device monitor` available, plus a confirmed ST-Link/SWD upload path.

It assumes the default wiring from [pin-plan.md](pin-plan.md), the first-pass GUI state model in [gui-design.md](gui-design.md), and command surface in `src/main.cpp`.

Build presets used in this runbook:

- `pio run` and `pio run -t upload` for the conservative baseline (`env:stm32wb55_bringup`).
- `pio run -e stm32wb55_hardware_gates` and `pio run -e stm32wb55_hardware_gates -t upload` for staged hardware diagnostics (`display`, `sensors`, `gps`, `radio`, `i2cscan`, and input commands).

For stage tracking, use:

- [`docs/hardware-bringup-acceptance-matrix.md`](hardware-bringup-acceptance-matrix.md)
- [`tests/templates/bringup-first-power-on-template.md`](../tests/templates/bringup-first-power-on-template.md)

## Staged order

Use this order for first hardware validation:

1. USB-only baseline.
2. Display attach and diagnostics.
3. Controls (encoder/button/toggles).
4. Sensor bus modules.
5. GPS module.
6. Radio module (RF init only, no over-the-air transmit).
7. Power confidence.

At each stage, record pass/fail and stop before moving forward on any hard-stop condition.

## Shared baseline checks for every stage

1. Build target from repository root:

```bash
pio run
```

2. Hardware-gated build check:

```bash
pio run -e stm32wb55_hardware_gates
```

3. Capture stage artifact context:

```bash
scripts/build-artifact-manifest.sh --build --output artifacts/build-artifact-manifest.json
```

4. Flash the environment intended for the current stage after each electrical change:

```bash
pio run -t upload
# or, for hardware-gated stages:
pio run -e stm32wb55_hardware_gates -t upload
```

5. Open the USB CDC serial monitor in a second terminal:

```bash
pio device monitor
```

6. Confirm serial command path:

```bash
help
```

Expected output begins with:

```text
commands:
  help          show this message
  status        show mode, telemetry freshness, and radio state
  mode [name]   print current mode or set by value
  apps          print supported application profiles
  app [name]    print profile details and request gated selection
  telemetry     print latest telemetry snapshot
  display       show display diagnostics state
  radio         show radio diagnostics state
  sensors       show sensor and GPS diagnostics state
  i2cscan       run compile-gated sensor I2C scan
  pins          print planned input pin reads
  version       print firmware version
```

If none of these appear, check serial speed (`lora_device::kSerialBaudRate = 115200`), USB CDC wiring, and boot message stability before continuing.

## Golden boot assertions (required before any stage progression)

Before the first hardware stage, capture evidence that all of these are true:

- `LoRaWB LoRa Endpoint boot`
- `Firmware version: ...`
- `Firmware: firmware_name=LoRaWB LoRa Endpoint version=... hardware=STM32WB55 serial_console=1`
- `Hardware drivers are compile-gated; RF transmit remains guarded.`
- `Type 'help' for serial commands.`
- `help` command returns:
  - `help`, `status`, `telemetry`, `display`, `radio`, `sensors`, `i2cscan`, `pins`, `version`
- `status` command responds with `mode=`, `telemetry_seq=`, and `uptime_ms=`.
- No boot-loop pattern in first 60 seconds.

Record Stage 0 and Stage 1 outcomes in
[`tests/templates/bringup-first-power-on-template.md`](../tests/templates/bringup-first-power-on-template.md)
and
[`tests/templates/bringup-usb-template.md`](../tests/templates/bringup-usb-template.md).

---

## Stage 1 — USB-only baseline

Evidence template: [`tests/templates/bringup-usb-template.md`](../tests/templates/bringup-usb-template.md)

### Goal
Validate USB boot, console stability, and stub timing without any external peripherals.

### Commands

```bash
pio run
pio run -t upload
pio device monitor
```

Then send:

```text
help
status
telemetry
pins
version
```

### Expected output

Boot sequence on monitor should include:

```text
LoRaWB LoRa Endpoint boot
Firmware version: 0.1.0
Firmware: firmware_name=LoRaWB LoRa Endpoint version=0.1.0 serial_baud=115200 telemetry_interval_ms=5000 display_interval_ms=3000 status_interval_ms=10000 radio_interval_ms=15000 hardware=STM32WB55 serial_console=1
Hardware drivers are compile-gated; RF transmit remains guarded.
Type 'help' for serial commands.
```

Expect periodic logs after the first 1–3 cycles at default timing:
- `telemetry seq=...` every `5000` ms from `telemetry` status print path.
- `DISPLAY(STUB): ...` every `3000` ms.
- `status mode=...` every `10000` ms.
- `radio(stub) simulated_tx seq=...` every `15000` ms in the baseline mock radio
  path. This is a no-RF firmware diagnostic.

From `status` in stub baseline:

```text
status mode=standalone telemetry_seq=... radio_online=no radio_initialized=yes radio_available=no radio_mocked=yes outputs=... uptime_ms=...
```

From `pins` in stub baseline:

```text
pins encoderA=... encoderB=... enc_sw=... back=... home=... radio_toggle=... profile_toggle=...
pinstates back=0/1 home=0/1 encA=... encB=... encSw=... radio_en=... profile=...
input ...
```

### Stop conditions (hard stop)

- No boot banner in 10 s after upload.
- USB monitor disconnects repeatedly or shows no CDC output.
- `pio run -t upload` fails with lock/flash errors after two retried clean re-plugs.
- Hardware not recognized by USB on two separate ports/cables.

### Pass condition

- Stable 3/5/10/15-second cadence present on logs with no reset loop.
- `help` command is responsive and returns all listed commands.
- No `assert` strings in monitor output.

**Hardware required:** USB only.

---

## Stage 2 — Display validation

Evidence template: [`tests/templates/bringup-display-template.md`](../tests/templates/bringup-display-template.md)

### Goal
Add ST7789 and verify shared SPI pin safety, CS discipline, and render diagnostics.

### Commands

```bash
pio run -e stm32wb55_hardware_gates
pio run -e stm32wb55_hardware_gates -t upload
pio device monitor
```

Then run:

```text
display
status
```

### Expected output

Display diagnostics should show the ST7789 command path when hardware gates are active. This confirms firmware-side SPI writes, not that the physical panel accepted them. In boot you should see one of:

- `DISPLAY: ST7789 scaffold initialized (command-level, non-driver)`
- or, in the conservative baseline env only: `DISPLAY: LORA_DISPLAY_ST7789 compile-time path active, but display service is stubOnly`

From `display` diagnostics:

```text
display enabled=yes stubOnly=no renodeSimulation=no renderStub=no st7789CompileEnabled=yes st7789Initialized=yes st7789SpiInitialized=yes pinsConfigured=yes frameCounter=...
```

Counter fields should move from zero once display frames are emitted.

### Validation checks

- At least one visible frame action should occur for boot and periodic status/telemetry calls if the panel wiring and backlight polarity are correct.
- Observe `st7789Commands`, `st7789DataBytes`, `st7789SpiWrites`, and `lastView` moving from defaults.
- Cross-check visible frame behavior with screen states in [gui-design.md](gui-design.md), including boot/telemetry/status transitions and input strip visibility.
- Keep modem sequencing conservative: compare numeric `radio` output fields (`pins uart_tx=... uart_rx=... m0=... m1=... aux=...`) against `include/device_config.h` before RF wiring, and verify there is no startup contention with SPI display on power transitions.

### Stop conditions

- Any hard lockups, blank boot after re-flash, or `st7789SpiInitialized=yes` never reaching true after repeated successful uploads.
- Screen visibly inverted/garbled indicates power/backlight polarity or SPI pin mismatch.
- `frameCounter` does not increase after `display` boot/status/telemetry traffic.

### Pass condition

- Stable output plus coherent frame rendering in firmware logs and physical panel.

**Hardware required:** Display module and 3.3 V rail.

---

## Stage 3 — Controls

Evidence template: [`tests/templates/bringup-controls-template.md`](../tests/templates/bringup-controls-template.md)

### Goal
Validate control input wiring, debouncing counters, and toggle state persistence.

### Commands

```bash
pio run -e stm32wb55_hardware_gates
pio run -e stm32wb55_hardware_gates -t upload
pio device monitor
```

Then run:

```text
pins
status
```

Then perform one interaction for each connected control:
- Rotate encoder by one detent both directions.
- Press encoder switch.
- Press back/home button.
- Toggle radio-enable and profile toggles.

### Expected output

From `pins`, repeated outputs should start showing stable state transitions and counters in final `input ...` block:

```text
input ... enc_detents=... enc_delta=... enc_trans=... enc_invalid=... toggle_changes=...
```

The `status` line should reflect any active inputs in `outputs=` (for example `radio` / `home` / `back` / `sw`).

### Stop conditions

- Any input stuck high/low indefinitely after debounce window (wiring pull-up or wiring fault).
- Encoder detents jump unpredictably, high invalid transitions (`enc_invalid` climbing continuously).
- Wrong pin mapping versus expected values in `pins encoderA=...`.

### Pass condition

- Inputs change from idle to changed states and remain deterministic.

**Hardware required:** At least encoder and any buttons/toggles attached.

---

## Stage 4 — Sensor bus

Evidence template: [`tests/templates/bringup-sensors-template.md`](../tests/templates/bringup-sensors-template.md)

### Goal
Verify I2C bus health and each enabled sensor driver path.

### Commands

```bash
pio run -e stm32wb55_hardware_gates
pio run -e stm32wb55_hardware_gates -t upload
pio device monitor
```

Then run:

```text
sensors
i2cscan
status
telemetry
```

### Expected output

Sensor status baseline with all gates enabled should include:

```text
sensor ready=yes stubOnly=no ... sampleCounter=... stubSamples=... pins_i2c_sda=... sck=... dht11=... gpsRx=... gpsTx=... gpsBaud=9600
```

- With no I2C device present, `active=no` for those paths, with sample counts incrementing only where probes are seen.
- With BMP280 present, watch `bmp280=... active=yes` and `samples` increase.
- With ADXL345 present, watch `adxl345=... active=yes` and `samples` increase.
- `dht11=... configured=yes` and sample counters should only increase when frame timing and data are valid.
- `i2cScan compile=yes requested=yes|no ran=yes|no scans=...` plus `bmp280Hits` / `adxlHits` and `SENSOR_I2C_SCAN: 0x76` style lines.

### Stop conditions

- `i2cscan` hangs or floods continuously.
- Sensor bus returns `active=yes` but no sample counters over repeated telemetry windows.
- `sensors` shows `gps compile=yes` with no UART byte progress for prolonged periods while module is powered and wiring is believed correct.

### Pass condition

- `i2cscan` completes quickly and detects expected addresses.
- At least one enabled sensor demonstrates valid sample counters after 60 s.

**Hardware required:** I2C modules and pull-up plan per `pin-plan.md`.

---

## Stage 5 — GPS UART stream

Evidence template: [`tests/templates/bringup-gps-template.md`](../tests/templates/bringup-gps-template.md)

### Goal
Validate NMEA receive parsing and fix-state reporting without relying on RF or transmit.

### Commands

```bash
pio run -e stm32wb55_hardware_gates
pio run -e stm32wb55_hardware_gates -t upload
pio device monitor
```

Then run:

```text
sensors
status
telemetry
```

### Expected output

Sensor status should show:

```text
gps compile=yes configured=yes parser=on fix=no sat=0 fixAgeMs=none lat=... lon=...
```

Once module and satellites are available, expect:

```text
gpsFix=yes (telemetry)
gps_satellites increases
```

in telemetry and sensor output.

### Stop conditions

- `sensors` repeatedly reports `parser=on` with unchanged `gpsSamples=0/0` after module power-on and antenna present, indicating wiring/baud issues.
- Fix never improves from `none` with good signal outdoors for extended window; capture and review wiring and antenna orientation.

### Pass condition

- Serial parser remains healthy with stable memory behavior and eventually transitions `fix` from `no` to `yes` in expected environment.

**Hardware required:** GPS module on `PB7/PB6` and 3.3 V serial power.

---

## Stage 6 — Radio bring-up (no-air-time)

Evidence template: [`tests/templates/bringup-dx-lr03-template.md`](../tests/templates/bringup-dx-lr03-template.md) and
[`tests/templates/bringup-rf-safety-template.md`](../tests/templates/bringup-rf-safety-template.md)

### Goal
Validate radio init diagnostics, pin configuration, and mock/guard path without on-air transmissions.

### Commands

```bash
pio run -e stm32wb55_hardware_gates
pio run -e stm32wb55_hardware_gates -t upload
pio device monitor
```

Then run:

```text
radio
status
```

### Expected output

Boot and command path should show modem guard behavior:

```text
radio: DX-LR03 UART ready; module probe pending; TX guarded
```

If UART initialization fails, stop and diagnose wiring or pin configuration instead:

```text
radio: DX-LR03 UART init not available
```

From `radio` command:

```text
radio initialized=yes available=no mocked=yes dxLr03CompileEnabled=yes serialInitialized=yes moduleInitAttempted=yes moduleProbePassed=no serialBaud=9600 auxBusy=<yes|no> transmitGuarded=yes ... txPackets=0 lastSequence=0
```

On periodic cadence, watch for either `radio(stub) simulated_tx ...` (mock mode) or
`radio dx-lr03 guarded tx blocked ...` after a future module probe exists.
In the current firmware, `moduleProbePassed=no` remains expected because opening
the MCU UART is not proof that the module is present. The `radio at` command is
deferred and should report `no AT write queued`; do not treat it as a live
module probe until the command-mode probe is explicitly enabled after hardware
bring-up.

### Stop conditions

- Immediate brownout after radio power-up.
- `radio pinsConfigured=no` after boot.
- `serialInitialized=no` with expected hardware should stop before any field test
  and restart with UART/pin integrity checks.
- Any firmware path that queues AT writes before the documented command-mode
  probe is enabled should stop RF testing and be reviewed before continuing.

### Pass condition

- Pins are configured, status fields coherent, and no physical/firmware faults on modem startup.

**Hardware required:** DX-LR03-900T22D module and matching antenna stub/placeholder.

**No-air-time constraint:** Do not add any external commands or GPIO overrides to force TX/RX yet. Keep legal/regional approvals in place and follow `lora-regional-caveats.md` before enabling any over-the-air test.

---

## Stage 7 — Power confidence

Evidence template: [`tests/templates/bringup-first-power-on-template.md`](../tests/templates/bringup-first-power-on-template.md)

### Goal
Confirm sustained standby operation and avoid power-induced faults before battery integration.

### Commands

```bash
pio run -e stm32wb55_hardware_gates
pio run -e stm32wb55_hardware_gates -t upload
pio device monitor
```

Then run a 30-minute soak at default cadence and monitor:

- `status`
- `telemetry`
- `sensors`
- `radio`

### Validation checklist

- Keep logs stable with no unexpected reset bursts.
- Confirm input states remain coherent under load changes.
- Confirm display/sensor/radio diagnostics continue incrementing at expected intervals.
- Confirm no hard fault markers and no USB disconnection from inrush spikes.

### Pass condition

- 30 minutes of continuous run with stable uptime counters and no repeated brownout/reset behavior.
- If battery path is added, repeat this stage in bench conditions for inrush and low-voltage behavior before enclosure close.

**Hardware required:** full assembled stack on reliable bench supply or USB + battery path under test.

## Pre-decision checklist for next stage

Only continue when these are true:

1. Commands are stable and responsive in the staged stage for 10+ minutes.
2. No hard-stop condition has triggered.
3. Expected diagnostic fields for that stage moved from default and show deterministic behavior.
4. `testing-plan.md` stage entry is updated with observed pass/fail notes.
