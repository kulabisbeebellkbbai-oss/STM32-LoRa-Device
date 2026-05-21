#pragma once

#include <Arduino.h>
#include "app_modes.h"

struct TelemetryModel {
  uint32_t sampleIndex{0};
  unsigned long sampleMs{0};
  unsigned long uptimeMs{0};
  AppMode mode{AppMode::Standalone};

  bool sensorsValid{false};
  float batteryVoltage{0.0f};
  float batteryPercent{0.0f};
  float ambientTemperatureC{0.0f};
  float ambientHumidityPercent{0.0f};
  float pressureHPa{0.0f};
  float accelX{0.0f};
  float accelY{0.0f};
  float accelZ{0.0f};
  float gpsLatitude{0.0f};
  float gpsLongitude{0.0f};
  bool gpsFix{false};
  int gpsSatellites{0};

  void printTelemetryLine(Print &out) const;
  void printCompactStatus(Print &out) const;
};
