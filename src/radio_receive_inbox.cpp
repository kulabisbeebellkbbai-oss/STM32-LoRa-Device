#include "radio_receive_inbox.h"

#include <cstring>

namespace {

bool copyPayload(char *destination, const char *source, size_t payloadLength, bool padNull) {
  if (destination == nullptr || source == nullptr) {
    return false;
  }
  std::memset(destination, 0, RadioInboundEvent::kPayloadBytes);
  const size_t bytesToCopy = (payloadLength >= RadioInboundEvent::kPayloadBytes)
                                 ? RadioInboundEvent::kPayloadBytes - 1
                                 : payloadLength;
  std::memcpy(destination, source, bytesToCopy);
  if (padNull) {
    destination[bytesToCopy] = '\0';
  }
  return payloadLength < RadioInboundEvent::kPayloadBytes;
}

}  // namespace

bool RadioReceiveInbox::enqueue(RadioInboundEvent::Kind kind, const char *payload, size_t length,
                               bool injected) {
  RadioInboundEvent event;
  event.kind = kind;
  event.injected = injected;
  event.length = (length >= RadioInboundEvent::kPayloadBytes)
                     ? RadioInboundEvent::kPayloadBytes - 1
                     : static_cast<uint8_t>(length);
  copyPayload(event.payload, payload != nullptr ? payload : "", event.length, true);
  return enqueue(event);
}

bool RadioReceiveInbox::enqueue(const RadioInboundEvent &event) {
  if (isFull()) {
    if (!overflowAndDrop()) {
      return false;
    }
  }

  queue_[tail_] = event;
  tail_ = static_cast<uint8_t>((tail_ + 1) % kDepth);
  ++count_;
  return true;
}

bool RadioReceiveInbox::injectPayload(const char *payload, size_t length) {
  return enqueue(RadioInboundEvent::Kind::DataFrame, payload, length, true);
}

bool RadioReceiveInbox::dequeue(RadioInboundEvent &event) {
  if (isEmpty()) {
    return false;
  }
  event = queue_[head_];
  head_ = static_cast<uint8_t>((head_ + 1) % kDepth);
  --count_;
  return true;
}

bool RadioReceiveInbox::isEmpty() const {
  return count_ == 0;
}

bool RadioReceiveInbox::isFull() const {
  return count_ >= kDepth;
}

uint8_t RadioReceiveInbox::count() const {
  return count_;
}

void RadioReceiveInbox::clear() {
  head_ = 0;
  tail_ = 0;
  count_ = 0;
}

bool RadioReceiveInbox::overflowAndDrop() {
  RadioInboundEvent dropped{};
  if (dequeue(dropped)) {
    return true;
  }
  return false;
}
