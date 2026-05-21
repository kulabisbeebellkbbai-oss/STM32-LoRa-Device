# Pin Plan (Practical Wiring Draft)

This map is intended for prototype wiring and keeps the current board resources
limited to STM32WB55CGU6 GPIO plus the shared buses below.

Pin names are WeAct header pin names (`PAx`/`PBx`) for the STM32WB55CGU6 and
assume 3.3 V signaling for all logic pins.

## Fixed Power Rails

- **3V3**: STM32WB55 `3V3` to all logic VCC pins.
- **5V**: DX-LR03-900T22D VCC requires 4.5-5.5 V. Do not power any MCU GPIO from
  5 V.
- **GND**: Common ground bus between MCU and every peripheral module.

## Recommended Pin Assignments (v0.1)

This draft uses only pins exposed by the repo-local WeAct STM32WB55CGU6 variant
and intentionally avoids native USB pins (`PA11`/`PA12`) and SWD pins
(`PA13`/`PA14`) so upload, debug, and serial logs remain available.

| Subsystem | STM32WB55CGU6 Pin | Peripheral Signal | Direction | Electrical note |
| --- | --- | --- | --- | --- |
| Power/clock | `PB3` (SPI1_SCK) | ST7789 `SCK` | MCU out | Display-only SPI clock |
| Power/clock | `PA7` (SPI1_MOSI) | ST7789 `SDA` | MCU out | Display-only SPI MOSI |
| Power/clock | not connected | ST7789 `SDO` (if fitted) | Peripheral in | Leave display MISO unused in this draft. |
| Radio | `PA2` (LPUART1_TX) | DX-LR03 `RXD` | MCU out | 9600 8N1 default, no flow control |
| Radio | `PA3` (LPUART1_RX) | DX-LR03 `TXD` | Peripheral in | Module response/input path at 3.3 V |
| Radio | `PB0` | DX-LR03 `M0` | MCU out | Reserved/custom mode control if fitted; default low in firmware. |
| Radio | `PB1` | DX-LR03 `M1` | MCU out | Sleep/wake control from vendor docs; firmware drives high/wake. |
| Radio | `PA15` | DX-LR03 `AUX` | Peripheral in | Module busy/tx/rx status |
| Display | `PA4` (SPI1_CS) | ST7789 `CS` | MCU out | Keep high during display idle states |
| Display | `PB4` | ST7789 `DC` | MCU out | Data/command select |
| Display | `PB5` | ST7789 `RST` | MCU out | Active low |
| Display | `PA8` | ST7789 `BL` | MCU out | Optional PWM |
| Sensors | `PB9` (I2C1_SDA) | BMP280/ADXL345 SDA | MCU I/O | 4.7k pull-up if no onboard pull-up |
| Sensors | `PB8` (I2C1_SCL) | BMP280/ADXL345 SCL | MCU out | 4.7k pull-up if needed |
| DHT11 | `PB2` | DHT11 data | MCU in/out | 4.7k pull-up to 3.3 V |
| GPS | `PB7` (USART1_RX) | GY-NEO6MV2 `TX` | Peripheral in | NMEA RX at MCU |
| GPS | `PB6` (USART1_TX) | GY-NEO6MV2 `RX` | MCU out | Optional command RX |
| Input | `PA0` | Encoder A | MCU in | Interrupt-capable where possible |
| Input | `PA1` | Encoder B | MCU in | Interrupt-capable where possible |
| Input | `PA6` | Encoder SW | MCU in | Active low with pull-up, debounce in SW |
| Input | `PC13` | Back button | MCU in | On-board user button, active low |
| Input | `PA9` | Home/status button | MCU in | Active low with pull-up |
| Input | `PA10` | Radio-enable toggle | MCU in | Read at boot + live |
| Input | `PA5` | Profile/power toggle | MCU in | Read at boot + live |
| USB | USB D+/D- (native) | Host / serial bring-up | MCU/USB peripheral | Keep enabled for logs |

`PB6`/`PB7` are reserved for GPS `Serial1`. `PA2`/`PA3` are assigned to the
DX-LR03 UART link and must stay clear of existing control signals.

## Shared-Bus Constraint

The ST7789 display uses SPI1 exclusively. The radio is now UART-based, so there
is no SPI bus arbitration between radio and display; keep both paths on short,
well-terminated leads and check startup timing once the module is attached.

## Pin Plan Risk Log

- Confirm with your exact WeAct board silk-screen mapping before soldering; board
  revisions can swap header pinout order on clones.
- If any pin conflicts with your enclosure wiring or existing power design, treat
  this file as a proposal and update the table in one pass with an updated
  revision comment.

Generated wire artifacts:

- [assets/lora-endpoint-pin-map.mmd](../assets/lora-endpoint-pin-map.mmd)
- [assets/lora-endpoint-architecture.mmd](../assets/lora-endpoint-architecture.mmd)
