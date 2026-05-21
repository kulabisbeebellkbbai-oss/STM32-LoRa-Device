# Repository Guidelines

## Project Structure & Module Organization

Keep firmware source in `src/`, shared headers in `include/`, tests and test notes in `tests/`, assets in `assets/`, and contributor or hardware notes in `docs/`. Keep `.agents/` for public agent workflow notes and keep private/local agent metadata out of the repository. The current firmware target is an STM32WB55 LoRa endpoint with display, sensors, GPS, and later USB host-interface support.

## Build, Test, and Development Commands

- `pio run`: build the default STM32WB55 PlatformIO firmware skeleton.
- `pio run -t upload`: upload once the programmer/debug connection is confirmed.
- `pio device monitor`: open the serial monitor for USB bring-up logs.

## Coding Style & Naming Conventions

Use C++ for firmware code and keep modules small. Prefer service-style names for hardware slices, such as `RadioService`, `SensorService`, `DisplayService`, `InputService`, and `UsbCommandService`. Keep hardware pin choices documented in `docs/pin-plan.md` before using them in code.

## Testing Guidelines

Add tests with new behavior and mirror the source layout where practical. Name tests after the behavior they verify. Run `pio run` before handing off changes. For hardware changes, update `docs/testing-plan.md` with the smoke test used and keep battery testing deferred until USB-powered standalone hardware is stable.

## Commit & Pull Request Guidelines

Use Conventional Commits v1.0.0 for all commits: `<type>[optional scope]: <description>`. Prefer `feat`, `fix`, `docs`, `test`, `refactor`, `build`, `ci`, and `chore`; mark breaking changes with `!` or a `BREAKING CHANGE:` footer. Pull requests should include a short summary, test results, and relevant screenshots for UI changes.

For manager-thread coordination workflows, use [.agents/manager-workflow.md](.agents/manager-workflow.md).
