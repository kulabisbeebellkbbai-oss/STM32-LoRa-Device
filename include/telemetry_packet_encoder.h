#pragma once

#include <cstdint>

#include "native_packet.h"
#include "telemetry_model.h"

namespace lora_message {

constexpr uint8_t kTelemetryPayloadScaleLatLon = 7;
constexpr uint16_t kTelemetryPayloadSerializedBytes = 39;

struct TelemetryPayload {
  uint32_t sampleIndex{0};
  uint32_t sampleMs{0};
  uint32_t uptimeMs{0};
  uint8_t mode{0};
  uint8_t flags{0};
  uint16_t batteryMv{0};
  uint16_t batteryPercentTenths{0};
  int16_t ambientTempDeciC{0};
  uint16_t ambientHumidityDeciPercent{0};
  uint16_t pressureHpaDeci{0};
  int16_t accelXMilliG{0};
  int16_t accelYMilliG{0};
  int16_t accelZMilliG{0};
  int32_t gpsLatitudeE7{0};
  int32_t gpsLongitudeE7{0};
  uint8_t gpsSatellites{0};
};

bool encodeTelemetryPayload(const TelemetryModel &telemetry, TelemetryPayload &payload);
bool makeTelemetryPacket(const TelemetryModel &telemetry, const NodeIdentity &source,
                        const NodeIdentity &destination, NativePacket &packet);
bool decodeTelemetryPayload(const NativePacket &packet, TelemetryPayload &payload);

}  // namespace lora_message
