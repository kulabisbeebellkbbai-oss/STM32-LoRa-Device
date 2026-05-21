#pragma once

#include <cstddef>
#include <cstdint>

#include "native_packet.h"

namespace lora_message {

class BoundedMessageStore {
 public:
  explicit BoundedMessageStore(uint8_t capacity = 12);

  bool push(const NativePacket &packet);
  bool pop(NativePacket &packet);
  bool peekOldest(NativePacket &packet) const;
  bool peekNewest(NativePacket &packet) const;
  uint8_t size() const;
  uint8_t capacity() const;
  uint32_t droppedCount() const;
  void clear();

 private:
  static constexpr uint8_t kMaxCapacity = 16;
  bool isFullLocked() const;

  uint8_t capacity_{0};
  uint8_t head_{0};
  uint8_t tail_{0};
  uint8_t size_{0};
  uint32_t dropped_{0};
  NativePacket entries_[kMaxCapacity]{};
};

}  // namespace lora_message
