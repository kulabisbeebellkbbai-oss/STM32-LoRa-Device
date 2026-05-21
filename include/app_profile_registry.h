#pragma once

#include <Arduino.h>
#include <cstdint>

#include "app_modes.h"
#include "config_service.h"

struct RadioStatus;

struct AppProfile {
  const char *appId;
  const char *appName;
  AppMode mode;
  bool requiresRadio;
  bool requiresNetworkJoinOrPeer;
  bool configReady;
  bool txLocked;
  const char *blockedReason;
};

struct AppProfileSelectionContext {
  const RadioStatus &radioStatus;
  const ConfigService::ConfigStatus &configStatus;
};

class AppProfileRegistry {
 public:
  static constexpr uint8_t kProfileCount = 5;

  struct SelectionInfo {
    bool canSelectNow;
    const char *blockedReason;
  };

  const AppProfile &profileFor(AppMode mode) const;
  const AppProfile *findById(const char *appId) const;
  SelectionInfo selectionInfo(const AppProfile &profile, const AppProfileSelectionContext &context) const;
  SelectionInfo selectionInfo(AppMode mode, const AppProfileSelectionContext &context) const;
  void printProfiles(Print &out, AppMode activeMode, const AppProfileSelectionContext &context) const;
  void printProfileLine(Print &out, const AppProfile &profile, AppMode activeMode,
                       const AppProfileSelectionContext &context) const;

 private:
  static const AppProfile *findByMode(AppMode mode);
  static const AppProfile *findByIdInternal(const char *appId);
};
