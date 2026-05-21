# GUI Design Notes (First Pass)

This document defines the first-pass local UI for the STM32WB55 LoRa endpoint
based on the current firmware behavior and current input controls.

## Scope and target

- Display: ST7789 SPI TFT, `240x280` (portrait).
- Coordinate system: origin `(0,0)` at the top-left, `x = 0..239`, `y = 0..279`.
- Assumed render flow in current firmware:
  - `showBootMessage()` at boot.
  - `showTelemetry()` every `lora_device::kDisplayIntervalMs` (3 s).
  - `showStatus()` every `lora_device::kRadioIntervalMs` (15 s).
- The current implementation is diagnostic-first (`240x280` exists as scaffolding:
  frame counters, color test blocks, and serial mirror), not a polished UI.

## First-pass screen layout (portrait)

Use a simple 5-zone layout for the first pass:

- **Zone A — Header band** (`y: 0-23`, `x: 0-239`)
  - Mode/identity/status summary.
  - `status` mode string, radio state, uptime.
  - Future text/alert area.
- **Zone B — Core telemetry grid** (`y: 24-176`, `x: 0-239`)
  - Four block cards: battery, temperature, pressure, and GPS validity.
  - First row uses `y: 24-99`; second row uses `y: 101-176`.
- **Zone C — Input + profile strip** (`y: 178-227`, `x: 0-239`)
  - Back/home/toggle/encoder state indicators while present.
- **Zone D — Footer / action strip** (`y: 229-279`, `x: 2-237`)
  - Current screen state label + short hint text.

### Coordinate reference (first-pass visual plan)

```
240x280 (portrait)
+-----------------------------------+ 0
| A: Header                        |
|                                   |
|-----------------------------------| 24
| B: Telemetry / main panel         |
|                                   |
|                                   |
|-----------------------------------| 178
| C: Inputs & profile strip         |
|-----------------------------------| 229
| D: Footer / state strip           |
+-----------------------------------+ 279
```

## Screen states

### 1) Boot state

- **Trigger:** first successful `setup()` completion and `displayService.showBootMessage()`.
- **Current implementation intent:** emits `DISPLAY(STUB): boot complete | ...` over
  serial and, when hardware path is active, renders the block dashboard once.
- **Required content:**
  - Firmware version/build line.
  - Current mode.
  - Placeholder for startup progress / fault banner.

### 2) Telemetry state

- **Trigger:** default periodic renderer (`showTelemetry` at 3 s interval).
- **Current implementation intent:** serial `DISPLAY(...): ...` line includes
  `telemetry`, `btn_back`, `btn_home`, `radio_enable`, `profile`.
- **Planned on-screen composition:**
  - Header: mode + radio + uptime.
  - Core cards: packet/telemetry counters, sensor readiness, fix flag.
  - Input strip: back/home and encoder/toggle indicators.

### 3) Status state

- **Trigger:** periodic `showStatus` call (15 s interval) to expose transport
  state and visual heartbeat.
- **Current implementation intent:** serial `DISPLAY(...): status seq=... radio=...`.
- **Planned on-screen composition:** compact status line with mode, radio state,
  last TX status, and "last render" timestamp.
- **Visual intent (first-pass):** status block regions make pass/fail and
  online/offline obvious at a glance.

### 4) Menu state (planned)

- **Trigger:** menu affordance (encoder press from telemetry state).
- **Planned composition:** selector list and cursor row.
- **First-pass expectations:**
  - One menu depth (`root` + `detail` for the selected mode).
  - One confirm action and one cancel action.
- **Note:** not yet fully implemented in this firmware milestone.

### 5) Dialog/alert state (planned)

- **Trigger:** invalid command paths, long fault streak, or hardware hard-stop.
- **Planned composition:** full-screen warning strip with dismiss action.
- **First-pass expectation:** clear severity, one-line reason, and safe retry text.

## State transitions (documented behavior)

| Current state | Input/navigation action | Next state |
| --- | --- | --- |
| Boot | Initial run timeout or command refresh | Telemetry |
| Telemetry | Rotate encoder | Move selection cursor (within telemetry panels) |
| Telemetry | Encoder press | Menu (planned) |
| Telemetry | Back button | Menu exit/cancel action (planned) |
| Telemetry | Home button | Top-level summary reset (planned) |
| Menu | Rotate encoder | Next/prev entry |
| Menu | Encoder press | Enter detail/commit |
| Menu | Back button | Telemetry |
| Any screen | No input + periodic tick | Refresh own screen content |

## Input navigation assumptions

- **Primary control:** rotary encoder (A/B quadrature + push switch).
- **Supporting keys:** back/home buttons for cancel and fast reset.
- **Stateful toggles:** radio-enable and profile-mode toggles are treated as latched
  inputs (read each loop and reflected in header/input strip and radio logic).
- **Assumed navigation timing:** UI events are polled once per loop and rendered
  on existing display cadence (3 s/15 s split while menu mode is not implemented).
- **Encoder semantics for first-pass:** treat one physical detent as one cursor
  step after debounce; multiple detents produce repeated step events.
- **Backstop behavior:** if navigation is not determinable, remain in current
  state and keep rendering heartbeat/status output.

## No-hardware validation plan

Use this sequence before wiring the panel to confirm frame-state design is ready:

1. `pio run` and `scripts/host-build-smoke.sh` pass.
2. Check that `display_service` reports `geometry=240x280` and frame counters
   exist in `display` diagnostics.
3. Confirm main loop cadence outputs in monitor (or logs):
   - `telemetry`/`status` serial cadence near `kTelemetryIntervalMs`,
     `kDisplayIntervalMs`, `kRadioIntervalMs`.
4. Confirm serial output includes `DISPLAY( ... )` records from both `showTelemetry`
   and `showStatus` paths.
5. Confirm `display` command prints:
   `lastView`, `frameCounter`, `st7789...` counters, and `pinsConfigured`.
6. In no-hardware mode (`renode`/stub), verify stable boot/loop output and no
   assert-like crashes.

Acceptance at this stage: consistent state labels (`boot`, `telemetry`, `status`),
monotonic frameCounter, and no regression in serial command help/`status` output.

## Hardware visual acceptance checks

Use these checks once ST7789 is connected and powered (before radio or long sensor
exercise):

- Backlight comes on after boot, then remains stable.
- No hard flicker/garble at boot and during periodic updates.
- `boot` frame visible at least once after startup.
- Telemetry state is readable without overlap and not clipped at edges.
- `status` corner indicator appears with predictable color changes
  (online/offline distinction per implementation status).
- SPI CS/D/C/RST pins hold sane values and do not show rapid bus collisions
  with radio activity.
- A fixed 4 s+ stable display with no reset loop and no frozen frame.
- Panel orientation matches expected portrait orientation (`240` wide, `280` tall).

If any of the checks fail, keep the firmware in software-only diagnostics and
return to cabling/pin review before enabling advanced menu/state transitions.

## Acceptance artifact checklist (per build milestone)

- **Software milestone:** docs + diagnostics only, boot/telemetry/status states are
  named and captured in serial logs.
- **Hardware first visual milestone:** all checks in this document are visually
  clear; `display` command reflects expected counters and pin states.
- **Next milestone:** add menu/dialog screens and transition rules while preserving
  current telemetry/status loops as fallback safe states.
