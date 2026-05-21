# Renode Simulation

This project includes a host-only Renode smoke test for the first firmware
version. It validates that a Renode-specific build of the firmware reaches
Arduino `setup()` and prints the boot banner over the modeled USART1.

## Reusable Smoke Test Helper

`scripts/renode-smoke.sh` is now a reusable Renode helper script for any
PlatformIO MCU project that has:

- a PlatformIO environment with a Renode-compatible build, and
- a Robot Framework test that reads the firmware path and platform path from
  environment variables.

All key inputs are overrideable with environment variables.

```bash
RENODE_PROJECT_NAME=my_project \
PLATFORMIO_RENODE_ENV=my_board_renode \
RENODE_REPL_SOURCE=renode/my_board.repl \
RENODE_ROBOT_FILE=renode/my_project_boot.robot \
./scripts/renode-smoke.sh
```

## Tooling

`scripts/renode-smoke.sh` expects `pio` and `renode-test` on `PATH`, or explicit
`PIO_BIN` and `RENODE_TEST_BIN` environment variables. If your Renode install
requires a specific Robot CSS file or Renode binary directory, set `RENODE_CSS`
or `RENODE_BIN_DIR`.

## Run the Smoke Test

```bash
scripts/renode-smoke.sh
```

### What the script does (LoRa defaults)

1. Builds `env:${PLATFORMIO_RENODE_ENV:-stm32wb55_renode}` with PlatformIO.
2. Exports the ELF path through `RENODE_ELF` and `LORA_RENODE_ELF`, and the
   platform path through `RENODE_PLATFORM` and `LORA_RENODE_PLATFORM`.
3. Runs `${RENODE_ROBOT_FILE:-renode/lora_device_boot.robot}` through
   `renode-test`.

## Environment Variables

Copy this script and set whichever overrides fit your project:

- `PIO_BIN` (`pio` from `PATH` by default)
- `RENODE_TEST_BIN` (`renode-test` from `PATH` by default)
- `RENODE_CSS` (unset by default)
- `RENODE_BIN_DIR` (unset by default)
- `RENODE_BIN_NAME` (`renode`)
- `RENODE_PROJECT_NAME` (`lora_device`)
- `RENODE_PROJECT_SLUG` (`${RENODE_PROJECT_NAME}` with path-unsafe characters
  replaced by `_`)
- `PLATFORMIO_BUILD_DIR` (`/tmp/pio_${RENODE_PROJECT_SLUG}_build`)
- `PLATFORMIO_RENODE_ENV` (`stm32wb55_renode`)
- `RENODE_REPL_SOURCE` (`${PROJECT_ROOT}/renode/stm32wb55cg_stub.repl`)
- `RENODE_REPL_DIR` (`/tmp/${RENODE_PROJECT_SLUG}_renode`)
- `RENODE_REPL_COPY_MODE` (`dir`; use `file` for a single `.repl`, or `none`
  when Renode can load the original path directly)
- `RENODE_ROBOT_FILE` (`${PROJECT_ROOT}/renode/lora_device_boot.robot`)
- `RENODE_WORKDIR` (`${PROJECT_ROOT}`)
- `RENODE_ELF_VARIABLE_NAME` (`RENODE_ELF`)
- `RENODE_PLATFORM_VARIABLE_NAME` (`RENODE_PLATFORM`)

For reuse in another project, `${RENODE_PROJECT_NAME}`,
`${PLATFORMIO_RENODE_ENV}`, `${RENODE_REPL_SOURCE}`, and
`${RENODE_ROBOT_FILE}` usually cover the required project-specific inputs.
New Robot tests should prefer `%{RENODE_ELF}` and `%{RENODE_PLATFORM}`. The
`LORA_RENODE_*` aliases are kept for older project-specific tests.

By default the script copies the entire directory containing the selected
`.repl` file into `/tmp`. This keeps sibling files and relative platform
includes intact while avoiding Renode path parsing problems when a workspace
path contains spaces.

## Simulation Scope

The Renode `v1.16.1` release used here does not include a complete STM32WB55
board model. The project therefore uses `renode/stm32wb55cg_stub.repl`, a
minimal Cortex-M4F platform with flash, RAM, USART1, GPIO, basic timers, and
placeholder register blocks for STM32 startup code.

To keep the production firmware path intact, `env:stm32wb55_renode` defines
`RENODE_SIMULATION`. That build flag:

- skips the real STM32WB55 clock-tree setup in the local Arduino variant,
- uses a small direct USART1 console so Renode can observe boot output without
  depending on the full Arduino `HardwareSerial` bring-up path,
- intentionally excludes the USB CDC flags used by physical bring-up builds.

## Not Simulated Yet

- USB CDC serial behavior.
- DX-LR03-900 UART modem behavior.
- ST7789 display behavior.
- BMP280, ADXL345, DHT11, and GPS devices.
- STM32WB dual-core wireless stack behavior.
- Real ST-Link upload/debug behavior.
- Serial RX command injection through the direct simulation console.

Those remain hardware or future-model tests. Treat this smoke test as a boot and
serial-output regression gate, not as proof of peripheral hardware behavior.
