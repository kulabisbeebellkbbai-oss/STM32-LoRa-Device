#include "message_store.h"

namespace lora_message {

BoundedMessageStore::BoundedMessageStore(uint8_t capacity) {
  if (capacity == 0) {
    capacity_ = 1;
  } else if (capacity > kMaxCapacity) {
    capacity_ = kMaxCapacity;
  } else {
    capacity_ = capacity;
  }
}

bool BoundedMessageStore::isFullLocked() const {
  return size_ == capacity_;
}

uint8_t BoundedMessageStore::size() const {
  return size_;
}

uint8_t BoundedMessageStore::capacity() const {
  return capacity_;
}

uint32_t BoundedMessageStore::droppedCount() const {
  return dropped_;
}

void BoundedMessageStore::clear() {
  head_ = 0;
  tail_ = 0;
  size_ = 0;
  dropped_ = 0;
}

bool BoundedMessageStore::push(const NativePacket &packet) {
  bool evicted = false;
  if (isFullLocked()) {
    head_ = static_cast<uint8_t>((head_ + 1) % capacity_);
    --size_;
    ++dropped_;
    evicted = true;
  }

  entries_[tail_] = packet;
  tail_ = static_cast<uint8_t>((tail_ + 1) % capacity_);
  ++size_;
  return !evicted;
}

bool BoundedMessageStore::peekOldest(NativePacket &packet) const {
  if (size_ == 0) {
    return false;
  }
  packet = entries_[head_];
  return true;
}

bool BoundedMessageStore::peekNewest(NativePacket &packet) const {
  if (size_ == 0) {
    return false;
  }
  const uint8_t index = (tail_ + capacity_ - 1) % capacity_;
  packet = entries_[index];
  return true;
}

bool BoundedMessageStore::pop(NativePacket &packet) {
  if (!peekOldest(packet)) {
    return false;
  }
  head_ = static_cast<uint8_t>((head_ + 1) % capacity_);
  --size_;
  return true;
}

}  // namespace lora_message
