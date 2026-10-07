# RFM69H Radio Module Notes

Inventory contains two SPI RFM69H modules:

| SKU | Inventory status | Frequency profile | Firmware env |
| --- | --- | --- | --- |
| `RFM69H-868MHZ-SPI` | on hand, 1 available | 868 MHz | `stm32wb55_rfm69h_868` |
| `RFM69H-915MHZ-SPI` | on hand, 1 available | 915 MHz | `stm32wb55_rfm69h_915` |

The user request said `RFM68H`; the local inventory records and module document
identify these as `RFM69H`.

## Current Firmware Support

The first implementation is a guarded bring-up scaffold:

- selects the 868 MHz or 915 MHz profile at compile time;
- configures safe RFM69H chip-select, reset, and DIO0 GPIO states;
- initializes the shared SPI bus only in the RFM69H build environments;
- reads the RFM69H `RegVersion` register (`0x10`) as a non-transmit hardware
  probe and expects `0x24` before setting `moduleProbePassed=yes`;
- reports `rfm69hCompileEnabled`, `rfm69hProfile`, `rfm69hFrequencyHz`, and
  `rfm69hSpiInitialized` from the `radio` command;
- keeps `transmitGuarded=yes` and does not perform over-the-air transmit.

Register-level RFM69H probing, packet mode, encryption policy, bitrate/deviation
selection, and receive/transmit tests are intentionally deferred until hardware
is wired and the regional test plan is selected.

## Wiring Plan

| RFM69H signal | STM32WB55 pin | Notes |
| --- | --- | --- |
| `VCC` | `3V3` | Do not use 5 V. |
| `GND` | `GND` | Common ground with MCU. |
| `SCK` | `PB3` / SPI1 SCK | Shared with ST7789 SCK if display is fitted. |
| `MOSI` | `PA7` / SPI1 MOSI | Shared with ST7789 MOSI if display is fitted. |
| `MISO` | `PA6` / SPI1 MISO | Conflicts with encoder switch in the v0.1 plan. |
| `NSS` / `CS` | `PA15` | Separate RFM69H chip select; mutually exclusive with DX-LR03 `AUX`. |
| `RESET` | `PB0` | Active high reset; firmware keeps it low after a short reset pulse. Mutually exclusive with DX-LR03 `M0`. |
| `DIO0` | `PB1` | First interrupt/status line; mutually exclusive with DX-LR03 `M1`. |
| `ANT` | band-matched antenna | Use an 868 MHz antenna for the 868 module and a 915 MHz antenna for the 915 module. |

Fit only one radio family during initial bring-up: DX-LR03 UART modem or one
RFM69H SPI module. Do not wire the DX-LR03 and RFM69H control pins at the same
time with the current pin plan.

## Build and Smoke Commands

```bash
pio run -e stm32wb55_rfm69h_868
pio run -e stm32wb55_rfm69h_915
```

After flashing one of those environments, run:

```bash
scripts/hardware-smoke.sh --port /dev/ttyACM0 --commands tests/rfm69h-radio-commands.tsv
```

Expected radio status fields:

```text
rfm69hCompileEnabled=yes
rfm69hProfile=868MHz
rfm69hFrequencyHz=868000000
rfm69hSpiInitialized=yes
rfm69hVersion=0x24
transmitGuarded=yes
```

For the 915 MHz environment, `rfm69hProfile=915MHz` and
`rfm69hFrequencyHz=915000000`.

## Regional Note

The 915 MHz module aligns with the existing Canada/Ontario 902-928 MHz planning
track. The 868 MHz module is useful for build coverage and non-transmit bench
work, but it is not part of the Canada/Ontario transmit plan unless a separate
legal/regulatory review explicitly permits the intended use.
