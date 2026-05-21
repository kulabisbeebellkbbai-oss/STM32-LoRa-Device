# Hardware Baseline

## Core Electronics

| Function | Selected part | Inventory status | Notes |
| --- | --- | --- | --- |
| MCU board | STM32WB55CGU6 WeAct STM32WB55 Core Board | on_order, 3 ordered | USB-C board target using repo-local PlatformIO board support. |
| Display | 1.69 inch ST7789 SPI TFT, 240x280 | on_order, 2 ordered | SPI display for local status, menus, and radio mode state. |
| LoRa radio | DX-SMART DX-LR03-900T22D (`DX-LR03-900` family) | on_order, 2 ordered | ASR6601-based 850-931 MHz UART LoRa module with AT-command modes (`transparent`, `fixed`, `broadcast`). 4.5-5.5 V power, 3.3 V I/O. Default UART: 9600 8N1. |
| Accelerometer | GY-291 ADXL345 | on_hand, 5 available | I2C preferred to reduce SPI bus load. |
| Pressure/temp | BMP280 | on_hand, 5 available | I2C preferred. |
| Temp/humidity | DHT11 | on_hand, 5 available | Use for basic environmental telemetry. |
| GPS | GY-NEO6MV2 | on_hand, 1 available | UART NMEA source. |
| Battery | 523450 3.7 V 1000 mAh LiPo | on_order, 10 ordered | Battery integration deferred until case design. |
| Boost power | MT3608 boost module, 5 V output | not_in_inventory from query | Treat as a purchase/verification item unless already physically present. |

## Bring-Up Policy

1. Start with USB power only.
2. Verify MCU serial logs.
3. Add display.
4. Add I2C environmental sensors.
5. Add GPS UART.
6. Add DX-LR03-900 UART radio path with module power, control pins, and AT-mode smoke checks.
7. Add input controls.
8. Enable USB-connected host interface only after standalone operation is stable.
9. Add battery and enclosure power validation last.

## Board Definition

Assumption:

- +28 dBm is listed in some DX-LR03 documentation for the `DX-LR03-900T30D`; for this build, the requested `DX-LR03-900T22D` TX power has not been independently verified and is treated as TBD.

The PlatformIO board target is `weact_wb55cgu6`, using:

- `boards/weact_wb55cgu6.json`
- `variants/WEACT_WB55CGU6/`

This board definition was copied from the Aquaponics Controller STM32 firmware
setup. It targets STM32WB55CGU6 at 64 MHz with 1 MB flash and 256 KB RAM, uses
the Arduino STM32 core, and defaults to ST-Link upload/debug.

## Radio Module Source

The active radio design uses the DX-LR03-900 module family notes in
[dx-lr03-900-module.md](dx-lr03-900-module.md). This is a UART/AT module path,
not the prior SX1278 SPI/RadioLib path.
