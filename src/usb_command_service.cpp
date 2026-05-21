#include "usb_command_service.h"
#include "device_config.h"

void UsbCommandService::begin() {
  writeIndex = 0;
}

bool UsbCommandService::parseLine(const char *line, UsbCommandFrame &out) const {
  return parseUsbCommandLine(line, out);
}

bool UsbCommandService::parseLineForTest(const char *line, UsbCommandFrame &out) const {
  return parseLine(line, out);
}

bool UsbCommandService::poll(UsbCommandFrame &out) {
  while (LORA_DEVICE_CONSOLE.available()) {
    const char c = static_cast<char>(LORA_DEVICE_CONSOLE.read());

    if (c == '\r') {
      continue;
    }

    if (c == '\n') {
      buffer[writeIndex] = '\0';
      writeIndex = 0;

      bool parsed = parseLine(buffer, out);
      if (parsed) {
        return true;
      }
      return false;
    }

    if (writeIndex < kMaxLine) {
      buffer[writeIndex++] = c;
    }
  }

  return false;
}
