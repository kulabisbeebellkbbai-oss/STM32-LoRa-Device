/*
 * WeAct Studio STM32WB55CGU6 Core Board Arduino STM32 variant.
 *
 * Pin evidence:
 * - WeAct vendor BLE example: LED = PE4, KEY = PC13.
 * - WeAct V1.0 schematic exposes PA0-PA10, PA14, PA15, PB0-PB9, PE4,
 *   BOOT0, VBAT, VDD5V, VDD33, and GND on the two 2.54 mm headers.
 */
#pragma once

/*----------------------------------------------------------------------------
 *        Pins
 *----------------------------------------------------------------------------*/

#define PA0                     PIN_A0
#define PA1                     PIN_A1
#define PA2                     PIN_A2
#define PA3                     PIN_A3
#define PA4                     PIN_A4
#define PA5                     PIN_A5
#define PA6                     PIN_A6
#define PA7                     PIN_A7
#define PA8                     PIN_A8
#define PA9                     PIN_A9
#define PA10                    10
#define PA11                    11
#define PA12                    12
#define PA13                    13
#define PA14                    14
#define PA15                    15
#define PB0                     16
#define PB1                     17
#define PB2                     18
#define PB3                     19
#define PB4                     20
#define PB5                     21
#define PB6                     22
#define PB7                     23
#define PB8                     24
#define PB9                     25
#define PC13                    26
#define PC14                    27
#define PC15                    28
#define PE4                     29
#define PH3                     30

// Alternate pins number
#define PA7_ALT1                (PA7 | ALT1)
#define PB8_ALT1                (PB8 | ALT1)
#define PB9_ALT1                (PB9 | ALT1)

#define NUM_DIGITAL_PINS        31
#define NUM_ANALOG_INPUTS       10

// On-board LED pin number
#define LED                     PE4
#define LED1                    LED
#define LED_BUILTIN             LED

// On-board user button
#define SW1_BTN                 PC13
#define USER_BTN                SW1_BTN

// SPI definitions
#define PIN_SPI_SS              PA4
#define PIN_SPI_SS1             PA15
#define PIN_SPI_SS2             PB2
#define PIN_SPI_SS3             PE4
#define PIN_SPI_MOSI            PA7
#define PIN_SPI_MISO            PA6
#define PIN_SPI_SCK             PB3

// I2C definitions
#define PIN_WIRE_SDA            PB9
#define PIN_WIRE_SCL            PB8

// Timer definitions
#define TIMER_TONE              TIM16
#define TIMER_SERVO             TIM17

// UART definitions
#define SERIAL_UART_INSTANCE    1
#define PIN_SERIAL_RX           PB7
#define PIN_SERIAL_TX           PB6

/* Extra HAL configuration */
#define PREFETCH_ENABLE         1U

// Reserve the last flash page for Arduino EEPROM emulation.
#define EEPROM_FLASH_PAGE_NUMBER 255

/*----------------------------------------------------------------------------
 *        Arduino objects - C++ only
 *----------------------------------------------------------------------------*/

#ifdef __cplusplus
  #define SERIAL_PORT_MONITOR   Serial
  #define SERIAL_PORT_HARDWARE  Serial1
#endif
