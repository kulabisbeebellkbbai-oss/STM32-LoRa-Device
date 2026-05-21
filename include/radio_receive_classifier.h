#pragma once

#include <Arduino.h>

#include "radio_receive_inbox.h"

class RadioReceiveClassifier {
 public:
  bool ingestByte(uint8_t value, RadioReceiveInbox &inbox);
  bool ingestText(const char *text, RadioReceiveInbox &inbox);
  bool flushBufferedLine(RadioReceiveInbox &inbox);
  void reset();

  bool hasBufferedLine() const;

 private:
  static constexpr uint8_t kLineBufferBytes = RadioInboundEvent::kPayloadBytes;
  char lineBuffer_[kLineBufferBytes + 1]{};
  uint8_t lineLength_{0};
  bool sawBinaryByte_{false};

  bool enqueueBufferedLine(RadioReceiveInbox &inbox, bool forceDataFrame);
  static bool isLikelyModemStatus(const char *line, uint8_t lineLength);
  static bool isAtResponseLine(const char *line, uint8_t lineLength);
  static bool isBinaryByte(uint8_t byte);
  static RadioInboundEvent::Kind classifyLine(const char *line, uint8_t lineLength, bool forceDataFrame);
};
