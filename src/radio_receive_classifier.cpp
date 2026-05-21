#include "radio_receive_classifier.h"

#include <cstring>

namespace {

bool endsWith(const char *text, const char *suffix) {
  if (text == nullptr || suffix == nullptr) {
    return false;
  }
  const size_t textLength = std::strlen(text);
  const size_t suffixLength = std::strlen(suffix);
  if (suffixLength > textLength) {
    return false;
  }
  return std::strncmp(text + textLength - suffixLength, suffix, suffixLength) == 0;
}

bool startsWithCaseInsensitive(const char *text, const char *prefix) {
  if (text == nullptr || prefix == nullptr) {
    return false;
  }
  for (size_t idx = 0;; ++idx) {
    const char lhs = text[idx];
    const char rhs = prefix[idx];
    if (rhs == '\0') {
      return true;
    }
    if (lhs == '\0') {
      return false;
    }
    const char lhsLower = (lhs >= 'A' && lhs <= 'Z') ? static_cast<char>(lhs + ('a' - 'A')) : lhs;
    const char rhsLower = (rhs >= 'A' && rhs <= 'Z') ? static_cast<char>(rhs + ('a' - 'A')) : rhs;
    if (lhsLower != rhsLower) {
      return false;
    }
  }
}

}  // namespace

bool RadioReceiveClassifier::ingestByte(uint8_t value, RadioReceiveInbox &inbox) {
  if (value == '\r') {
    return true;
  }
  if (value == '\n') {
    return enqueueBufferedLine(inbox, false);
  }

  if (lineLength_ < (kLineBufferBytes - 1)) {
    lineBuffer_[lineLength_++] = static_cast<char>(value);
    lineBuffer_[lineLength_] = '\0';
  }

  if (isBinaryByte(value)) {
    sawBinaryByte_ = true;
  }

  if (lineLength_ >= (kLineBufferBytes - 1) && value != '\n' && value != '\r') {
    return enqueueBufferedLine(inbox, true);
  }
  return true;
}

bool RadioReceiveClassifier::ingestText(const char *text, RadioReceiveInbox &inbox) {
  if (text == nullptr) {
    return false;
  }
  for (const char *cursor = text; *cursor != '\0'; ++cursor) {
    if (!ingestByte(static_cast<uint8_t>(*cursor), inbox)) {
      return false;
    }
  }
  return true;
}

bool RadioReceiveClassifier::flushBufferedLine(RadioReceiveInbox &inbox) {
  return enqueueBufferedLine(inbox, true);
}

void RadioReceiveClassifier::reset() {
  lineLength_ = 0;
  lineBuffer_[0] = '\0';
  sawBinaryByte_ = false;
}

bool RadioReceiveClassifier::hasBufferedLine() const {
  return lineLength_ > 0;
}

bool RadioReceiveClassifier::enqueueBufferedLine(RadioReceiveInbox &inbox, bool forceDataFrame) {
  const RadioInboundEvent::Kind eventKind = classifyLine(lineBuffer_, lineLength_, forceDataFrame);
  const bool enqueued = inbox.enqueue(eventKind, lineBuffer_, lineLength_);
  reset();
  return enqueued;
}

bool RadioReceiveClassifier::isLikelyModemStatus(const char *line, uint8_t lineLength) {
  if (line == nullptr || lineLength == 0) {
    return false;
  }
  if (line[0] == '+') {
    return true;
  }
  if (startsWithCaseInsensitive(line, "RDY") || startsWithCaseInsensitive(line, "READY") ||
      startsWithCaseInsensitive(line, "BUSY")) {
    return true;
  }
  return false;
}

bool RadioReceiveClassifier::isAtResponseLine(const char *line, uint8_t lineLength) {
  if (line == nullptr || lineLength == 0) {
    return false;
  }
  if (endsWith(line, "OK")) {
    return true;
  }
  return startsWithCaseInsensitive(line, "OK") || (startsWithCaseInsensitive(line, "ERROR") ||
                                                    strstr(line, "+CME ERROR") != nullptr ||
                                                    strstr(line, "ERROR:") != nullptr);
}

bool RadioReceiveClassifier::isBinaryByte(uint8_t byte) {
  return byte < 0x09 || (byte > 0x0D && byte < 0x20) || byte >= 0x7F;
}

RadioInboundEvent::Kind RadioReceiveClassifier::classifyLine(const char *line, uint8_t lineLength,
                                                           bool forceDataFrame) {
  if (line == nullptr || lineLength == 0) {
    return RadioInboundEvent::Kind::Unknown;
  }
  if (forceDataFrame) {
    return RadioInboundEvent::Kind::DataFrame;
  }
  if (isAtResponseLine(line, lineLength)) {
    return RadioInboundEvent::Kind::AtResponse;
  }
  if (isLikelyModemStatus(line, lineLength)) {
    return RadioInboundEvent::Kind::ModemStatus;
  }
  return RadioInboundEvent::Kind::DataFrame;
}
