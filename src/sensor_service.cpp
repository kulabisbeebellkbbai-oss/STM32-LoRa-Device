#include "sensor_service.h"

#include <cstdlib>
#include <cstring>
#include <Wire.h>

namespace {
constexpr uint8_t kBmp280IdReg = 0xD0;
constexpr uint8_t kBmp280TempDataReg = 0xFA;
constexpr uint8_t kBmp280PressDataReg = 0xF7;
constexpr uint8_t kBmp280IdExpected = 0x58;

constexpr uint8_t kAdxl345IdReg = 0x00;
constexpr uint8_t kAdxl345IdExpected = 0xE5;
constexpr uint8_t kAdxl345PowerCtlReg = 0x2D;
constexpr uint8_t kAdxl345DataFmtReg = 0x31;
constexpr uint8_t kAdxl345DataReg = 0x32;

constexpr uint8_t kBmp280ScanAddresses[] = {SensorService::kBmp280I2cAddressA,
                                            SensorService::kBmp280I2cAddressB};

void parseToken(const char *&cursor, char *out, size_t outLen) {
  if (outLen == 0) {
    return;
  }
  size_t n = 0;
  while (*cursor != '\0' && *cursor != ',') {
    if (n + 1 < outLen) {
      out[n] = *cursor;
      ++n;
    }
    ++cursor;
  }
  out[n] = '\0';
  if (*cursor == ',') {
    ++cursor;
  }
}
}  // namespace

SensorService::SensorService(bool stubOnly) : simulateInputsOnly(stubOnly) {
#if defined(RENODE_SIMULATION)
  simulateInputsOnly = true;
#endif
}

bool SensorService::compileFlagBmp280() const {
#if defined(LORA_SENSOR_BMP280)
  return true;
#else
  return false;
#endif
}

bool SensorService::compileFlagAdxl345() const {
#if defined(LORA_SENSOR_ADXL345)
  return true;
#else
  return false;
#endif
}

bool SensorService::compileFlagDht11() const {
#if defined(LORA_SENSOR_DHT11)
  return true;
#else
  return false;
#endif
}

bool SensorService::compileFlagGps() const {
#if defined(LORA_SENSOR_GPS)
  return true;
#else
  return false;
#endif
}

bool SensorService::compileFlagI2cScan() const {
#if defined(LORA_SENSOR_I2C_SCAN)
  return true;
#else
  return false;
#endif
}

bool SensorService::compileFlagI2cSensors() const {
  return compileFlagBmp280() || compileFlagAdxl345();
}

bool SensorService::compileFlagI2cScanPath() const {
  return compileFlagI2cScan();
}

void SensorService::begin() {
  sampleCounter = 0;
  sampleCounters_ = SampleCounters();
  gpsBytesParsed_ = 0;
  gpsState_ = GpsState();
  gpsLinePos_ = 0;
  resetDht11();

  if (simulateInputsOnly) {
    beginStubBaseline();
    return;
  }
  initDrivers();
}

void SensorService::beginStubBaseline() {
  ready = true;
  randomSeedState = 0xA5A5F00DU;
  gpsState_.parserEnabled = false;
  resetDht11();
}

void SensorService::initDrivers() {
  bool anyDriverEnabled = false;
  if (compileFlagI2cSensors()) {
    anyDriverEnabled = beginI2cSensors();
  }
  if (compileFlagGps()) {
    anyDriverEnabled = beginGpsReader() || anyDriverEnabled;
  }
#if !defined(RENODE_SIMULATION)
  dht11Configured = compileFlagDht11();
  dht11Available = false;
  dht11Ready_ = dht11Configured;
  anyDriverEnabled = anyDriverEnabled || dht11Configured;
#else
  dht11Configured = false;
  dht11Available = false;
  dht11Ready_ = false;
#endif

  if (!anyDriverEnabled) {
    beginStubBaseline();
    return;
  }

  ready = true;

#if !defined(RENODE_SIMULATION)
  if (compileFlagI2cScanPath() && i2cScanRequested) {
    scanI2cBus();
    i2cScanRequested = false;
    i2cScanRanThisBoot = true;
  }
#endif
}

bool SensorService::beginI2cSensors() {
#if defined(RENODE_SIMULATION)
  return false;
#else
  if (!compileFlagI2cSensors()) {
    return false;
  }
  Wire.begin(lora_device::PinConfig::kBmp280I2cSda, lora_device::PinConfig::kBmp280I2cScl);
  i2cBusInitialized = true;
  bool anyFound = false;

  if (compileFlagBmp280()) {
    anyFound = initBmp280();
  }
  if (compileFlagAdxl345()) {
    anyFound = initAdxl345() || anyFound;
  }
  return anyFound;
#endif
}

bool SensorService::beginGpsReader() {
#if defined(RENODE_SIMULATION)
  return false;
#else
#if defined(LORA_SENSOR_GPS)
  Serial1.begin(kGpsSerialBaud);
  gpsConfigured = true;
  gpsState_.parserEnabled = true;
  return true;
#else
  return false;
#endif
#endif
}

bool SensorService::initBmp280() {
#if defined(RENODE_SIMULATION)
  return false;
#else
#if defined(LORA_SENSOR_BMP280)
  for (uint8_t idx = 0; idx < sizeof(kBmp280ScanAddresses); ++idx) {
    uint8_t candidate = kBmp280ScanAddresses[idx];
    uint8_t id = 0x00;
    if (!readI2CRegister(candidate, kBmp280IdReg, &id)) {
      continue;
    }
    if (id == kBmp280IdExpected) {
      bmp280Available = true;
      bmp280Address = candidate;
      return true;
    }
  }
  return false;
#endif
  return false;
#endif
}

bool SensorService::initAdxl345() {
#if defined(RENODE_SIMULATION)
  return false;
#else
#if defined(LORA_SENSOR_ADXL345)
  uint8_t id = 0x00;
  if (!readI2CRegister(kAdxl345I2cAddress, kAdxl345IdReg, &id)) {
    return false;
  }
  if (id != kAdxl345IdExpected) {
    return false;
  }

  Wire.beginTransmission(kAdxl345I2cAddress);
  Wire.write(kAdxl345DataFmtReg);
  Wire.write(0x08);  // 2g full-resolution.
  if (Wire.endTransmission() != 0) {
    return false;
  }
  Wire.beginTransmission(kAdxl345I2cAddress);
  Wire.write(kAdxl345PowerCtlReg);
  Wire.write(0x08);  // measurement mode.
  if (Wire.endTransmission() != 0) {
    return false;
  }

  adxl345Available = true;
  return true;
#else
  return false;
#endif
#endif
}

TelemetryModel SensorService::sampleNow(AppMode mode, unsigned long nowMs) {
  TelemetryModel model;
  sampleCounter++;
  sampleCounters_.sampleCounter = sampleCounter;

  model.sampleIndex = sampleCounter;
  model.sampleMs = nowMs;
  model.uptimeMs = nowMs;
  model.mode = mode;
  model.sensorsValid = false;

  if (simulateInputsOnly || !ready) {
    sampleRandomFallback(model, nowMs);
    return model;
  }

  sampleHardwareSensors(model, nowMs);
  if (compileFlagGps() && gpsConfigured) {
    const bool gpsUpdated = sampleGps(model, nowMs);
    model.sensorsValid = model.sensorsValid || gpsUpdated;
  }

  return model;
}

void SensorService::sampleRandomFallback(TelemetryModel &model, unsigned long nowMs) {
  sampleCounters_.stubSamples++;
  const float phase = (nowMs % 600000UL) / 1000.0f;
  model.batteryVoltage = 3.85f + nextRandom01() * 0.3f;
  model.batteryPercent = 88.0f + (nextRandom01() - 0.5f) * 10.0f;
  model.ambientTemperatureC = 22.0f + sinf(phase * 0.07f) + nextRandom01();
  model.ambientHumidityPercent = 45.0f + nextRandom01() * 12.0f;
  model.pressureHPa = 1012.0f + nextRandom01() * 6.0f;
  model.accelX = nextRandom01() * 0.4f;
  model.accelY = nextRandom01() * 0.4f;
  model.accelZ = 1.0f + nextRandom01() * 0.2f;
  model.gpsFix = false;
  model.gpsSatellites = 0;
  model.gpsLatitude = 43.65f + sinf(phase * 0.01f) * 0.001f;
  model.gpsLongitude = -79.38f + cosf(phase * 0.01f) * 0.001f;
}

void SensorService::sampleHardwareSensors(TelemetryModel &model, unsigned long nowMs) {
  (void)nowMs;
  if (compileFlagBmp280() && bmp280Available && sampleBmp280(model)) {
    sampleCounters_.bmp280Samples++;
    model.sensorsValid = true;
  } else if (compileFlagBmp280() && !bmp280Available) {
    sampleCounters_.bmp280Failures++;
  }

  if (compileFlagAdxl345() && adxl345Available && sampleAdxl345(model)) {
    sampleCounters_.adxl345Samples++;
    model.sensorsValid = true;
  } else if (compileFlagAdxl345() && !adxl345Available) {
    sampleCounters_.adxl345Failures++;
  }

  if (compileFlagDht11()) {
    bool attempt = false;
    if (sampleDht11(model, attempt)) {
      dht11Available = true;
      sampleCounters_.dht11Samples++;
      model.sensorsValid = true;
    } else if (attempt) {
      dht11Available = false;
      sampleCounters_.dht11Failures++;
    }
  }

  if (!compileFlagBmp280() && !compileFlagAdxl345() && !compileFlagDht11()) {
    model.sensorsValid = false;
  }
}

bool SensorService::sampleBmp280(TelemetryModel &model) {
#if defined(RENODE_SIMULATION)
  return false;
#else
#if defined(LORA_SENSOR_BMP280)
  uint8_t tempRaw[3]{};
  uint8_t pressRaw[3]{};
  if (!readI2CRegisters(bmp280Address, kBmp280TempDataReg, tempRaw, sizeof(tempRaw)) ||
      !readI2CRegisters(bmp280Address, kBmp280PressDataReg, pressRaw, sizeof(pressRaw))) {
    return false;
  }

  const uint32_t tempRawU =
      (static_cast<uint32_t>(tempRaw[0]) << 12) | (static_cast<uint32_t>(tempRaw[1]) << 4) |
      (static_cast<uint32_t>(tempRaw[2]) >> 4);
  const uint32_t pressRawU =
      (static_cast<uint32_t>(pressRaw[0]) << 12) | (static_cast<uint32_t>(pressRaw[1]) << 4) |
      (static_cast<uint32_t>(pressRaw[2]) >> 4);

  model.ambientTemperatureC = 15.0f + static_cast<float>(tempRawU % 4000U) / 100.0f;
  model.pressureHPa = 950.0f + static_cast<float>(pressRawU % 12000U) / 100.0f;
  return true;
#else
  return false;
#endif
#endif
}

bool SensorService::sampleAdxl345(TelemetryModel &model) {
#if defined(RENODE_SIMULATION)
  return false;
#else
#if defined(LORA_SENSOR_ADXL345)
  uint8_t raw[6]{};
  if (!readI2CRegisters(kAdxl345I2cAddress, kAdxl345DataReg, raw, sizeof(raw))) {
    return false;
  }

  const int16_t x = (static_cast<int16_t>(raw[1]) << 8) | static_cast<int16_t>(raw[0]);
  const int16_t y = (static_cast<int16_t>(raw[3]) << 8) | static_cast<int16_t>(raw[2]);
  const int16_t z = (static_cast<int16_t>(raw[5]) << 8) | static_cast<int16_t>(raw[4]);
  model.accelX = static_cast<float>(x) / kAdxl345ScaleDivisor;
  model.accelY = static_cast<float>(y) / kAdxl345ScaleDivisor;
  model.accelZ = static_cast<float>(z) / kAdxl345ScaleDivisor;
  return true;
#else
  return false;
#endif
#endif
}

bool SensorService::sampleDht11(TelemetryModel &model, bool &attempted) {
  attempted = false;

#if defined(LORA_SENSOR_DHT11)
#if defined(RENODE_SIMULATION)
  (void)model;
  return false;
#else
  if (!dht11Ready_) {
    return false;
  }

  const unsigned long nowMs = millis();
  if (nowMs - dht11LastSampleMs_ < kDht11SampleIntervalMs) {
    return false;
  }

  dht11LastSampleMs_ = nowMs;
  attempted = true;

  constexpr uint8_t kDht11StartDelayUs = 20U;
  constexpr uint8_t kDht11StopHoldoffUs = 20U;
  constexpr uint8_t kDht11BitCount = 40U;

  const uint8_t pin = lora_device::PinConfig::kDht11Data;
  uint8_t frame[kDht11FrameBytes]{};
  const auto pinToBit = [](uint32_t pulseUs) -> uint8_t {
    return pulseUs > kDht11BitOneThresholdUs ? 1U : 0U;
  };

  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);
  delay(kDht11StopHoldoffUs);
  digitalWrite(pin, LOW);
  delayMicroseconds(18000U);
  digitalWrite(pin, HIGH);
  delayMicroseconds(kDht11StartDelayUs);
  pinMode(pin, INPUT_PULLUP);

  if (pulseIn(pin, LOW, kDht11PulseTimeoutUs) == 0) {
    return false;
  }
  if (pulseIn(pin, HIGH, kDht11PulseTimeoutUs) == 0) {
    return false;
  }

  for (uint8_t bit = 0; bit < kDht11BitCount; ++bit) {
    if (pulseIn(pin, LOW, kDht11PulseTimeoutUs) == 0) {
      return false;
    }
    const uint32_t highUs = pulseIn(pin, HIGH, kDht11PulseTimeoutUs);
    if (highUs == 0) {
      return false;
    }
    const uint8_t bitValue = pinToBit(highUs);
    frame[bit / 8] = static_cast<uint8_t>((frame[bit / 8] << 1U) | bitValue);
  }

  float tempC = 0.0f;
  float humPercent = 0.0f;
  if (!parseDht11Frame(frame, tempC, humPercent)) {
    return false;
  }

  model.ambientTemperatureC = tempC;
  model.ambientHumidityPercent = humPercent;
  return true;
#endif
#else
  (void)model;
  (void)attempted;
  return false;
#endif
}

bool SensorService::parseDht11Frame(const uint8_t *bytes, float &temperatureC, float &humidityPercent) {
  if (bytes == nullptr) {
    return false;
  }

  const uint8_t checksum = static_cast<uint8_t>(bytes[0] + bytes[1] + bytes[2] + bytes[3]);
  if (checksum != bytes[4]) {
    return false;
  }

  const float humidity = static_cast<float>(bytes[0]) + static_cast<float>(bytes[1]) / 10.0f;
  const uint8_t tempSignBit = static_cast<uint8_t>(bytes[2] & 0x80U);
  const uint8_t tempWhole = static_cast<uint8_t>(bytes[2] & 0x7FU);
  const float tempFrac = static_cast<float>(bytes[3]) / 10.0f;

  if (bytes[1] > 9U || bytes[3] > 9U) {
    return false;
  }
  if (humidity < kDht11MinHumidityPercent || humidity > kDht11MaxHumidityPercent) {
    return false;
  }

  const float temp = (tempSignBit != 0U ? -1.0f : 1.0f) * (static_cast<float>(tempWhole) + tempFrac);
  if (temp < kDht11MinTemperatureC || temp > kDht11MaxTemperatureC) {
    return false;
  }

  humidityPercent = humidity;
  temperatureC = temp;
  return true;
}

void SensorService::resetDht11() {
  dht11LastSampleMs_ = 0;
  dht11Ready_ = false;
}

bool SensorService::sampleGps(TelemetryModel &model, unsigned long nowMs) {
#if defined(RENODE_SIMULATION)
  (void)model;
  (void)nowMs;
  return false;
#else
  bool parsedThisCall = false;
#if defined(LORA_SENSOR_GPS)
  while (Serial1.available() > 0) {
    const char incoming = static_cast<char>(Serial1.read());
    lastGpsByteMs_ = nowMs;
    gpsBytesParsed_++;

    if (incoming == '\r') {
      continue;
    }

    if (incoming == '$') {
      gpsLinePos_ = 0;
      gpsLine_[gpsLinePos_++] = incoming;
      continue;
    }

    if (incoming == '\n') {
      if (gpsLinePos_ > 0) {
        gpsLine_[gpsLinePos_] = '\0';
        if (parseGpsLine(gpsLine_)) {
          if (gpsState_.hasFix || gpsState_.satellites > 0) {
            sampleCounters_.gpsSamples++;
          } else {
            sampleCounters_.gpsFailures++;
          }
          model.gpsFix = gpsState_.hasFix;
          model.gpsSatellites = gpsState_.satellites;
          model.gpsLatitude = gpsState_.lastLatitude;
          model.gpsLongitude = gpsState_.lastLongitude;
          parsedThisCall = true;
        }
      }
      gpsLinePos_ = 0;
      continue;
    }

    if (gpsLinePos_ >= (kGpsLineSize - 1)) {
      sampleCounters_.gpsFailures++;
      gpsLinePos_ = 0;
      continue;
    }
    gpsLine_[gpsLinePos_++] = incoming;
  }

  if (!parsedThisCall && gpsLinePos_ > 0 && (nowMs - lastGpsByteMs_) > 250) {
    gpsLinePos_ = 0;
    sampleCounters_.gpsFailures++;
  }
#else
  (void)model;
  (void)nowMs;
#endif
  return parsedThisCall;
#endif
}

bool SensorService::parseGpsLine(const char *line) {
  if (line == nullptr || line[0] == '\0') {
    return false;
  }

  if (std::strncmp(line, "$GPRMC", 6) == 0 || std::strncmp(line, "$GNRMC", 6) == 0) {
    char working[kGpsLineSize];
    std::strncpy(working, line, sizeof(working) - 1);
    working[sizeof(working) - 1] = '\0';
    parseRmcSentence(working);
    return true;
  }

  if (std::strncmp(line, "$GPGGA", 6) == 0 || std::strncmp(line, "$GNGGA", 6) == 0) {
    char working[kGpsLineSize];
    std::strncpy(working, line, sizeof(working) - 1);
    working[sizeof(working) - 1] = '\0';
    parseGgaSentence(working);
    return true;
  }

  return false;
}

void SensorService::parseRmcSentence(char *line) {
  const char *cursor = line;
  char token[32];
  char latRaw[24]{};
  char lonRaw[24]{};
  char latDir[4]{};
  char lonDir[4]{};
  parseToken(cursor, token, sizeof(token));         // $GPRMC / $GNRMC
  parseToken(cursor, token, sizeof(token));         // UTC time
  parseToken(cursor, token, sizeof(token));         // fix
  const bool hasFix = (token[0] == 'A');
  parseToken(cursor, latRaw, sizeof(latRaw));       // latitude
  parseToken(cursor, latDir, sizeof(latDir));       // N/S
  parseToken(cursor, lonRaw, sizeof(lonRaw));       // longitude
  parseToken(cursor, lonDir, sizeof(lonDir));       // E/W
  parseToken(cursor, token, sizeof(token));         // speed
  parseToken(cursor, token, sizeof(token));         // course
  parseToken(cursor, token, sizeof(token));         // date

  if (!hasFix) {
    gpsState_.hasFix = false;
    return;
  }

  float lat = 0.0f;
  float lon = 0.0f;
  if (parseDegreesMinutes(latRaw, latDir[0], lat) &&
      parseDegreesMinutes(lonRaw, lonDir[0], lon)) {
    gpsState_.lastLatitude = lat;
    gpsState_.lastLongitude = lon;
    gpsState_.lastValidFixMs = millis();
    gpsState_.hasFix = true;
  } else {
    gpsState_.hasFix = false;
  }
}

void SensorService::parseGgaSentence(char *line) {
  const char *cursor = line;
  char token[24];
  parseToken(cursor, token, sizeof(token));  // $GPGGA / $GNGGA
  parseToken(cursor, token, sizeof(token));  // UTC
  parseToken(cursor, token, sizeof(token));  // lat
  parseToken(cursor, token, sizeof(token));  // lat dir
  parseToken(cursor, token, sizeof(token));  // lon
  parseToken(cursor, token, sizeof(token));  // lon dir
  parseToken(cursor, token, sizeof(token));  // fix
  parseToken(cursor, token, sizeof(token));  // satellites
  if (token[0] != '\0') {
    gpsState_.satellites = std::atoi(token);
  }
}

bool SensorService::parseGpsFloat(const char *text, float &out) const {
  if (text == nullptr || text[0] == '\0') {
    return false;
  }
  char *end = nullptr;
  out = std::strtof(text, &end);
  return end != nullptr && end != text;
}

bool SensorService::parseDegreesMinutes(const char *raw, char direction, float &degreesOut) const {
  float value = 0.0f;
  if (!parseGpsFloat(raw, value)) {
    return false;
  }
  const uint16_t degrees = static_cast<uint16_t>(value / 100.0f);
  const float minutes = value - static_cast<float>(degrees) * 100.0f;
  degreesOut = static_cast<float>(degrees) + (minutes / 60.0f);
  if (direction == 'S' || direction == 'W') {
    degreesOut = -degreesOut;
  }
  return true;
}

bool SensorService::probeI2CAddress(uint8_t address) const {
  Wire.beginTransmission(address);
  const uint8_t result = Wire.endTransmission();
  return result == 0;
}

bool SensorService::readI2CRegister(uint8_t address, uint8_t reg, uint8_t *out) const {
  if (!i2cBusInitialized) {
    return false;
  }
  if (out == nullptr) {
    return false;
  }
  Wire.beginTransmission(address);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  if (Wire.requestFrom(address, static_cast<uint8_t>(1U)) != 1) {
    return false;
  }
  *out = Wire.read();
  return true;
}

bool SensorService::readI2CRegisters(uint8_t address, uint8_t reg, uint8_t *buffer, uint8_t count) const {
  if (!i2cBusInitialized) {
    return false;
  }
  if (buffer == nullptr || count == 0) {
    return false;
  }
  Wire.beginTransmission(address);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  if (Wire.requestFrom(address, count) != count) {
    return false;
  }
  for (uint8_t i = 0; i < count; ++i) {
    if (!Wire.available()) {
      return false;
    }
    buffer[i] = Wire.read();
  }
  return true;
}

void SensorService::requestI2cScan() {
  i2cScanRequested = true;
}

void SensorService::scanI2cBus() {
  i2cScanRequested = false;
  if (!compileFlagI2cScanPath()) {
    LORA_DEVICE_CONSOLE.println(F("SENSOR_I2C_SCAN: compile-time path disabled"));
    return;
  }
#if defined(RENODE_SIMULATION)
  LORA_DEVICE_CONSOLE.println(F("SENSOR_I2C_SCAN: not simulated in Renode"));
  return;
#else
#if defined(LORA_SENSOR_I2C_SCAN)
  sampleCounters_.i2cScans++;
  sampleCounters_.bmp280I2cHits = 0;
  sampleCounters_.adxl345I2cHits = 0;
  i2cScanRanThisBoot = true;

  if (!i2cBusInitialized) {
    Wire.begin(lora_device::PinConfig::kBmp280I2cSda, lora_device::PinConfig::kBmp280I2cScl);
    i2cBusInitialized = true;
  }

  for (uint8_t addr = 1; addr < 128; ++addr) {
    if (!probeI2CAddress(addr)) {
      continue;
    }

    if (addr == kBmp280ScanAddresses[0] || addr == kBmp280ScanAddresses[1]) {
      sampleCounters_.bmp280I2cHits++;
    }
    if (addr == kAdxl345I2cAddress) {
      sampleCounters_.adxl345I2cHits++;
    }

    LORA_DEVICE_CONSOLE.print(F("SENSOR_I2C_SCAN: 0x"));
    if (addr < 0x10) {
      LORA_DEVICE_CONSOLE.print(F("0"));
    }
    LORA_DEVICE_CONSOLE.print(addr, HEX);
    LORA_DEVICE_CONSOLE.print(F(" "));
  }
  LORA_DEVICE_CONSOLE.println();
#endif
#endif
}

void SensorService::printStatus(Print &out) const {
  out.print(F("sensor ready="));
  out.print(ready ? F("yes") : F("no"));
  out.print(F(" stubOnly="));
  out.print(simulateInputsOnly ? F("yes") : F("no"));
  out.print(F(" renodeSimulation="));
#if defined(RENODE_SIMULATION)
  out.print(F("yes"));
#else
  out.print(F("no"));
#endif
  out.print(F(" sampleCounter="));
  out.print(sampleCounters_.sampleCounter);
  out.print(F(" stubSamples="));
  out.print(sampleCounters_.stubSamples);

  out.print(F(" pins_i2c_sda="));
  out.print(lora_device::PinConfig::kBmp280I2cSda);
  out.print(F(" sck="));
  out.print(lora_device::PinConfig::kBmp280I2cScl);
  out.print(F(" dht11="));
  out.print(lora_device::PinConfig::kDht11Data);
  out.print(F(" gpsRx="));
  out.print(lora_device::PinConfig::kGpsRx);
  out.print(F(" gpsTx="));
  out.print(lora_device::PinConfig::kGpsTx);
  out.print(F(" gpsBaud="));
  out.print(kGpsSerialBaud);

  out.print(F(" bmp280=addr:"));
  out.print(kBmp280I2cAddressA, HEX);
  out.print(F("/"));
  out.print(kBmp280I2cAddressB, HEX);
  out.print(F(" selected:"));
  out.print(bmp280Address, HEX);
  out.print(F(" compile="));
  out.print(compileFlagBmp280() ? F("yes") : F("no"));
  out.print(F(" active="));
  out.print(bmp280Available ? F("yes") : F("no"));
  out.print(F(" samples="));
  out.print(sampleCounters_.bmp280Samples);
  out.print(F("/"));
  out.print(sampleCounters_.bmp280Failures);

  out.print(F(" adxl345=addr:"));
  out.print(kAdxl345I2cAddress, HEX);
  out.print(F(" compile="));
  out.print(compileFlagAdxl345() ? F("yes") : F("no"));
  out.print(F(" active="));
  out.print(adxl345Available ? F("yes") : F("no"));
  out.print(F(" samples="));
  out.print(sampleCounters_.adxl345Samples);
  out.print(F("/"));
  out.print(sampleCounters_.adxl345Failures);

  out.print(F(" dht11=pin:"));
  out.print(lora_device::PinConfig::kDht11Data);
  out.print(F(" compile="));
  out.print(compileFlagDht11() ? F("yes") : F("no"));
  out.print(F(" configured="));
  out.print(dht11Configured ? F("yes") : F("no"));
  out.print(F(" active="));
  out.print(dht11Available ? F("yes") : F("no"));
  out.print(F(" samples="));
  out.print(sampleCounters_.dht11Samples);
  out.print(F("/"));
  out.print(sampleCounters_.dht11Failures);

  out.print(F(" gps compile="));
  out.print(compileFlagGps() ? F("yes") : F("no"));
  out.print(F(" configured="));
  out.print(gpsConfigured ? F("yes") : F("no"));
  out.print(F(" parser="));
  out.print(gpsState_.parserEnabled ? F("on") : F("off"));
  out.print(F(" fix="));
  out.print(gpsState_.hasFix ? F("yes") : F("no"));
  out.print(F(" sat="));
  out.print(gpsState_.satellites);
  out.print(F(" fixAgeMs="));
  if (gpsState_.lastValidFixMs == 0) {
    out.print(F("none"));
  } else {
    out.print(millis() - gpsState_.lastValidFixMs);
  }
  out.print(F(" lat="));
  out.print(gpsState_.lastLatitude, 6);
  out.print(F(" lon="));
  out.print(gpsState_.lastLongitude, 6);

  out.print(F(" i2cScan compile="));
  out.print(compileFlagI2cScanPath() ? F("yes") : F("no"));
  out.print(F(" requested="));
  out.print(i2cScanRequested ? F("yes") : F("no"));
  out.print(F(" ran="));
  out.print(i2cScanRanThisBoot ? F("yes") : F("no"));
  out.print(F(" scans="));
  out.print(sampleCounters_.i2cScans);
  out.print(F(" bmp280Hits="));
  out.print(sampleCounters_.bmp280I2cHits);
  out.print(F(" adxlHits="));
  out.print(sampleCounters_.adxl345I2cHits);
  out.print(F(" gpsSamples="));
  out.print(sampleCounters_.gpsSamples);
  out.print(F("/"));
  out.print(sampleCounters_.gpsFailures);
  out.println();
}

uint16_t SensorService::nextRandom16() {
  randomSeedState ^= randomSeedState << 13;
  randomSeedState ^= randomSeedState >> 17;
  randomSeedState ^= randomSeedState << 5;
  return static_cast<uint16_t>(randomSeedState >> 16);
}

float SensorService::nextRandom01() {
  return nextRandom16() / 65535.0f;
}
