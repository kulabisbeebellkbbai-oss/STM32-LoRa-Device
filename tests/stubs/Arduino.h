#pragma once

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>

using uint8_t = std::uint8_t;
using uint16_t = std::uint16_t;
using uint32_t = std::uint32_t;
using uint64_t = std::uint64_t;
using int8_t = std::int8_t;
using int16_t = std::int16_t;
using int32_t = std::int32_t;
using bool_t = bool;

using PrintBuffer = std::string;

#define F(x) x

class Print {
 public:
  virtual ~Print() = default;

  Print() = default;
  Print(const Print &) = default;
  Print &operator=(const Print &) = default;

  void print(const char *text) { append(text); }
  void print(char c) { append(std::string(1, c)); }
  void print(const std::string &text) { append(text); }
  void print(bool value) { append(value ? "true" : "false"); }
  void print(unsigned long value) { append(std::to_string(value)); }
  void print(long value) { append(std::to_string(value)); }
  void print(int value) { append(std::to_string(value)); }
  virtual size_t write(uint8_t value) {
    buffer_.push_back(static_cast<char>(value));
    return 1u;
  }

  void println() { buffer_.push_back('\n'); }
  void println(const char *text) {
    print(text);
    println();
  }

  void clear() { buffer_.clear(); }
  const PrintBuffer &output() const { return buffer_; }

 private:
  void append(const char *text) {
    if (text != nullptr) {
      buffer_.append(text);
    }
  }
  void append(const std::string &text) { buffer_.append(text); }

  PrintBuffer buffer_{};
};
