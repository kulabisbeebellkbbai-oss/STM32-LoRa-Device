#pragma once

#include <Arduino.h>
#include <cstring>

#include "app_modes.h"

enum class UsbCommand {
  None,
  Help,
  Status,
  Mode,
  Telemetry,
  Pins,
  Messages,
  Display,
  Apps,
  App,
  Profile,
  ProfileExport,
  ProfileImport,
  Radio,
  RadioAt,
  RadioHelp,
  RadioConfig,
  Modem,
  ModemContract,
  ModemStatus,
  Config,
  ConfigSignoff,
  Sensors,
  I2cScan,
  Version,
  Unknown,
};

struct UsbCommandFrame {
  UsbCommand command{UsbCommand::None};
  bool hasModeArgument{false};
  AppMode requestedMode{AppMode::Standalone};
  bool hasPayloadArgument{false};
  char argument[48]{};
  char raw[24]{};
};

class UsbCommandService {
 public:
  void begin();
  bool poll(UsbCommandFrame &out);

  // Host-test and shared-module helper for command lexing without serial plumbing.
  bool parseLineForTest(const char *line, UsbCommandFrame &out) const;

 private:
  static constexpr uint8_t kMaxLine = 64;
  char buffer[kMaxLine + 1]{};
  uint8_t writeIndex{0};

  bool parseLine(const char *line, UsbCommandFrame &out) const;
};

bool parseUsbCommandLine(const char *line, UsbCommandFrame &out);
