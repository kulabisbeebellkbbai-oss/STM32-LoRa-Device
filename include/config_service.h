#pragma once

#include <Arduino.h>

class ConfigService {
 public:
  struct ConfigStatus {
    const char *profile;
    const char *region;
    const char *authorityReferences;
    const char *frequencyBandMHz;
    bool txLocked;
    bool signedOff;
    bool signoffAttempted;
    const char *signoffReason;
  };

  ConfigService();

  const ConfigStatus &status() const { return status_; }
  bool isTransmitAllowed() const { return !status_.txLocked && status_.signedOff; }
  bool requestSignoffPlaceholder();

  void printStatus(Print &out) const;
  void printSignoffAcknowledgement(Print &out) const;

 private:
  ConfigStatus status_;
};
