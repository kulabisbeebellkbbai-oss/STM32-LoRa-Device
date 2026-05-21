#pragma once

enum class AppMode {
  Standalone,
  Meshtastic,
  MeshCore,
  LoRaWAN,
  UsbHostInterface,
};

const char *appModeName(AppMode mode);

bool appModeFromString(const char *text, AppMode &mode);
