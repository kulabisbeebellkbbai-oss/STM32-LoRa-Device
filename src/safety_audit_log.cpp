#include "safety_audit_log.h"

#include <cstring>

namespace {
constexpr uint8_t kMaxMessageLen = sizeof(lora_message::SafetyAuditEvent::message);
}

namespace lora_message {

namespace {
void setMessage(SafetyAuditEvent &event, const char *message) {
  event.message[0] = '\0';
  if (message == nullptr) {
    return;
  }
  const size_t len = std::strlen(message);
  const size_t copyLen = (len >= kMaxMessageLen) ? (kMaxMessageLen - 1) : len;
  std::memcpy(event.message, message, copyLen);
  event.message[copyLen] = '\0';
}
}

SafetyAuditLog::SafetyAuditLog(uint8_t capacity) {
  if (capacity == 0) {
    capacity_ = 1;
  } else if (capacity > kMaxCapacity) {
    capacity_ = kMaxCapacity;
  } else {
    capacity_ = capacity;
  }
}

bool SafetyAuditLog::full_() const {
  return size_ >= capacity_;
}

uint8_t SafetyAuditLog::size() const {
  return size_;
}

uint8_t SafetyAuditLog::capacity() const {
  return capacity_;
}

uint32_t SafetyAuditLog::dropped() const {
  return dropped_;
}

void SafetyAuditLog::clear() {
  head_ = 0;
  tail_ = 0;
  size_ = 0;
  dropped_ = 0;
}

bool SafetyAuditLog::push(AuditSeverity severity, AuditEventCode code, const NodeIdentity &source,
                          uint32_t eventMs, const char *message) {
  SafetyAuditEvent event{};
  event.eventMs = eventMs;
  event.severity = severity;
  event.code = code;
  event.source = source;
  setMessage(event, message);
  return record(event);
}

bool SafetyAuditLog::record(const SafetyAuditEvent &event) {
  if (size_ >= capacity_) {
    head_ = static_cast<uint8_t>((head_ + 1) % capacity_);
    --size_;
    ++dropped_;
  }

  events_[tail_] = event;
  tail_ = static_cast<uint8_t>((tail_ + 1) % capacity_);
  ++size_;
  return size_ <= capacity_;
}

bool SafetyAuditLog::latest(SafetyAuditEvent &event) const {
  if (size_ == 0) {
    return false;
  }
  const uint8_t index = (tail_ + capacity_ - 1) % capacity_;
  event = events_[index];
  return true;
}

bool SafetyAuditLog::oldest(SafetyAuditEvent &event) const {
  if (size_ == 0) {
    return false;
  }
  event = events_[head_];
  return true;
}

}  // namespace lora_message
