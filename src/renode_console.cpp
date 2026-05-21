#include "device_config.h"

#if defined(RENODE_SIMULATION)

namespace {
constexpr uintptr_t kUsart1Base = 0x40013800UL;
volatile uint32_t &reg(uintptr_t offset) {
  return *reinterpret_cast<volatile uint32_t *>(kUsart1Base + offset);
}

constexpr uintptr_t kCr1 = 0x00;
constexpr uintptr_t kBrr = 0x0C;
constexpr uintptr_t kIsr = 0x1C;
constexpr uintptr_t kRdr = 0x24;
constexpr uintptr_t kTdr = 0x28;

constexpr uint32_t kCr1Ue = 1UL << 0;
constexpr uint32_t kCr1Re = 1UL << 2;
constexpr uint32_t kCr1Te = 1UL << 3;
constexpr uint32_t kIsrRxne = 1UL << 5;
}

RenodeConsole renodeConsole;

void RenodeConsole::begin(uint32_t baud) {
  const uint32_t divisor = baud == 0 ? 1 : (lora_device::kSerialBaudRate / baud);
  reg(kBrr) = divisor == 0 ? 1 : divisor;
  reg(kCr1) = kCr1Ue | kCr1Re | kCr1Te;
}

int RenodeConsole::available() {
  return (reg(kIsr) & kIsrRxne) != 0 ? 1 : 0;
}

int RenodeConsole::read() {
  if (!available()) {
    return -1;
  }
  return static_cast<int>(reg(kRdr) & 0xFF);
}

size_t RenodeConsole::write(uint8_t value) {
  reg(kTdr) = value;
  return 1;
}

#endif
