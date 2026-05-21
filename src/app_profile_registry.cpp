#include "app_profile_registry.h"
#include "radio_service.h"

#include <cstring>

namespace {
constexpr AppProfile kProfiles[] = {
    {"standalone", "Standalone", AppMode::Standalone, false, false, true, true,
     "Standalone remains RF-locked by project policy in this firmware revision."},
    {"meshtastic", "Meshtastic", AppMode::Meshtastic, true, true, false, true,
     "Requires later radio configuration and hardware validation before this mode can be selected."},
    {"meshcore", "MeshCore", AppMode::MeshCore, true, true, false, true,
     "Requires later radio configuration and hardware validation before this mode can be selected."},
    {"lorawan", "LoRaWAN", AppMode::LoRaWAN, true, true, false, true,
     "Requires later radio configuration and hardware validation before this mode can be selected."},
    {"usb-host-interface", "USB Host Interface", AppMode::UsbHostInterface, false, false, true, true,
     "USB host mode is currently diagnostic-only; RF remains locked."},
};

const char *configReadyText(bool configReady) {
  return configReady ? "ready" : "needs_config";
}
}  // namespace

const AppProfile &AppProfileRegistry::profileFor(AppMode mode) const {
  const AppProfile *profile = findByMode(mode);
  if (profile == nullptr) {
    return kProfiles[0];
  }
  return *profile;
}

const AppProfile *AppProfileRegistry::findById(const char *appId) const {
  return findByIdInternal(appId);
}

AppProfileRegistry::SelectionInfo AppProfileRegistry::selectionInfo(
    const AppProfile &profile, const AppProfileSelectionContext &context) const {
  if (profile.txLocked || context.configStatus.txLocked || !context.configStatus.signedOff) {
    return {false, profile.blockedReason};
  }
  if (!profile.configReady) {
    return {false, profile.blockedReason};
  }

  if (profile.requiresRadio && !context.radioStatus.initialized) {
    return {false, "radio subsystem not initialized"};
  }
  if (profile.requiresRadio && !context.radioStatus.serialInitialized) {
    return {false, "radio serial not initialized"};
  }
  if (profile.requiresRadio && !context.radioStatus.moduleInitAttempted) {
    return {false, "radio hardware validation has not started"};
  }
  if (profile.requiresRadio && !context.radioStatus.moduleProbePassed) {
    return {false, "radio module has not completed configuration validation in this firmware."};
  }

  return {true, profile.blockedReason};
}

AppProfileRegistry::SelectionInfo AppProfileRegistry::selectionInfo(
    AppMode mode, const AppProfileSelectionContext &context) const {
  const AppProfile *profile = findByMode(mode);
  if (profile == nullptr) {
    return {false, "unknown application mode"};
  }
  return selectionInfo(*profile, context);
}

void AppProfileRegistry::printProfileLine(Print &out, const AppProfile &profile, AppMode activeMode,
                                         const AppProfileSelectionContext &context) const {
  const AppProfileRegistry::SelectionInfo selection = selectionInfo(profile, context);

  out.print(F("app="));
  out.print(profile.appId);
  out.print(F(" name=\""));
  out.print(profile.appName);
  out.print(F("\" active="));
  out.print(profile.mode == activeMode ? F("yes") : F("no"));
  out.print(F(" requires_radio="));
  out.print(profile.requiresRadio ? F("yes") : F("no"));
  out.print(F(" requires_network_or_peer="));
  out.print(profile.requiresNetworkJoinOrPeer ? F("yes") : F("no"));
  out.print(F(" config_readiness="));
  out.print(configReadyText(profile.configReady));
  out.print(F(" tx_locked="));
  out.print(profile.txLocked ? F("yes") : F("no"));
  out.print(F(" selectable_now="));
  out.print(selection.canSelectNow ? F("yes") : F("no"));
  out.print(F(" reason=\""));
  if (!selection.canSelectNow && selection.blockedReason != nullptr) {
    out.print(selection.blockedReason);
  } else if (selection.canSelectNow) {
    out.print(F(""));
  } else {
    out.print(F("selection blocked"));
  }
  out.print(F("\""));
  out.println();
}

void AppProfileRegistry::printProfiles(Print &out, AppMode activeMode,
                                      const AppProfileSelectionContext &context) const {
  out.print(F("supported app profiles: "));
  out.print(kProfileCount);
  out.println(F(""));
  for (uint8_t index = 0; index < kProfileCount; ++index) {
    printProfileLine(out, kProfiles[index], activeMode, context);
  }
}

const AppProfile *AppProfileRegistry::findByMode(AppMode mode) {
  for (uint8_t i = 0; i < kProfileCount; ++i) {
    if (kProfiles[i].mode == mode) {
      return &kProfiles[i];
    }
  }
  return nullptr;
}

const AppProfile *AppProfileRegistry::findByIdInternal(const char *appId) {
  if (appId == nullptr) {
    return nullptr;
  }
  for (uint8_t i = 0; i < kProfileCount; ++i) {
    if (strcasecmp(kProfiles[i].appId, appId) == 0) {
      return &kProfiles[i];
    }
    if (strcasecmp(kProfiles[i].mode == AppMode::UsbHostInterface ? "usb"
                                                                : appModeName(kProfiles[i].mode),
                   appId) == 0) {
      return &kProfiles[i];
    }
  }
  return nullptr;
}
