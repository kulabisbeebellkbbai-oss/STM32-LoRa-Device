# LoRa End Point

Firmware project for an STM32WB55-based portable LoRa endpoint with display, GPS,
basic environmental sensing, and pluggable radio application modes.

## Hardware Baseline

- MCU: STM32WB55CGU6, initially tested over USB on the WeAct STM32WB55 Core Board.
- Display: 1.69 inch ST7789 SPI IPS TFT, 240x280.
- LoRa radio: DX-SMART DX-LR03-900T22D (DX-LR03-900 family, ASR6601), 850-931 MHz UART AT modem.
- Sensors: GY-291 ADXL345, BMP280, DHT11, GY-NEO6MV2 GPS.
- Inputs: rotary encoder, back/home buttons, 12-key keypad, and two toggles for
  normal standalone use.
- Power: USB for initial testing. Battery testing waits until the enclosure stage.
- Battery path: 523450 3.7 V 1000 mAh LiPo through MT3608 boost module set to 5 V.

## Firmware Goals

Minimum application targets:

- Meshtastic-compatible firmware path.
- MeshCore-compatible firmware path.
- LoRaWAN application path.
- Standalone local device UI and sensor telemetry screen.
- Later USB-connected host interface, enabled after standalone hardware tests pass.

## Current Setup

This repository contains a first flash-prep firmware version:

- PlatformIO project metadata is present in `platformio.ini`, pinned to
  `ststm32@19.6.0`.
- `env:stm32wb55_bringup` is the conservative flash target. `env:stm32wb55_hardware_gates`
  compiles the display, radio, sensor, GPS, and I2C scan feature gates and lets
  the sensor service attempt hardware reads when modules are connected.
- Firmware entry point is in `src/main.cpp`, with service modules under `src/`
  and `include/`.
- The first firmware build prints a serial boot banner, accepts `help`,
  `status`, `mode`, `apps`, `app [name]`, `telemetry`, `display`, `radio`,
  `radio at`, `radio help`, `radio config`, `config`, `config signoff`, `sensors`,
  `i2cscan`, `pins`, and `version` commands, and uses safe stub services until hardware
  modules are wired.
- `config` exposes the Canada/Ontario 902-928 MHz profile skeleton and keeps
  `txAllowed=false`; `config signoff` records a placeholder without unlocking RF
  transmit.
- Hardware, flashing, pin-planning, wiring, RF caveats, and smoke-test notes are
  in `docs/`.
- The first build target uses the repo-local `weact_wb55cgu6` board definition
  copied from the Aquaponics Controller firmware setup.

## Documentation for hardware build

Before connecting the modules, review:

- `docs/pin-plan.md` — practical STM32 pin assignments and signal directions.
- `docs/wiring-diagrams.md` — practical wiring diagrams and module wiring notes.
- `docs/flashing-prep.md` — pre-flash checklist and upload/monitor sequence.
- `docs/testing-plan.md` — no-hardware build checks and hardware smoke tests.
- `docs/gui-design.md` — first-pass 240x280 screen layout and validation assumptions.
- `docs/renode-simulation.md` — Renode boot-banner simulation scope and command.
- `docs/lora-regional-caveats.md` — regional legal and RF test constraints.
- `docs/dx-lr03-900-module.md` — active DX-LR03-900 radio module notes.

## Commands

```bash
scripts/host-build-smoke.sh
```

Build the default STM32WB55 firmware with a short temporary build path. This is
the preferred no-hardware check when the repository path contains spaces.

```bash
pio run
```

Build the default STM32WB55 firmware directly.

```bash
pio run -e stm32wb55_hardware_gates
```

Compile the hardware-gated paths without uploading or probing hardware.

```bash
scripts/renode-smoke.sh
```

Build the Renode simulation firmware and verify the boot banner in the local
Renode stub model.

```bash
pio run -t upload
```

Upload once the programmer/debug connection is confirmed.

```bash
pio device monitor
```

Open a serial monitor for USB bring-up logs.

For a quick board-level wiring overview before hardware assembly, start with:

```bash
ls docs assets
```
