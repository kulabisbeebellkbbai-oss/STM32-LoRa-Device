# Display Bring-Up Template

Use this template for ST7789 wired checks before enabling any radio tests.

## Session

- Date / time (UTC):
- Operator:
- Board env:
- Build environment:
- Monitor port / connection:
- Artifact manifest:

## Wiring and Safety Preconditions

- ST7789 VIN/GND power and SPI lines connected per [pin-plan](../../docs/pin-plan.md)
- Backlight polarity confirmed (no direct shorts to exposed rails)
- Other peripherals that are not yet staged are disconnected unless intentionally active

## Commands Executed

- [ ] `pio run -e stm32wb55_hardware_gates`
- [ ] `pio run -e stm32wb55_hardware_gates -t upload`
- [ ] `pio device monitor`
- [ ] `display`
- [ ] `status`

## Golden Assertions

- [ ] `display enabled=yes` with expected `st7789CompileEnabled`
- [ ] `pinsConfigured=yes`
- [ ] `renderStub`/`stubOnly` match preplanned stage
- [ ] `frameCounter` increments on boot/status/telemetry cycles
- [ ] `lastView` advances across boot + status render states
- [ ] `st7789Commands`, `st7789DataBytes`, `st7789SpiWrites` increment when panel is attached

## Visual Check

- [ ] Panel initializes without garble
- [ ] Boot/status transitions visible and readable
- [ ] No sustained full-frame inversion or flicker lock

## Failure Triggers

- [ ] No frame rendering after repeated upload and reset
- [ ] `st7789SpiInitialized` remains `no`
- [ ] Persistent backlight/screen polarity fault symptoms (inverted strip, dark full panel, smoke smell)
- [ ] UART or power symptoms suggest contention with radio lines

## Pass / Fail

- Pass criteria:
  - [ ] met
  - [ ] not met
- Notes / corrective actions:
- Safe to proceed to controls/sensors: [ ] yes [ ] no
