#pragma once

#include <Arduino.h>
#include "app_modes.h"
#include "device_config.h"
#include "telemetry_model.h"

class SensorService {
 public:
  struct SampleCounters {
    uint32_t sampleCounter{0};
    uint32_t stubSamples{0};
    uint32_t bmp280Samples{0};
    uint32_t bmp280Failures{0};
    uint32_t adxl345Samples{0};
    uint32_t adxl345Failures{0};
    uint32_t dht11Samples{0};
    uint32_t dht11Failures{0};
    uint32_t gpsSamples{0};
    uint32_t gpsFailures{0};
    uint32_t i2cScans{0};
    uint32_t bmp280I2cHits{0};
    uint32_t adxl345I2cHits{0};
  };

  struct GpsState {
    bool hasFix{false};
    int satellites{0};
    unsigned long lastValidFixMs{0};
    float lastLatitude{0.0f};
    float lastLongitude{0.0f};
    bool parserEnabled{true};
  };

  static constexpr uint8_t kBmp280I2cAddressA = 0x76;
  static constexpr uint8_t kBmp280I2cAddressB = 0x77;
  static constexpr uint8_t kAdxl345I2cAddress = 0x53;
  static constexpr uint16_t kGpsSerialBaud = 9600;

  explicit SensorService(bool stubOnly = true);

  void begin();
  TelemetryModel sampleNow(AppMode mode, unsigned long nowMs);
  void printStatus(Print &out) const;
  void scanI2cBus();

  void requestI2cScan();
  const SampleCounters &sampleCounters() const { return sampleCounters_; }
  const GpsState &gpsState() const { return gpsState_; }

  bool isSimulatingHardware() const { return simulateInputsOnly; }
  bool isReady() const { return ready; }

 private:
  bool simulateInputsOnly{true};
  bool ready{false};
  bool i2cBusInitialized{false};
  bool bmp280Available{false};
  bool adxl345Available{false};
  bool dht11Configured{false};
  bool dht11Available{false};
  bool gpsConfigured{false};
  bool gpsScanRequested{false};
  bool i2cScanRequested{false};
  bool i2cScanRanThisBoot{false};
  uint8_t bmp280Address{kBmp280I2cAddressA};
  uint32_t sampleCounter{0};
  uint32_t randomSeedState{0x12345678U};
  SampleCounters sampleCounters_;
  GpsState gpsState_;

  static constexpr uint8_t kGpsLineSize = 96;
  char gpsLine_[kGpsLineSize]{};
  uint8_t gpsLinePos_{0};
  unsigned long gpsBytesParsed_{0};
  unsigned long lastGpsByteMs_{0};
  bool gpsParserEnabled_{true};

  static constexpr float kAdxl345ScaleDivisor = 256.0f;
  static constexpr uint16_t kDht11PulseTimeoutUs = 1000U;
  static constexpr uint16_t kDht11BitOneThresholdUs = 48U;
  static constexpr unsigned long kDht11SampleIntervalMs = 2000UL;
  static constexpr uint8_t kDht11FrameBytes = 5U;
  static constexpr float kDht11MinTemperatureC = -40.0f;
  static constexpr float kDht11MaxTemperatureC = 80.0f;
  static constexpr float kDht11MinHumidityPercent = 0.0f;
  static constexpr float kDht11MaxHumidityPercent = 100.0f;

  bool compileFlagBmp280() const;
  bool compileFlagAdxl345() const;
  bool compileFlagDht11() const;
  bool compileFlagGps() const;
  bool compileFlagI2cScan() const;
  bool compileFlagI2cSensors() const;
  bool compileFlagI2cScanPath() const;

  void initDrivers();
  bool beginI2cSensors();
  bool beginGpsReader();
  bool initBmp280();
  bool initAdxl345();

  void beginStubBaseline();
  void sampleRandomFallback(TelemetryModel &model, unsigned long nowMs);
  void sampleHardwareSensors(TelemetryModel &model, unsigned long nowMs);
  bool sampleBmp280(TelemetryModel &model);
  bool sampleAdxl345(TelemetryModel &model);
  bool sampleDht11(TelemetryModel &model, bool &attempted);
  bool parseDht11Frame(const uint8_t *bytes, float &temperatureC, float &humidityPercent);
  bool sampleGps(TelemetryModel &model, unsigned long nowMs);

  bool probeI2CAddress(uint8_t address) const;
  bool readI2CRegister(uint8_t address, uint8_t reg, uint8_t *out) const;
  bool readI2CRegisters(uint8_t address, uint8_t reg, uint8_t *buffer, uint8_t count) const;
  bool parseGpsLine(const char *line);
  void parseRmcSentence(char *line);
  void parseGgaSentence(char *line);
  bool parseGpsFloat(const char *text, float &out) const;
  bool parseDegreesMinutes(const char *raw, char direction, float &degreesOut) const;
  void resetDht11();
  uint16_t nextRandom16();
  float nextRandom01();

  unsigned long dht11LastSampleMs_{0};
  bool dht11Ready_{false};
};
