#include "telemetry_packet_encoder.h"

#include <cmath>
#include <cstring>

namespace {
constexpr int16_t kInt16Min = -32768;
constexpr int16_t kInt16Max = 32767;
constexpr uint16_t kUInt16Max = 65535;
constexpr int32_t kInt32Min = -2147483647L - 1L;
constexpr int32_t kInt32Max = 2147483647L;
}  // namespace

namespace lora_message {

namespace {
void writeU16(uint8_t *out, uint16_t value) {
  out[0] = static_cast<uint8_t>(value & 0xffu);
  out[1] = static_cast<uint8_t>((value >> 8) & 0xffu);
}

void writeI16(uint8_t *out, int16_t value) {
  writeU16(out, static_cast<uint16_t>(value));
}

void writeU32(uint8_t *out, uint32_t value) {
  out[0] = static_cast<uint8_t>(value & 0xffu);
  out[1] = static_cast<uint8_t>((value >> 8) & 0xffu);
  out[2] = static_cast<uint8_t>((value >> 16) & 0xffu);
  out[3] = static_cast<uint8_t>((value >> 24) & 0xffu);
}

void writeI32(uint8_t *out, int32_t value) {
  writeU32(out, static_cast<uint32_t>(value));
}

uint16_t readU16(const uint8_t *in) {
  return static_cast<uint16_t>(in[0] | (static_cast<uint16_t>(in[1]) << 8));
}

int16_t readI16(const uint8_t *in) {
  return static_cast<int16_t>(readU16(in));
}

uint32_t readU32(const uint8_t *in) {
  return static_cast<uint32_t>(in[0]) |
         (static_cast<uint32_t>(in[1]) << 8) |
         (static_cast<uint32_t>(in[2]) << 16) |
         (static_cast<uint32_t>(in[3]) << 24);
}

int32_t readI32(const uint8_t *in) {
  return static_cast<int32_t>(readU32(in));
}

template <typename T>
T clampValue(T value, T minValue, T maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

int16_t clampToInt16(long value) {
  return static_cast<int16_t>(clampValue(static_cast<long>(value), static_cast<long>(kInt16Min),
                                        static_cast<long>(kInt16Max)));
}

uint16_t clampToUInt16(long value) {
  return static_cast<uint16_t>(clampValue(static_cast<long>(value), 0L, static_cast<long>(kUInt16Max)));
}

int32_t clampToInt32(long long value) {
  return static_cast<int32_t>(clampValue(value, static_cast<long long>(kInt32Min),
                                        static_cast<long long>(kInt32Max)));
}
}  // namespace

bool encodeTelemetryPayload(const TelemetryModel &telemetry, TelemetryPayload &payload) {
  payload = TelemetryPayload{};
  payload.sampleIndex = telemetry.sampleIndex;
  payload.sampleMs = telemetry.sampleMs;
  payload.uptimeMs = telemetry.uptimeMs;
  payload.mode = static_cast<uint8_t>(telemetry.mode);
  payload.flags = telemetry.sensorsValid ? 0x01 : 0x00;
  payload.batteryMv = clampToUInt16(std::llround(telemetry.batteryVoltage * 1000.0f));
  payload.batteryPercentTenths = clampToUInt16(std::llround(telemetry.batteryPercent * 10.0f));
  payload.ambientTempDeciC = clampToInt16(std::llround(telemetry.ambientTemperatureC * 10.0f));
  payload.ambientHumidityDeciPercent = clampToUInt16(std::llround(telemetry.ambientHumidityPercent * 10.0f));
  payload.pressureHpaDeci = clampToUInt16(std::llround(telemetry.pressureHPa * 10.0f));
  payload.accelXMilliG = clampToInt16(std::llround(telemetry.accelX * 1000.0f));
  payload.accelYMilliG = clampToInt16(std::llround(telemetry.accelY * 1000.0f));
  payload.accelZMilliG = clampToInt16(std::llround(telemetry.accelZ * 1000.0f));
  payload.flags |= telemetry.gpsFix ? 0x02 : 0x00;
  payload.gpsSatellites = static_cast<uint8_t>(telemetry.gpsSatellites);
  const float scale = std::pow(10.0f, static_cast<float>(kTelemetryPayloadScaleLatLon));
  payload.gpsLatitudeE7 = clampToInt32(std::llround(telemetry.gpsLatitude * scale));
  payload.gpsLongitudeE7 = clampToInt32(std::llround(telemetry.gpsLongitude * scale));
  return true;
}

bool makeTelemetryPacket(const TelemetryModel &telemetry, const NodeIdentity &source,
                        const NodeIdentity &destination, NativePacket &packet) {
  TelemetryPayload payload{};
  if (!encodeTelemetryPayload(telemetry, payload)) {
    return false;
  }

  packet = NativePacket{};
  packet.type = PacketType::Telemetry;
  packet.sequence = telemetry.sampleIndex;
  packet.createdMs = telemetry.sampleMs;
  packet.source = source;
  packet.destination = destination;
  size_t offset = 0;
  writeU32(&packet.payload[offset], payload.sampleIndex);
  offset += 4;
  writeU32(&packet.payload[offset], payload.sampleMs);
  offset += 4;
  writeU32(&packet.payload[offset], payload.uptimeMs);
  offset += 4;
  packet.payload[offset++] = payload.mode;
  packet.payload[offset++] = payload.flags;
  writeU16(&packet.payload[offset], payload.batteryMv);
  offset += 2;
  writeU16(&packet.payload[offset], payload.batteryPercentTenths);
  offset += 2;
  writeI16(&packet.payload[offset], payload.ambientTempDeciC);
  offset += 2;
  writeU16(&packet.payload[offset], payload.ambientHumidityDeciPercent);
  offset += 2;
  writeU16(&packet.payload[offset], payload.pressureHpaDeci);
  offset += 2;
  writeI16(&packet.payload[offset], payload.accelXMilliG);
  offset += 2;
  writeI16(&packet.payload[offset], payload.accelYMilliG);
  offset += 2;
  writeI16(&packet.payload[offset], payload.accelZMilliG);
  offset += 2;
  writeI32(&packet.payload[offset], payload.gpsLatitudeE7);
  offset += 4;
  writeI32(&packet.payload[offset], payload.gpsLongitudeE7);
  offset += 4;
  packet.payload[offset++] = payload.gpsSatellites;
  packet.payloadLength = static_cast<uint16_t>(offset);
  return true;
}

bool decodeTelemetryPayload(const NativePacket &packet, TelemetryPayload &payload) {
  if (packet.type != PacketType::Telemetry || packet.payloadLength != kTelemetryPayloadSerializedBytes) {
    return false;
  }
  size_t offset = 0;
  payload = TelemetryPayload{};
  payload.sampleIndex = readU32(&packet.payload[offset]);
  offset += 4;
  payload.sampleMs = readU32(&packet.payload[offset]);
  offset += 4;
  payload.uptimeMs = readU32(&packet.payload[offset]);
  offset += 4;
  payload.mode = packet.payload[offset++];
  payload.flags = packet.payload[offset++];
  payload.batteryMv = readU16(&packet.payload[offset]);
  offset += 2;
  payload.batteryPercentTenths = readU16(&packet.payload[offset]);
  offset += 2;
  payload.ambientTempDeciC = readI16(&packet.payload[offset]);
  offset += 2;
  payload.ambientHumidityDeciPercent = readU16(&packet.payload[offset]);
  offset += 2;
  payload.pressureHpaDeci = readU16(&packet.payload[offset]);
  offset += 2;
  payload.accelXMilliG = readI16(&packet.payload[offset]);
  offset += 2;
  payload.accelYMilliG = readI16(&packet.payload[offset]);
  offset += 2;
  payload.accelZMilliG = readI16(&packet.payload[offset]);
  offset += 2;
  payload.gpsLatitudeE7 = readI32(&packet.payload[offset]);
  offset += 4;
  payload.gpsLongitudeE7 = readI32(&packet.payload[offset]);
  offset += 4;
  payload.gpsSatellites = packet.payload[offset++];
  return true;
}

}  // namespace lora_message
