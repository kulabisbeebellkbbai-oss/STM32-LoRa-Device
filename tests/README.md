# Tests

Start with PlatformIO builds, executable host-side tests, and hardware smoke
tests. Extend the host tests as packet formatting, telemetry serialization, and
menu state transitions move into platform-independent modules.

## Host-side Build Checks (No Hardware)

- Quick non-hardware smoke check:
  - `scripts/host-build-smoke.sh`
  - default environments: `stm32wb55_bringup` and `stm32wb55_hardware_gates`
- Renode boot-banner simulation:
  - `scripts/renode-smoke.sh`
- Reusable Renode helper for other PlatformIO MCU projects:
  - `RENODE_PROJECT_NAME=my_project PLATFORMIO_RENODE_ENV=my_board_renode RENODE_REPL_SOURCE=renode/my_board.repl RENODE_ROBOT_FILE=renode/my_boot.robot scripts/renode-smoke.sh`
- Direct build equivalent:
  - `pio run -e stm32wb55_bringup`
  - `pio run -e stm32wb55_hardware_gates`

Current expectations:
- Script succeeds with `pio` present on `PATH` or with `PIO_BIN` pointing at a
  PlatformIO CLI path.
- Build is reproducible and does not attempt upload or serial probing.
- The hardware-gated build compiles display/radio/sensor/GPS/I2C scan paths but
  still does not prove connected peripheral behavior.
- DX-LR03 radio status semantics: a hardware-gated build may report
  `serialInitialized=yes`, but `moduleProbePassed` must remain `no` until
  `radio at` receives an `OK` response. Do not use UART initialization alone as
  online proof.
- USB command expectations are captured in `tests/usb-command-cases.tsv` and
  executed by `scripts/host-unit-tests.sh`. The `input_c_escape` column uses
  C-style escapes such as `\t` for tab-delimiter cases.
- The command corpus now includes `config`, `config signoff`, `apps`, and `app`
  rows for regional lock-state/profile diagnostics.
- Renode smoke passes by building `env:stm32wb55_renode`, loading the ELF into
  the repo-local Renode stub platform, and matching the UART boot banner.

## Bring-Up Evidence Templates

Use these templates to collect stage evidence before advancing to the next stage:

- `tests/templates/bringup-first-power-on-template.md`
- `tests/templates/bringup-usb-template.md`
- `tests/templates/bringup-display-template.md`
- `tests/templates/bringup-controls-template.md`
- `tests/templates/bringup-sensors-template.md`
- `tests/templates/bringup-gps-template.md`
- `tests/templates/bringup-dx-lr03-template.md`
- `tests/templates/bringup-rf-safety-template.md`

## Planned host tests

- Protocol payload serializer unit tests (pure C++).
- CLI/boot banner/output helpers (if moved to platform-independent modules).
- Build-only checks for service-specific translation units once added.

## Host unit tests

Run pure host-side command-parser + config and profile checks:

```bash
scripts/host-unit-tests.sh
```

The parser expectations live in `tests/usb-command-cases.tsv` and are executed
without hardware. Add new command rows there and keep this test matrix aligned with
the command grammar in `UsbCommandService`.

## Hardware Serial Smoke Harness

Use these commands for host-visible smoke planning and serial-device checks:

- `scripts/hardware-smoke.sh --help` shows full usage.
- `scripts/hardware-smoke.sh --list` prints candidate serial ports from
  `scripts/serial-port-discovery.sh`.
- `scripts/hardware-smoke.sh --discover --dry-run` verifies command plan only
  (`tests/hardware-smoke-commands.tsv`) with no serial I/O.
- `scripts/hardware-smoke.sh --discover` runs the serial command smoke and checks
  expected output substrings.

Results are written under:

- `tests/results/hardware-smoke/<timestamp>/run.log`
- `tests/results/hardware-smoke/<timestamp>/serial.log`
- `tests/results/hardware-smoke/<timestamp>/commands.tsv`
- `tests/results/hardware-smoke/<timestamp>/summary.tsv`
