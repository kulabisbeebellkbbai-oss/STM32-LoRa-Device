#pragma once

#include <Arduino.h>

class RadioReceiveInbox;

struct RadioInboundEvent {
  enum class Kind : uint8_t {
    AtResponse,
    ModemStatus,
    DataFrame,
    Unknown,
  };

  static constexpr uint8_t kPayloadBytes = 64;

  Kind kind{Kind::Unknown};
  bool injected{false};
  uint8_t length{0};
  char payload[kPayloadBytes]{};
};

class RadioReceiveInbox {
 public:
  static constexpr uint8_t kDepth = 8;

  bool enqueue(RadioInboundEvent::Kind kind, const char *payload, size_t length,
              bool injected = false);
  bool enqueue(const RadioInboundEvent &event);
  bool injectPayload(const char *payload, size_t length);
  bool dequeue(RadioInboundEvent &event);
  bool isEmpty() const;
  bool isFull() const;
  uint8_t count() const;
  void clear();

 private:
  RadioInboundEvent queue_[kDepth]{};
  uint8_t head_{0};
  uint8_t tail_{0};
  uint8_t count_{0};

  bool overflowAndDrop();
};
