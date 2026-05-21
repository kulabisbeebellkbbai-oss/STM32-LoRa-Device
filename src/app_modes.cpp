#include "app_modes.h"

#include <strings.h>

const char *appModeName(AppMode mode) {
  switch (mode) {
    case AppMode::Standalone:
      return "standalone";
    case AppMode::Meshtastic:
      return "meshtastic";
    case AppMode::MeshCore:
      return "meshcore";
    case AppMode::LoRaWAN:
      return "lorawan";
    case AppMode::UsbHostInterface:
      return "usb-host-interface";
    default:
      return "unknown";
  }
}

bool appModeFromString(const char *text, AppMode &mode) {
  if (text == nullptr) {
    return false;
  }

  if (strcasecmp(text, "standalone") == 0 || strcasecmp(text, "stand") == 0) {
    mode = AppMode::Standalone;
    return true;
  }
  if (strcasecmp(text, "meshtastic") == 0) {
    mode = AppMode::Meshtastic;
    return true;
  }
  if (strcasecmp(text, "meshcore") == 0 || strcasecmp(text, "mesh-core") == 0) {
    mode = AppMode::MeshCore;
    return true;
  }
  if (strcasecmp(text, "lorawan") == 0 || strcasecmp(text, "lora") == 0) {
    mode = AppMode::LoRaWAN;
    return true;
  }
  if (strcasecmp(text, "usb") == 0 || strcasecmp(text, "usb-host-interface") == 0 ||
      strcasecmp(text, "usbinterface") == 0) {
    mode = AppMode::UsbHostInterface;
    return true;
  }
  return false;
}
