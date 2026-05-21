# DX-LR03-900 Radio Module Notes

Requested part: `DX-LR03-900T22D`

Vendor package checked:
<https://github.com/DX-SMART/LORAMODULE/blob/main/DX-LR03-900Mhz%20Development%20%26%20User%20Information.rar>

The downloaded package currently contains `DX-LR03-900T30D` technical and serial
application guides. Treat these as DX-LR03-900 family references until the exact
T22D sheet is available.

## Design-Relevant Facts From Vendor Package

- Radio family: DX-LR03-900, ASR6601-based LoRa serial module.
- Interface model: UART serial module with AT command mode and transparent,
  fixed-point, and broadcast transfer modes.
- Default UART: 9600 bps, 8 data bits, no parity, 1 stop bit.
- Main pins: `M0`, `M1`, `RXD`, `TXD`, `AUX`, `VCC`, `GND`.
- `M1`: sleep/wake control in vendor docs; low enters sleep, high wakes.
- `AUX`: module RF/buffer status; high means receive/send accumulation or busy,
  low means idle/available.
- Supply: 4.0-5.5 V in the T30D manual; docs elsewhere in this project use
  4.5-5.5 V conservatively until the exact T22D sheet is checked.
- IO supply range in the T30D manual: 3.0-3.7 V, typical 3.3 V.
- Frequency range in the T30D manual: 850-931 MHz, 500 kHz channel steps.
- T30D manual lists up to +28 dBm output. Do not assume this value for the
  requested T22D until its exact datasheet is checked.
- Vendor note: connect an antenna before powering the module.

## Firmware Implications

This module replaces the old SX1278 SPI/RadioLib design. The firmware should
treat the radio as a guarded serial peripheral:

- STM32 talks to the module over UART.
- Module RF configuration will be performed through AT commands after command-mode
  probing is enabled and hardware bring-up has verified the UART path.
- Firmware must check `AUX` before sending data or AT commands.
- Current firmware defers discovery queries and should report `no AT write queued`
  for `radio at`, `radio help`, and `radio config` during no-hardware prep.
- No direct SX127x register access or RadioLib driver path is expected.
- TX remains disabled until `ConfigService` has a signed Ontario/Canada profile.
- `HardwareSerial.begin()` only proves the MCU UART was opened. A future module
  probe must enter command mode with `+++`, send `AT`, and receive a valid `OK`
  response before the module can be reported online.

## Current Wiring Allocation

| DX-LR03 signal | STM32WB55 pin | Direction | Notes |
| --- | --- | --- | --- |
| `VCC` | 5 V rail | Power | Stable 5 V supply, sized for TX bursts. |
| `GND` | GND | Power | Common ground with MCU. |
| `RXD` | `PA2` / LPUART1 TX | MCU to module | 3.3 V logic. |
| `TXD` | `PA3` / LPUART1 RX | Module to MCU | 3.3 V logic. |
| `M0` | `PB0` | MCU to module | Reserved/custom mode control if fitted. |
| `M1` | `PB1` | MCU to module | Sleep control, default high/wake. |
| `AUX` | `PA15` | Module to MCU | Busy/status input. |

## Inventory Status

Inventory queries for `DX-LR03-900T22D`, `LoRa 900`, and `SX1262` returned no
matching local records. Treat the module as user-specified hardware until it is
added to inventory.
