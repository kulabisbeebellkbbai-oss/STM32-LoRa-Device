#pragma once

#include <Arduino.h>
#include "config_service.h"

struct AppProfile;
struct RadioStatus;

struct RadioTxGateResult {
  bool allowed{false};
  const char *denyReason{nullptr};
};

class RadioTxGate {
 public:
  RadioTxGateResult evaluate(const RadioStatus &radioStatus,
                            const ConfigService::ConfigStatus &configStatus,
                            const AppProfile &profile) const;
};
