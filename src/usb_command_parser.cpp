#include <cstring>

#include "app_modes.h"
#include "usb_command_service.h"

namespace {
constexpr uint8_t kParserMaxLine = 64;
constexpr const char *kTokenDelimiters = " \t";

bool isLineEmpty(const char *line) {
  if (line == nullptr) {
    return true;
  }
  for (const char *cursor = line; *cursor != '\0'; ++cursor) {
    if (*cursor != ' ' && *cursor != '\t') {
      return false;
    }
  }
  return true;
}

void normalizeLine(char *line) {
  for (char *cursor = line; *cursor != '\0'; ++cursor) {
    if (*cursor >= 'A' && *cursor <= 'Z') {
      *cursor = static_cast<char>(*cursor + ('a' - 'A'));
    }
  }
}

void copyRawToken(const char *token, UsbCommandFrame &out) {
  out.raw[0] = '\0';
  if (token == nullptr) {
    return;
  }
  strncpy(out.raw, token, sizeof(out.raw) - 1);
  out.raw[sizeof(out.raw) - 1] = '\0';
}

void copyArgumentToken(const char *token, UsbCommandFrame &out) {
  out.argument[0] = '\0';
  out.hasPayloadArgument = false;
  if (token == nullptr) {
    return;
  }
  strncpy(out.argument, token, sizeof(out.argument) - 1);
  out.argument[sizeof(out.argument) - 1] = '\0';
  out.hasPayloadArgument = true;
}

void clearOptionalArguments(UsbCommandFrame &out) {
  out.hasModeArgument = false;
  out.requestedMode = AppMode::Standalone;
  out.hasPayloadArgument = false;
  out.argument[0] = '\0';
}

bool hasExtraArgument() {
  char *extra = strtok(nullptr, kTokenDelimiters);
  return extra != nullptr && *extra != '\0';
}
}  // namespace

bool parseUsbCommandLine(const char *line, UsbCommandFrame &out) {
  if (isLineEmpty(line)) {
    return false;
  }

  char temp[kParserMaxLine + 1];
  strncpy(temp, line, sizeof(temp) - 1);
  temp[sizeof(temp) - 1] = '\0';
  normalizeLine(temp);

  out = UsbCommandFrame{};
  out.command = UsbCommand::Unknown;
  clearOptionalArguments(out);
  out.raw[0] = '\0';

  char *token = strtok(temp, kTokenDelimiters);
  if (token == nullptr) {
    return false;
  }

  if (strcmp(token, "help") == 0) {
    if (hasExtraArgument()) {
      copyRawToken(token, out);
      return true;
    }
    out.command = UsbCommand::Help;
    return true;
  }

  if (strcmp(token, "status") == 0) {
    if (hasExtraArgument()) {
      copyRawToken(token, out);
      return true;
    }
    out.command = UsbCommand::Status;
    return true;
  }

  if (strcmp(token, "telemetry") == 0) {
    if (hasExtraArgument()) {
      copyRawToken(token, out);
      return true;
    }
    out.command = UsbCommand::Telemetry;
    return true;
  }

  if (strcmp(token, "messages") == 0) {
    if (hasExtraArgument()) {
      copyRawToken(token, out);
      return true;
    }
    out.command = UsbCommand::Messages;
    return true;
  }

  if (strcmp(token, "pins") == 0) {
    if (hasExtraArgument()) {
      copyRawToken(token, out);
      return true;
    }
    out.command = UsbCommand::Pins;
    return true;
  }

  if (strcmp(token, "display") == 0) {
    if (hasExtraArgument()) {
      copyRawToken(token, out);
      return true;
    }
    out.command = UsbCommand::Display;
    return true;
  }

  if (strcmp(token, "apps") == 0) {
    if (hasExtraArgument()) {
      copyRawToken(token, out);
      return true;
    }
    out.command = UsbCommand::Apps;
    return true;
  }

  if (strcmp(token, "profile") == 0) {
    clearOptionalArguments(out);
    char *subcommand = strtok(nullptr, kTokenDelimiters);
    if (subcommand == nullptr) {
      out.command = UsbCommand::Profile;
      return true;
    }
    if (strcmp(subcommand, "export") == 0) {
      char *arg = strtok(nullptr, kTokenDelimiters);
      if (arg != nullptr) {
        copyArgumentToken(arg, out);
        AppMode parsed;
        if (appModeFromString(arg, parsed)) {
          out.hasModeArgument = true;
          out.requestedMode = parsed;
        }
        if (hasExtraArgument()) {
          out.command = UsbCommand::Unknown;
        } else {
          out.command = UsbCommand::ProfileExport;
        }
      } else {
        out.command = UsbCommand::ProfileExport;
      }
      return true;
    }
    if (strcmp(subcommand, "import") == 0) {
      char *arg = strtok(nullptr, kTokenDelimiters);
      if (arg != nullptr) {
        copyArgumentToken(arg, out);
      }
      if (hasExtraArgument()) {
        out.command = UsbCommand::Unknown;
      } else {
        out.command = UsbCommand::ProfileImport;
      }
      return true;
    }
    copyRawToken(token, out);
    return true;
  }

  if (strcmp(token, "app") == 0) {
    char *arg = strtok(nullptr, kTokenDelimiters);
    if (arg == nullptr) {
      out.command = UsbCommand::App;
      out.hasModeArgument = false;
      return true;
    }
    out.raw[0] = '\0';
    strncpy(out.raw, arg, sizeof(out.raw) - 1);
    out.raw[sizeof(out.raw) - 1] = '\0';
    AppMode parsed;
    if (appModeFromString(arg, parsed)) {
      if (hasExtraArgument()) {
        out.command = UsbCommand::Unknown;
      } else {
        out.command = UsbCommand::App;
        out.hasModeArgument = true;
        out.requestedMode = parsed;
      }
    } else {
      out.command = UsbCommand::Unknown;
    }
    return true;
  }

  if (strcmp(token, "radio") == 0) {
    char *subcommand = strtok(nullptr, kTokenDelimiters);
    if (subcommand == nullptr) {
      out.command = UsbCommand::Radio;
      return true;
    }
    if (strcmp(subcommand, "at") == 0) {
      if (hasExtraArgument()) {
        out.command = UsbCommand::Unknown;
      } else {
        out.command = UsbCommand::RadioAt;
      }
      return true;
    }
    if (strcmp(subcommand, "help") == 0) {
      if (hasExtraArgument()) {
        out.command = UsbCommand::Unknown;
      } else {
        out.command = UsbCommand::RadioHelp;
      }
      return true;
    }
    if (strcmp(subcommand, "config") == 0) {
      if (hasExtraArgument()) {
        out.command = UsbCommand::Unknown;
      } else {
        out.command = UsbCommand::RadioConfig;
      }
      return true;
    }
    copyRawToken(token, out);
    return true;
  }

  if (strcmp(token, "modem") == 0) {
    char *subcommand = strtok(nullptr, kTokenDelimiters);
    if (subcommand == nullptr) {
      out.command = UsbCommand::Modem;
      return true;
    }
    if (strcmp(subcommand, "contract") == 0) {
      if (hasExtraArgument()) {
        out.command = UsbCommand::Unknown;
      } else {
        out.command = UsbCommand::ModemContract;
      }
      return true;
    }
    if (strcmp(subcommand, "status") == 0) {
      if (hasExtraArgument()) {
        out.command = UsbCommand::Unknown;
      } else {
        out.command = UsbCommand::ModemStatus;
      }
      return true;
    }
    copyRawToken(token, out);
    return true;
  }

  if (strcmp(token, "config") == 0) {
    char *subcommand = strtok(nullptr, kTokenDelimiters);
    if (subcommand == nullptr) {
      out.command = UsbCommand::Config;
      return true;
    }
    if (strcmp(subcommand, "signoff") == 0) {
      if (hasExtraArgument()) {
        out.command = UsbCommand::Unknown;
      } else {
        out.command = UsbCommand::ConfigSignoff;
      }
      return true;
    }
    copyRawToken(token, out);
    return true;
  }

  if (strcmp(token, "sensors") == 0) {
    if (hasExtraArgument()) {
      copyRawToken(token, out);
      return true;
    }
    out.command = UsbCommand::Sensors;
    return true;
  }

  if (strcmp(token, "i2cscan") == 0) {
    if (hasExtraArgument()) {
      copyRawToken(token, out);
      return true;
    }
    out.command = UsbCommand::I2cScan;
    return true;
  }

  if (strcmp(token, "version") == 0) {
    if (hasExtraArgument()) {
      copyRawToken(token, out);
      return true;
    }
    out.command = UsbCommand::Version;
    return true;
  }

  if (strcmp(token, "mode") == 0) {
    char *arg = strtok(nullptr, kTokenDelimiters);
    if (arg != nullptr && *arg != '\0') {
      out.raw[0] = '\0';
      strncpy(out.raw, arg, sizeof(out.raw) - 1);
      out.raw[sizeof(out.raw) - 1] = '\0';
      AppMode parsed;
      if (appModeFromString(arg, parsed)) {
        if (hasExtraArgument()) {
          out.command = UsbCommand::Unknown;
        } else {
          out.command = UsbCommand::Mode;
          out.hasModeArgument = true;
          out.requestedMode = parsed;
        }
      } else {
        out.command = UsbCommand::Unknown;
      }
    } else {
      out.command = UsbCommand::Mode;
      out.hasModeArgument = false;
    }
    return true;
  }

  copyRawToken(token, out);
  return true;
}
