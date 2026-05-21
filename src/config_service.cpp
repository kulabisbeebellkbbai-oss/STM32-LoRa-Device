#include "config_service.h"

namespace {
constexpr ConfigService::ConfigStatus kCanadaOntarioRadioProfile{
    "ca_902_928_tx_locked",
    "Canada/Ontario",
    "ISED RSS-210 Issue 11 (2024-06-25); RSS-247; RSS-Gen; RIC-67",
    "902-928 MHz",
    true,
    false,
    false,
    "Transmit remains locked pending documented authority and approved module review.",
};
}  // namespace

ConfigService::ConfigService() : status_(kCanadaOntarioRadioProfile) {}

bool ConfigService::requestSignoffPlaceholder() {
  status_.signoffAttempted = true;
  status_.txLocked = true;
  status_.signedOff = false;
  status_.signoffReason = "Signoff placeholder recorded; TX remains locked pending documented authority and approved module review.";
  return false;
}

void ConfigService::printStatus(Print &out) const {
  out.print(F("config profile="));
  out.print(status_.profile);
  out.print(F(" region=\""));
  out.print(status_.region);
  out.print(F("\" authority=\""));
  out.print(status_.authorityReferences);
  out.print(F("\" band="));
  out.print(status_.frequencyBandMHz);
  out.print(F(" txLocked="));
  out.print(status_.txLocked ? F("true") : F("false"));
  out.print(F(" signedOff="));
  out.print(status_.signedOff ? F("true") : F("false"));
  out.print(F(" signoffAttempted="));
  out.print(status_.signoffAttempted ? F("true") : F("false"));
  out.print(F(" txAllowed="));
  out.print(isTransmitAllowed() ? F("true") : F("false"));
  out.print(F(" reason=\""));
  out.print(status_.signoffReason);
  out.println(F("\""));
}

void ConfigService::printSignoffAcknowledgement(Print &out) const {
  out.println(F("config signoff request accepted as non-authorizing acknowledgement only"));
  out.println(F("No transmit authorization has been granted in this firmware revision."));
  out.print(F("Current lock state: "));
  printStatus(out);
}
