#pragma once

#include <array>
#include <cstdint>
#include <cstring>

#include "node_identity.h"

namespace lora_message {

constexpr uint8_t kNativePacketVersion = 1;
constexpr uint16_t kNativePacketMaxPayloadBytes = 128;
constexpr uint16_t kNativePacketChecksumSize = 2;
constexpr size_t kNodeIdentitySerializedBytes = 8;
constexpr size_t kNativePacketHeaderBytes =
    4 + 1 + 1 + 1 + 1 + 2 + 4 + 4 + kNodeIdentitySerializedBytes + kNodeIdentitySerializedBytes;
constexpr size_t kNativePacketMaxSerializedBytes =
    kNativePacketHeaderBytes + kNativePacketMaxPayloadBytes + kNativePacketChecksumSize;
constexpr size_t kTextPayloadSerializedBytes = 1 + 64;
constexpr size_t kControlPayloadSerializedBytes = 1 + 1 + 2 + 64;
constexpr size_t kDiagnosticPayloadSerializedBytes = 1 + 2 + 61;
constexpr uint8_t kNativePacketMagic[4] = {0x4c, 0x6f, 0x52, 0x58};  // LoRx

enum class PacketType : uint8_t {
  Telemetry = 0x10,
  Text = 0x20,
  Control = 0x30,
  Diagnostic = 0x40,
};

enum PacketFlags : uint8_t {
  kPacketFlagNone = 0x00,
  kPacketFlagRequiresAck = 0x01,
  kPacketFlagLocalOnly = 0x02,
};

struct NativePacket {
  uint8_t version{kNativePacketVersion};
  PacketType type{PacketType::Diagnostic};
  uint8_t flags{kPacketFlagNone};
  uint8_t reserved{0};
  uint16_t payloadLength{0};
  uint32_t sequence{0};
  uint32_t createdMs{0};
  NodeIdentity source{};
  NodeIdentity destination{};
  uint8_t payload[kNativePacketMaxPayloadBytes]{};
};

struct TextPacketPayload {
  uint8_t channel{0};
  char message[64]{};
};

struct ControlPacketPayload {
  uint8_t command{0};
  uint8_t argumentCount{0};
  uint8_t reserved[2]{};
  uint8_t arguments[64]{};
};

struct DiagnosticPacketPayload {
  uint8_t level{0};
  uint16_t code{0};
  char message[61]{};
};

struct SerializedPacket {
  std::array<uint8_t, kNativePacketMaxSerializedBytes> bytes{};
  size_t size{0};
};

bool buildTextPayload(const char *message, uint8_t channel, TextPacketPayload &payload);
bool buildControlPayload(uint8_t command, const uint8_t *arguments, uint8_t argumentCount,
                        ControlPacketPayload &payload);
bool buildDiagnosticPayload(uint8_t level, uint16_t code, const char *message,
                           DiagnosticPacketPayload &payload);

bool makeTextPacket(const TextPacketPayload &payload, const NodeIdentity &source,
                   const NodeIdentity &destination, uint32_t sequence, uint32_t nowMs,
                   NativePacket &packet);
bool makeControlPacket(const ControlPacketPayload &payload, const NodeIdentity &source,
                      const NodeIdentity &destination, uint32_t sequence, uint32_t nowMs,
                      NativePacket &packet);
bool makeDiagnosticPacket(const DiagnosticPacketPayload &payload, const NodeIdentity &source,
                         const NodeIdentity &destination, uint32_t sequence, uint32_t nowMs,
                         NativePacket &packet);

uint16_t calculatePacketCrc16(const uint8_t *data, size_t size);
bool serializePacket(const NativePacket &packet, SerializedPacket &out);
bool parsePacket(const uint8_t *buffer, size_t size, NativePacket &out);

}  // namespace lora_message
