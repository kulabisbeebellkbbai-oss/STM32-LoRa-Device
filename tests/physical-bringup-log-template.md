# Physical Bring-Up Log Template

## Session

- Date:
- Operator:
- Host:
- Repo path:
- Commit / revision:
- Hardware target:
- Environment:

## Preflight Script Run

- `scripts/first-flash-preflight.sh` run: [ ] pass / [ ] fail
- Start time:
- End time:

### Preflight outcomes

| Check | Result | Notes |
| --- | --- | --- |
| PlatformIO availability | | |
| Documentation presence | | |
| Host build smoke (`pio run`) | | |
| Renode smoke (`renode-test`) | | |
| Build environment(s) used | | |

### Commands suggested by preflight output

- [ ] `st-info --probe`
- [ ] `STM32_Programmer_CLI -l`
- [ ] `pio run -t upload`
- [ ] `pio device monitor`
- [ ] `pio run -e stm32wb55_hardware_gates -t upload`
- [ ] Hardware probe checks from `docs/flashing-prep.md`

## Hardware Checks (before first flash)

- USB cable and power path:
- ST-Link/SWD probe identified:
- Probe tool used:
- Any existing firmware on board:
- Serial port observed:
- Initial monitor output observed:
- First boot banner:
- Reset behavior observed:

## Next actions

- [ ] Initial flash command executed
- [ ] Monitor log captured and linked
- [ ] Pins verified against `docs/pin-plan.md`
- [ ] Hardware smoke tests logged in `docs/testing-plan.md`
- [ ] Deviations from checklist captured:
