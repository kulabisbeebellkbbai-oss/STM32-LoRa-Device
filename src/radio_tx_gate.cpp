#include "radio_tx_gate.h"

#include "app_profile_registry.h"
#include "config_service.h"
#include "radio_service.h"

namespace {
constexpr const char *kTxDenyNotInitialized = "radio not initialized";
constexpr const char *kTxDenyTransmitGuard = "radio transmit remains guard-locked";
constexpr const char *kTxDenyProfileLocked = "active profile policy forbids transmit";
constexpr const char *kTxDenyConfigLocked = "regulatory signoff required before transmit";
constexpr const char *kTxDenyRadioUnavailable = "radio hardware unavailable";
constexpr const char *kTxDenyRadioBusy = "radio AUX/busy signal indicates blocked transmit window";
}  // namespace

RadioTxGateResult RadioTxGate::evaluate(const RadioStatus &radioStatus,
                                        const ConfigService::ConfigStatus &configStatus,
                                        const AppProfile &profile) const {
  if (!radioStatus.initialized) {
    return {false, kTxDenyNotInitialized};
  }

  if (configStatus.txLocked || !configStatus.signedOff) {
    return {false, kTxDenyConfigLocked};
  }

  if (profile.txLocked) {
    return {false, kTxDenyProfileLocked};
  }

  if (radioStatus.transmitGuarded) {
    return {false, kTxDenyTransmitGuard};
  }

  if (profile.requiresRadio && !radioStatus.available) {
    return {false, kTxDenyRadioUnavailable};
  }

  if (radioStatus.auxBusy || radioStatus.isBusy) {
    return {false, kTxDenyRadioBusy};
  }

  return {true, nullptr};
}
