#pragma once

#include <Arduino.h>

namespace lora_device {

constexpr uint32_t kSerialBaudRate = 115200;
constexpr uint32_t kBootDelayMs = 250;
constexpr uint32_t kTelemetryIntervalMs = 5000;
constexpr uint32_t kStatusIntervalMs = 10000;
constexpr uint32_t kRadioIntervalMs = 15000;
constexpr uint32_t kDisplayIntervalMs = 3000;
constexpr uint32_t kInputDebounceThreshold = 3;

struct PinConfig {
  // Logical pin assignments from the v0.1 wiring plan. These stay within the
  // pins exposed by the repo-local WEACT_WB55CGU6 Arduino variant and avoid
  // native USB and SWD pins so flashing and serial logging remain stable.
  static constexpr uint32_t kDxLr03BaudRate = 9600;
  static constexpr uint8_t kRadioUartTx = PA2;  // MCU TX -> DX-LR03 RXD
  static constexpr uint8_t kRadioUartRx = PA3;  // MCU RX <- DX-LR03 TXD
  static constexpr uint8_t kRadioModeControl = PB0;  // DX-LR03 M0/custom IO if fitted
  static constexpr uint8_t kRadioSleepControl = PB1;  // DX-LR03 M1, high=wake
  static constexpr uint8_t kRadioAux = PA15;

  static constexpr uint8_t kDisplaySpiSck = PIN_SPI_SCK;
  static constexpr uint8_t kDisplaySpiMosi = PA7;
  static constexpr uint8_t kDisplayCs = PA4;
  static constexpr uint8_t kDisplayDc = PB4;
  static constexpr uint8_t kDisplayReset = PB5;
  static constexpr uint8_t kDisplayBacklight = PA8;

  static constexpr uint8_t kGpsRx = PIN_SERIAL_RX;
  static constexpr uint8_t kGpsTx = PIN_SERIAL_TX;

  static constexpr uint8_t kDht11Data = PB2;
  static constexpr uint8_t kBmp280I2cSda = PIN_WIRE_SDA;
  static constexpr uint8_t kBmp280I2cScl = PIN_WIRE_SCL;
  static constexpr uint8_t kAdxl345I2cSda = PIN_WIRE_SDA;
  static constexpr uint8_t kAdxl345I2cScl = PIN_WIRE_SCL;

  // Planned UI controls
  static constexpr uint8_t kEncoderA = PA0;
  static constexpr uint8_t kEncoderB = PA1;
  static constexpr uint8_t kEncoderSwitch = PA6;
  static constexpr uint8_t kBackButton = PC13;
  static constexpr uint8_t kHomeButton = PA9;
  static constexpr uint8_t kRadioEnableToggle = PA10;
  static constexpr uint8_t kProfileToggle = PA5;
};

}  // namespace lora_device

#if defined(RENODE_SIMULATION)
class RenodeConsole : public Print {
 public:
  void begin(uint32_t baud);
  int available();
  int read();
  size_t write(uint8_t value) override;
};

extern RenodeConsole renodeConsole;

  #define LORA_DEVICE_CONSOLE renodeConsole
#else
  #define LORA_DEVICE_CONSOLE Serial
#endif
