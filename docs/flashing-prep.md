# Flashing Prep Checklist

This checklist keeps STM32WB55 flash workflows deterministic and reduces random
`upload` failures during early board bring-up.

## Before You Flash

For the staged bring-up sequence and per-stage stop conditions, use:

- [Physical bring-up runbook](physical-bringup-runbook.md)

- Verify `pio run` currently succeeds in `stm32wb55_bringup`.
- Verify `pio run -e stm32wb55_hardware_gates` succeeds before enabling any
  compile-time hardware gate on a flashed build.
- Confirm board target in `platformio.ini` is `board = weact_wb55cgu6` and
  `env:stm32wb55_bringup` is the active target.

- Confirm the ST-Link/SWD transport is visible with whichever probe utility is
  installed on the host. `pio device list` is useful for serial ports, but it is
  not an SWD probe check.

```bash
command -v st-info >/dev/null && st-info --probe
command -v STM32_Programmer_CLI >/dev/null && STM32_Programmer_CLI -l
```

If neither command is installed, skip this check for the no-hardware build pass
and perform it before the first future flash session.

- Unplug/re-plug USB after flashing-related kernel hiccups and test cable changes.
- Keep the board on a stable 5 V USB source or known-good bench supply.
- If the board uses a power switch, connect USB before attaching radios.

## Recommended Flash Order

1. **Baseline firmware only**: `pio run -t upload`
2. Re-seat and open serial monitor:

```bash
pio device monitor
```

3. Confirm boot banner and version string.
4. Add hardware incrementally and repeat upload only if GPIO/ISR behavior
   changes in code.

## Serial/Upload Notes for ST-Link

- Default upload uses PlatformIO `stlink` debug protocol from `platformio.ini`.
- The diagnostic console is Arduino STM32 USB CDC generic `Serial`; GPS uses
  `Serial1` on `PB7`/`PB6`.
- The repo-local WeAct variant leaves USB `NOE` unassigned so USB CDC does not
  reconfigure `PA13`/SWDIO during the first-flash/debug path.
- If flash fails intermittently, stop all external peripherals and retry on a
  simple USB-only build.
- Verify board remains on 3.3 V logic (all modules listed are expected to be
  3.3 V compatible for direct pin level use).

## Recovery Steps

- Power cycle the board and retry upload once.
- If upload still fails:
  - switch to a known-good USB cable/port,
  - shorten SPI/I2C jumpers temporarily if radio/display are powered,
  - re-run `pio run` and only then `pio run -t upload`.
- If the upload tool claims flash locked or memory map errors, power-cycle the
  probe and re-run; if unresolved, do a full erase-first sequence on your ST-Link
  tooling flow.

## Post-Flash Sanity

- USB serial banner appears within 3–10 s on monitor.
- No immediate hardfault/reset loop in the first 60 s.
- Inputs do not need to be connected for initial flash validation.

## When to Pause

Pause firmware flashing and wire-check first when any of these occurs:

- repeated `pio device monitor` disconnections,
- unexpected current spike at 3.3 V rail,
- SPI pins are still unconnected but code reports pin assertions at runtime.
