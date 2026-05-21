/*
 * WeAct Studio STM32WB55CGU6 Core Board Arduino STM32 variant.
 *
 * Based on the generated STM32WB55CGU6 Arduino STM32 variant shape, with
 * WeAct-specific LED and button pins from the vendor BLE example.
 */
#if defined(ARDUINO_WEACT_WB55CGU6)
#include "pins_arduino.h"

const PinName digitalPin[] = {
  PA_0,   // D0/A0
  PA_1,   // D1/A1
  PA_2,   // D2/A2
  PA_3,   // D3/A3
  PA_4,   // D4/A4
  PA_5,   // D5/A5
  PA_6,   // D6/A6
  PA_7,   // D7/A7
  PA_8,   // D8/A8
  PA_9,   // D9/A9
  PA_10,  // D10
  PA_11,  // D11 USB_DM
  PA_12,  // D12 USB_DP
  PA_13,  // D13 SWDIO
  PA_14,  // D14 SWCLK / header P1.13
  PA_15,  // D15 header P2.6
  PB_0,   // D16 header P2.2
  PB_1,   // D17 header P2.3
  PB_2,   // D18 header P1.2
  PB_3,   // D19 header P2.7
  PB_4,   // D20 header P2.8
  PB_5,   // D21 header P2.9
  PB_6,   // D22 header P2.10 / USART1_TX
  PB_7,   // D23 header P2.11 / USART1_RX
  PB_8,   // D24 header P1.14 / I2C1_SCL
  PB_9,   // D25 header P1.15 / I2C1_SDA
  PC_13,  // D26 on-board KEY
  PC_14,  // D27 LSE OSC32_IN
  PC_15,  // D28 LSE OSC32_OUT
  PE_4,   // D29 on-board blue LED
  PH_3    // D30 BOOT0
};

const uint32_t analogInputPin[] = {
  0,  // A0, PA0
  1,  // A1, PA1
  2,  // A2, PA2
  3,  // A3, PA3
  4,  // A4, PA4
  5,  // A5, PA5
  6,  // A6, PA6
  7,  // A7, PA7
  8,  // A8, PA8
  9   // A9, PA9
};

#endif /* ARDUINO_WEACT_WB55CGU6 */
