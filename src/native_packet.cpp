#include "native_packet.h"

#include <cstring>

namespace {
constexpr size_t kHeaderBase = lora_message::kNativePacketHeaderBytes;
}  // namespace

namespace lora_message {

namespace {
void writeU16(uint8_t *out, uint16_t value) {
  out[0] = static_cast<uint8_t>(value & 0xffu);
  out[1] = static_cast<uint8_t>((value >> 8) & 0xffu);
}

void writeU32(uint8_t *out, uint32_t value) {
  out[0] = static_cast<uint8_t>(value & 0xffu);
  out[1] = static_cast<uint8_t>((value >> 8) & 0xffu);
  out[2] = static_cast<uint8_t>((value >> 16) & 0xffu);
  out[3] = static_cast<uint8_t>((value >> 24) & 0xffu);
}

uint16_t readU16(const uint8_t *in) {
  return static_cast<uint16_t>(in[0] | (static_cast<uint16_t>(in[1]) << 8));
}

uint32_t readU32(const uint8_t *in) {
  return static_cast<uint32_t>(in[0]) |
         (static_cast<uint32_t>(in[1]) << 8) |
         (static_cast<uint32_t>(in[2]) << 16) |
         (static_cast<uint32_t>(in[3]) << 24);
}

void writeIdentity(uint8_t *out, const NodeIdentity &identity) {
  writeU16(out, identity.regionId);
  writeU32(out + 2, identity.nodeId);
  writeU16(out + 6, identity.category);
}

NodeIdentity readIdentity(const uint8_t *in) {
  NodeIdentity identity{};
  identity.regionId = readU16(in);
  identity.nodeId = readU32(in + 2);
  identity.category = readU16(in + 6);
  return identity;
}
}  // namespace

uint16_t calculatePacketCrc16(const uint8_t *data, size_t size) {
  uint16_t crc = 0xffffu;
  if (data == nullptr) {
    return crc;
  }
  for (size_t i = 0; i < size; ++i) {
    crc ^= static_cast<uint16_t>(data[i]) << 8;
    for (uint8_t bit = 0; bit < 8; ++bit) {
      if ((crc & 0x8000u) != 0) {
        crc = static_cast<uint16_t>((crc << 1) ^ 0x1021u);
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

bool buildTextPayload(const char *message, uint8_t channel, TextPacketPayload &payload) {
  if (message == nullptr) {
    return false;
  }
  payload = TextPacketPayload{};
  payload.channel = channel;
  const size_t length = std::strlen(message);
  if (length > sizeof(payload.message) - 1) {
    return false;
  }
  std::memcpy(payload.message, message, length);
  return true;
}

bool buildControlPayload(uint8_t command, const uint8_t *arguments, uint8_t argumentCount,
                        ControlPacketPayload &payload) {
  if (arguments == nullptr && argumentCount > 0) {
    return false;
  }
  if (argumentCount > sizeof(payload.arguments)) {
    return false;
  }
  payload = ControlPacketPayload{};
  payload.command = command;
  payload.argumentCount = argumentCount;
  if (argumentCount > 0) {
    std::memcpy(payload.arguments, arguments, argumentCount);
  }
  return true;
}

bool buildDiagnosticPayload(uint8_t level, uint16_t code, const char *message,
                           DiagnosticPacketPayload &payload) {
  if (message == nullptr) {
    return false;
  }
  payload = DiagnosticPacketPayload{};
  payload.level = level;
  payload.code = code;
  const size_t length = std::strlen(message);
  if (length > sizeof(payload.message) - 1) {
    return false;
  }
  std::memcpy(payload.message, message, length);
  return true;
}

bool makeTextPacket(const TextPacketPayload &payload, const NodeIdentity &source,
                   const NodeIdentity &destination, uint32_t sequence, uint32_t nowMs,
                   NativePacket &packet) {
  packet = NativePacket{};
  packet.type = PacketType::Text;
  packet.sequence = sequence;
  packet.createdMs = nowMs;
  packet.source = source;
  packet.destination = destination;
  packet.payload[0] = payload.channel;
  std::memcpy(&packet.payload[1], payload.message, sizeof(payload.message));
  packet.payloadLength = static_cast<uint16_t>(kTextPayloadSerializedBytes);
  return true;
}

bool makeControlPacket(const ControlPacketPayload &payload, const NodeIdentity &source,
                      const NodeIdentity &destination, uint32_t sequence, uint32_t nowMs,
                      NativePacket &packet) {
  packet = NativePacket{};
  packet.type = PacketType::Control;
  packet.sequence = sequence;
  packet.createdMs = nowMs;
  packet.source = source;
  packet.destination = destination;
  packet.payload[0] = payload.command;
  packet.payload[1] = payload.argumentCount;
  packet.payload[2] = 0;
  packet.payload[3] = 0;
  std::memcpy(&packet.payload[4], payload.arguments, sizeof(payload.arguments));
  packet.payloadLength = static_cast<uint16_t>(kControlPayloadSerializedBytes);
  return true;
}

bool makeDiagnosticPacket(const DiagnosticPacketPayload &payload, const NodeIdentity &source,
                         const NodeIdentity &destination, uint32_t sequence, uint32_t nowMs,
                         NativePacket &packet) {
  packet = NativePacket{};
  packet.type = PacketType::Diagnostic;
  packet.sequence = sequence;
  packet.createdMs = nowMs;
  packet.source = source;
  packet.destination = destination;
  packet.payload[0] = payload.level;
  writeU16(&packet.payload[1], payload.code);
  std::memcpy(&packet.payload[3], payload.message, sizeof(payload.message));
  packet.payloadLength = static_cast<uint16_t>(kDiagnosticPayloadSerializedBytes);
  return true;
}

bool serializePacket(const NativePacket &packet, SerializedPacket &out) {
  if (packet.payloadLength > kNativePacketMaxPayloadBytes) {
    return false;
  }
  if (packet.version != kNativePacketVersion) {
    return false;
  }

  const size_t totalSize = kHeaderBase + packet.payloadLength + kNativePacketChecksumSize;
  if (totalSize > out.bytes.size()) {
    return false;
  }

  size_t offset = 0;
  std::memcpy(&out.bytes[offset], kNativePacketMagic, sizeof(kNativePacketMagic));
  offset += sizeof(kNativePacketMagic);
  out.bytes[offset++] = packet.version;
  out.bytes[offset++] = static_cast<uint8_t>(packet.type);
  out.bytes[offset++] = packet.flags;
  out.bytes[offset++] = packet.reserved;
  writeU16(&out.bytes[offset], packet.payloadLength);
  offset += 2;
  writeU32(&out.bytes[offset], packet.sequence);
  offset += 4;
  writeU32(&out.bytes[offset], packet.createdMs);
  offset += 4;
  writeIdentity(&out.bytes[offset], packet.source);
  offset += kNodeIdentitySerializedBytes;
  writeIdentity(&out.bytes[offset], packet.destination);
  offset += kNodeIdentitySerializedBytes;
  if (packet.payloadLength > 0) {
    std::memcpy(&out.bytes[offset], packet.payload, packet.payloadLength);
    offset += packet.payloadLength;
  }
  const uint16_t crc = calculatePacketCrc16(&out.bytes[0], offset);
  writeU16(&out.bytes[offset], crc);
  offset += kNativePacketChecksumSize;

  out.size = offset;
  return true;
}

bool parsePacket(const uint8_t *buffer, size_t size, NativePacket &out) {
  if (buffer == nullptr || size < kHeaderBase + kNativePacketChecksumSize) {
    return false;
  }
  if (std::memcmp(buffer, kNativePacketMagic, sizeof(kNativePacketMagic)) != 0) {
    return false;
  }

  size_t offset = sizeof(kNativePacketMagic);
  out = NativePacket{};
  out.version = buffer[offset++];
  if (out.version != kNativePacketVersion) {
    return false;
  }
  out.type = static_cast<PacketType>(buffer[offset++]);
  out.flags = buffer[offset++];
  out.reserved = buffer[offset++];
  out.payloadLength = readU16(&buffer[offset]);
  offset += 2;

  const size_t expectedSize = kHeaderBase + out.payloadLength + kNativePacketChecksumSize;
  if (size < expectedSize) {
    return false;
  }
  if (out.payloadLength > kNativePacketMaxPayloadBytes) {
    return false;
  }

  const uint16_t expectedCrc = readU16(&buffer[expectedSize - kNativePacketChecksumSize]);
  const uint16_t actualCrc = calculatePacketCrc16(buffer, expectedSize - kNativePacketChecksumSize);
  if (actualCrc != expectedCrc) {
    return false;
  }

  out.sequence = readU32(&buffer[offset]);
  offset += 4;
  out.createdMs = readU32(&buffer[offset]);
  offset += 4;
  out.source = readIdentity(&buffer[offset]);
  offset += kNodeIdentitySerializedBytes;
  out.destination = readIdentity(&buffer[offset]);
  offset += kNodeIdentitySerializedBytes;
  if (out.payloadLength > 0) {
    std::memcpy(out.payload, &buffer[offset], out.payloadLength);
    offset += out.payloadLength;
  }
  return true;
}

}  // namespace lora_message
