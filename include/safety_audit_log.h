#pragma once

#include <cstdint>

#include "native_packet.h"

namespace lora_message {

enum class AuditSeverity : uint8_t {
  Trace = 0,
  Info = 1,
  Warning = 2,
  Error = 3,
  Critical = 4,
};

enum class AuditEventCode : uint8_t {
  Unknown = 0,
  PacketParseFailure = 1,
  PacketOverflow = 2,
  InvalidContact = 3,
  QueueOverflow = 4,
  ConfigBlocked = 5,
};

struct SafetyAuditEvent {
  uint32_t eventMs{0};
  AuditSeverity severity{AuditSeverity::Info};
  AuditEventCode code{AuditEventCode::Unknown};
  NodeIdentity source{};
  char message[48]{};
};

class SafetyAuditLog {
 public:
  explicit SafetyAuditLog(uint8_t capacity = 16);

  bool push(AuditSeverity severity, AuditEventCode code, const NodeIdentity &source,
            uint32_t eventMs, const char *message);
  bool record(const SafetyAuditEvent &event);
  uint8_t size() const;
  uint8_t capacity() const;
  uint32_t dropped() const;
  bool latest(SafetyAuditEvent &event) const;
  bool oldest(SafetyAuditEvent &event) const;
  void clear();

 private:
  static constexpr uint8_t kMaxCapacity = 32;
  bool full_() const;
  uint8_t capacity_{0};
  uint8_t head_{0};
  uint8_t tail_{0};
  uint8_t size_{0};
  uint32_t dropped_{0};
  SafetyAuditEvent events_[kMaxCapacity]{};
};

}  // namespace lora_message
