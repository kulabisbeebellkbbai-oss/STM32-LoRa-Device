# Renode Investigation: STM32WB55 Arduino/PlatformIO Firmware (LoRa Device)

Date: 2026-05-20
Scope: Local checks only for this repository.

## Current Status

Renode is installed locally and the repo now has a passing no-hardware smoke path
through `scripts/renode-smoke.sh`.

Local toolchain observed during setup:

- Renode version: `Renode v1.16.1.17033`
- `renode-test`: available through the local Renode install

The installed Renode tree does not include a native STM32WB55 board model. The
repo therefore uses a local stub platform for firmware boot and metadata
regression, not a complete electrical model of the production MCU.

## Implemented Test Path

The active Robot test is:

- [`renode/lora_device_boot.robot`](../renode/lora_device_boot.robot)

Run it through:

```bash
scripts/renode-smoke.sh
```

The smoke test builds `env:stm32wb55_renode`, starts the local Renode platform,
and asserts the firmware boot metadata:

```text
LoRaWB LoRa Endpoint boot
Firmware version: 0.1.0
Firmware: firmware_name=LoRaWB LoRa Endpoint ...
Hardware drivers are compile-gated; RF transmit remains guarded.
```

This is the canonical no-hardware boot-output regression until a fuller
STM32WB55 Renode model is added.

## Limitations

- The Renode platform is a stub sufficient for boot/metadata regression, not a
  substitute for SWD, USB CDC, ST7789, DX-LR03, GPS, sensor, or input electrical
  validation.
- Robot-driven serial command injection is not currently treated as reliable for
  the firmware `RenodeConsole` receive path. Command regression is covered by
  host unit tests and the hardware serial smoke harness.
- Hardware-gated behavior must still be validated on a real board before any RF,
  sensor, display, or GPS signoff.

## Next Renode Improvement

Add a board model or console bridge that can reliably drive the firmware command
RX path from Robot Framework. Once that is available, move a small subset of
`tests/hardware-smoke-commands.tsv` into Renode command-regression coverage.
