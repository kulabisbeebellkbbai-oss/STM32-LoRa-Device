#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>

#include "config_service.h"
#include "radio_service.h"
#include "app_profile_registry.h"
#include "usb_command_service.h"
#include "native_packet.h"
#include "node_identity.h"
#include "telemetry_packet_encoder.h"
#include "message_store.h"
#include "safety_audit_log.h"
#include "radio_receive_inbox.h"
#include "radio_receive_classifier.h"
#include "radio_tx_gate.h"

namespace {
using namespace lora_message;

struct TestResult {
  bool passed{true};
};

std::string unescapeInput(const std::string &input) {
  std::string out;
  out.reserve(input.size());
  for (size_t idx = 0; idx < input.size(); ++idx) {
    if (input[idx] != '\\' || idx + 1 >= input.size()) {
      out.push_back(input[idx]);
      continue;
    }

    const char next = input[idx + 1];
    switch (next) {
      case 't':
        out.push_back('\t');
        break;
      case 'n':
        out.push_back('\n');
        break;
      case 'r':
        out.push_back('\r');
        break;
      case '\\':
        out.push_back('\\');
        break;
      default:
        out.push_back('\\');
        out.push_back(next);
        break;
    }
    ++idx;
  }
  return out;
}

bool mapExpectedCommand(const std::string &name, UsbCommand &command) {
  if (name == "Help") {
    command = UsbCommand::Help;
  } else if (name == "Status") {
    command = UsbCommand::Status;
  } else if (name == "Mode") {
    command = UsbCommand::Mode;
  } else if (name == "Telemetry") {
    command = UsbCommand::Telemetry;
  } else if (name == "Display") {
    command = UsbCommand::Display;
  } else if (name == "Apps") {
    command = UsbCommand::Apps;
  } else if (name == "App") {
    command = UsbCommand::App;
  } else if (name == "Radio") {
    command = UsbCommand::Radio;
  } else if (name == "RadioAt") {
    command = UsbCommand::RadioAt;
  } else if (name == "RadioHelp") {
    command = UsbCommand::RadioHelp;
  } else if (name == "RadioConfig") {
    command = UsbCommand::RadioConfig;
  } else if (name == "Config") {
    command = UsbCommand::Config;
  } else if (name == "ConfigSignoff") {
    command = UsbCommand::ConfigSignoff;
  } else if (name == "Sensors") {
    command = UsbCommand::Sensors;
  } else if (name == "I2cScan") {
    command = UsbCommand::I2cScan;
  } else if (name == "Pins") {
    command = UsbCommand::Pins;
  } else if (name == "Version") {
    command = UsbCommand::Version;
  } else if (name == "Messages") {
    command = UsbCommand::Messages;
  } else if (name == "Profile") {
    command = UsbCommand::Profile;
  } else if (name == "ProfileExport") {
    command = UsbCommand::ProfileExport;
  } else if (name == "ProfileImport") {
    command = UsbCommand::ProfileImport;
  } else if (name == "Modem") {
    command = UsbCommand::Modem;
  } else if (name == "ModemStatus") {
    command = UsbCommand::ModemStatus;
  } else if (name == "ModemContract") {
    command = UsbCommand::ModemContract;
  } else if (name == "Unknown") {
    command = UsbCommand::Unknown;
  } else {
    return false;
  }
  return true;
}

std::string commandName(UsbCommand command) {
  switch (command) {
    case UsbCommand::Help:
      return "Help";
    case UsbCommand::Status:
      return "Status";
    case UsbCommand::Mode:
      return "Mode";
    case UsbCommand::Telemetry:
      return "Telemetry";
    case UsbCommand::Pins:
      return "Pins";
    case UsbCommand::Display:
      return "Display";
    case UsbCommand::Apps:
      return "Apps";
    case UsbCommand::App:
      return "App";
    case UsbCommand::Radio:
      return "Radio";
    case UsbCommand::RadioAt:
      return "RadioAt";
    case UsbCommand::RadioHelp:
      return "RadioHelp";
    case UsbCommand::RadioConfig:
      return "RadioConfig";
    case UsbCommand::Config:
      return "Config";
    case UsbCommand::ConfigSignoff:
      return "ConfigSignoff";
    case UsbCommand::Sensors:
      return "Sensors";
    case UsbCommand::I2cScan:
      return "I2cScan";
    case UsbCommand::Version:
      return "Version";
    case UsbCommand::Messages:
      return "Messages";
    case UsbCommand::Profile:
      return "Profile";
    case UsbCommand::ProfileExport:
      return "ProfileExport";
    case UsbCommand::ProfileImport:
      return "ProfileImport";
    case UsbCommand::Modem:
      return "Modem";
    case UsbCommand::ModemStatus:
      return "ModemStatus";
    case UsbCommand::ModemContract:
      return "ModemContract";
    case UsbCommand::Unknown:
      return "Unknown";
    case UsbCommand::None:
      return "None";
  }
  return "Unknown";
}

void runUsbCommandCases(TestResult &result, const std::string &tsvPath, int &total) {
  std::ifstream file(tsvPath);
  if (!file.is_open()) {
    std::cerr << "FAIL: unable to open command case file: " << tsvPath << '\n';
    result.passed = false;
    return;
  }

  std::string line;
  int lineNo = 0;
  int passed = 0;
  int failed = 0;
  while (std::getline(file, line)) {
    ++lineNo;
    if (line.empty() || line.rfind("#", 0) == 0) {
      continue;
    }
    if (lineNo == 1 || line.rfind("input_c_escape", 0) == 0) {
      continue;
    }

    const size_t firstTab = line.find('\t');
    const size_t secondTab = line.find('\t', firstTab + 1);
    if (firstTab == std::string::npos || secondTab == std::string::npos) {
      std::cerr << "FAIL: malformed test row on line " << lineNo << '\n';
      ++failed;
      continue;
    }

    const auto input = unescapeInput(line.substr(0, firstTab));
    const auto expectedName = line.substr(firstTab + 1, secondTab - firstTab - 1);
    UsbCommand expected = UsbCommand::Unknown;
    if (!mapExpectedCommand(expectedName, expected)) {
      std::cerr << "FAIL: unknown expected command '" << expectedName << "' on line "
                << lineNo << '\n';
      ++failed;
      continue;
    }

    UsbCommandFrame frame{};
    const bool parsed = parseUsbCommandLine(input.c_str(), frame);
    if (!parsed || frame.command != expected) {
      ++failed;
      std::cerr << "FAIL: line " << lineNo << " expected " << expectedName << " got "
                << commandName(frame.command) << " input '" << input << "'\n";
      continue;
    }

    if (frame.command == UsbCommand::Mode && input.find("lorawan") != std::string::npos &&
        input.find("extra") == std::string::npos) {
      if (!frame.hasModeArgument || frame.requestedMode != AppMode::LoRaWAN) {
        ++failed;
        std::cerr << "FAIL: line " << lineNo << " mode lorawan did not preserve mode argument\n";
        continue;
      }
    }

    if (frame.command == UsbCommand::Mode && input == "mode" && frame.hasModeArgument) {
      ++failed;
      std::cerr << "FAIL: line " << lineNo << " bare mode unexpectedly has an argument\n";
      continue;
    }

    if (frame.command == UsbCommand::App && input == "app" && frame.hasModeArgument) {
      ++failed;
      std::cerr << "FAIL: line " << lineNo << " bare app unexpectedly has an argument\n";
      continue;
    }

    if (frame.command == UsbCommand::App && input.find("app lorawan") != std::string::npos &&
        input.find("extra") == std::string::npos) {
      if (!frame.hasModeArgument || frame.requestedMode != AppMode::LoRaWAN) {
        ++failed;
        std::cerr << "FAIL: line " << lineNo << " app lorawan did not preserve mode argument\n";
        continue;
      }
    }

    if (frame.command == UsbCommand::ProfileExport && input == "profile export lorawan" &&
        (!frame.hasModeArgument || frame.requestedMode != AppMode::LoRaWAN)) {
      ++failed;
      std::cerr << "FAIL: line " << lineNo << " profile export lorawan did not preserve mode argument\n";
      continue;
    }

    if (frame.command == UsbCommand::ProfileExport && input == "profile export") {
      if (frame.hasPayloadArgument) {
        ++failed;
        std::cerr << "FAIL: line " << lineNo
                  << " profile export without argument incorrectly marked payload\n";
        continue;
      }
    }

    if (frame.command == UsbCommand::ProfileImport && input == "profile import test_payload") {
      if (!frame.hasPayloadArgument || std::strcmp(frame.argument, "test_payload") != 0) {
        ++failed;
        std::cerr << "FAIL: line " << lineNo << " profile import did not preserve payload\n";
        continue;
      }
    }

    if (frame.command == UsbCommand::ProfileImport && input == "profile import") {
      if (frame.hasPayloadArgument) {
        ++failed;
        std::cerr << "FAIL: line " << lineNo << " profile import without payload set unexpected payload\n";
        continue;
      }
    }

    if (frame.command == UsbCommand::Unknown && input == "unknown" &&
        std::strcmp(frame.raw, "unknown") != 0) {
      ++failed;
      std::cerr << "FAIL: line " << lineNo << " unknown command did not preserve raw token\n";
      continue;
    }

    ++passed;
    ++total;
  }

  if (failed == 0) {
    std::cout << "PASS: USB command cases from " << tsvPath << " (" << passed << ")\n";
  } else {
    std::cerr << "FAIL: USB command cases (" << failed << " failing)\n";
    result.passed = false;
  }
}

void runConfigServiceChecks(TestResult &result) {
  ConfigService config;
  auto status = config.status();
  if (!status.txLocked || status.signedOff || status.signoffAttempted || config.isTransmitAllowed()) {
    result.passed = false;
    std::cerr << "FAIL: ConfigService default invariants violated\n";
    std::cerr << "  txLocked=" << (status.txLocked ? "true" : "false")
              << " signedOff=" << (status.signedOff ? "true" : "false")
              << " signoffAttempted=" << (status.signoffAttempted ? "true" : "false")
              << " txAllowed=" << (config.isTransmitAllowed() ? "true" : "false") << '\n';
  }

  const bool placeholder = config.requestSignoffPlaceholder();
  status = config.status();
  if (placeholder || !status.txLocked || status.signedOff || !status.signoffAttempted ||
      config.isTransmitAllowed()) {
    result.passed = false;
    std::cerr << "FAIL: ConfigService requestSignoffPlaceholder invariants violated\n";
    std::cerr << "  placeholderResult=" << (placeholder ? "true" : "false")
              << " txLocked=" << (status.txLocked ? "true" : "false")
              << " signedOff=" << (status.signedOff ? "true" : "false")
              << " signoffAttempted=" << (status.signoffAttempted ? "true" : "false")
              << " txAllowed=" << (config.isTransmitAllowed() ? "true" : "false") << '\n';
  }

  Print statusOutput;
  config.printStatus(statusOutput);
  const std::string statusText = statusOutput.output();
  if (statusText.find("txLocked=true") == std::string::npos ||
      statusText.find("signedOff=false") == std::string::npos ||
      statusText.find("signoffAttempted=true") == std::string::npos ||
      statusText.find("txAllowed=false") == std::string::npos) {
    result.passed = false;
    std::cerr << "FAIL: ConfigService printStatus missing locked-state fields\n"
              << statusText << '\n';
  }

  Print acknowledgementOutput;
  config.printSignoffAcknowledgement(acknowledgementOutput);
  const std::string acknowledgementText = acknowledgementOutput.output();
  if (acknowledgementText.find("non-authorizing") == std::string::npos ||
      acknowledgementText.find("No transmit authorization") == std::string::npos ||
      acknowledgementText.find("txAllowed=false") == std::string::npos) {
    result.passed = false;
    std::cerr << "FAIL: ConfigService signoff acknowledgement is not clearly locked\n"
              << acknowledgementText << '\n';
  }

  if (result.passed) {
    std::cout << "PASS: ConfigService invariants\n";
  }
}

void runAppProfileChecks(TestResult &result) {
  const AppProfileRegistry registry;
  if (AppProfileRegistry::kProfileCount == 0 || AppProfileRegistry::kProfileCount > 255) {
    result.passed = false;
    std::cerr << "FAIL: AppProfileRegistry profile count is invalid\n";
    return;
  }

  RadioStatus radioStatus{};
  radioStatus.initialized = true;
  radioStatus.serialInitialized = true;
  radioStatus.moduleInitAttempted = true;
  radioStatus.moduleProbePassed = true;
  ConfigService config;
  const AppProfileSelectionContext context{radioStatus, config.status()};

  const AppProfile &standalone = registry.profileFor(AppMode::Standalone);
  if (standalone.requiresRadio || standalone.requiresNetworkJoinOrPeer || !standalone.configReady ||
      !standalone.txLocked) {
    result.passed = false;
    std::cerr << "FAIL: Standalone profile has unexpected status fields\n";
  }

  const AppProfile &meshtastic = registry.profileFor(AppMode::Meshtastic);
  if (registry.selectionInfo(AppMode::Meshtastic, context).canSelectNow || meshtastic.configReady ||
      !meshtastic.requiresRadio || !meshtastic.requiresNetworkJoinOrPeer) {
    result.passed = false;
    std::cerr << "FAIL: Meshtastic profile does not reflect conservative lock state\n";
  }

  const AppProfile *lorawan = registry.findById("lorawan");
  if (lorawan == nullptr || lorawan->mode != AppMode::LoRaWAN) {
    result.passed = false;
    std::cerr << "FAIL: unable to resolve LoRaWAN profile by app id\n";
  }

  if (registry.selectionInfo(AppMode::UsbHostInterface, context).canSelectNow) {
    const auto selection = registry.selectionInfo(AppMode::UsbHostInterface, context);
    result.passed = false;
    std::cerr << "FAIL: USB host profile bypassed TX/config lock: "
              << (selection.blockedReason != nullptr ? selection.blockedReason : "unknown") << '\n';
  }

  AppProfile unknownProfile = registry.profileFor(AppMode::Standalone);
  if (unknownProfile.mode != AppMode::Standalone) {
    result.passed = false;
    std::cerr << "FAIL: AppProfileRegistry fallback mode is not standalone\n";
  }

  RadioStatus radioDisabled{};
  const AppProfileSelectionContext disabledContext{radioDisabled, config.status()};
  const auto blockedSelection = registry.selectionInfo(AppMode::Meshtastic, disabledContext);
  if (blockedSelection.canSelectNow || blockedSelection.blockedReason == nullptr) {
    result.passed = false;
    std::cerr << "FAIL: Meshtastic selection status must block selection now\n";
  }

  if (result.passed) {
    std::cout << "PASS: App profile registry\n";
  }
}

std::string inboundEventKindName(RadioInboundEvent::Kind kind) {
  switch (kind) {
    case RadioInboundEvent::Kind::AtResponse:
      return "AtResponse";
    case RadioInboundEvent::Kind::ModemStatus:
      return "ModemStatus";
    case RadioInboundEvent::Kind::DataFrame:
      return "DataFrame";
    case RadioInboundEvent::Kind::Unknown:
      return "Unknown";
  }
  return "Unknown";
}

void runRadioReceiveChecks(TestResult &result) {
  RadioReceiveInbox inbox;
  RadioReceiveClassifier classifier;
  RadioInboundEvent event{};

  const bool parsed = classifier.ingestText("OK\n+STATUS:BUSY\npayload-frame\n", inbox);
  if (!parsed) {
    result.passed = false;
    std::cerr << "FAIL: radio receive classifier should accept text input\n";
    return;
  }

  if (!inbox.dequeue(event) || event.kind != RadioInboundEvent::Kind::AtResponse || event.payload[0] != 'O') {
    result.passed = false;
    std::cerr << "FAIL: first inbound event kind mismatch: ";
    if (event.kind == RadioInboundEvent::Kind::AtResponse) {
      std::cerr << "AtResponse";
    } else {
      std::cerr << inboundEventKindName(event.kind);
    }
    std::cerr << '\n';
    return;
  }

  if (!inbox.dequeue(event) || event.kind != RadioInboundEvent::Kind::ModemStatus) {
    result.passed = false;
    std::cerr << "FAIL: second inbound event should be modem status\n";
    return;
  }

  if (!inbox.dequeue(event) || event.kind != RadioInboundEvent::Kind::DataFrame ||
      std::strncmp(event.payload, "payload-frame", 13) != 0) {
    result.passed = false;
    std::cerr << "FAIL: third inbound event should be data frame\n";
    return;
  }

  if (!inbox.injectPayload("injected", 8)) {
    result.passed = false;
    std::cerr << "FAIL: could not inject payload frame\n";
    return;
  }
  if (!inbox.dequeue(event) || event.kind != RadioInboundEvent::Kind::DataFrame ||
      std::strcmp(event.payload, "injected") != 0 || !event.injected) {
    result.passed = false;
    std::cerr << "FAIL: injected payload event not preserved\n";
    return;
  }

  if (inbox.count() != 0) {
    result.passed = false;
    std::cerr << "FAIL: injected payload should be consumed before queue-empty checks\n";
    return;
  }

  for (uint8_t idx = 0; idx < (RadioReceiveInbox::kDepth + 2); ++idx) {
    const char payloadLine[2] = {(char)('a' + idx % 26), '\0'};
    inbox.injectPayload(payloadLine, 1);
  }

  if (inbox.count() != RadioReceiveInbox::kDepth) {
    result.passed = false;
    std::cerr << "FAIL: inbox should clamp at configured depth\n";
    return;
  }

  if (result.passed) {
    std::cout << "PASS: Radio inbound classifier/inbox checks\n";
  }
}

void runRadioTxGateChecks(TestResult &result) {
  const AppProfile unlockedProfile{"open", "Open", AppMode::Standalone, false, false, true, false, "unlocked"};
  ConfigService config;
  const ConfigService::ConfigStatus lockedConfig = config.status();

  RadioTxGate gate;
  RadioStatus radio{};
  radio.initialized = true;
  radio.transmitGuarded = false;

  const RadioTxGateResult denied = gate.evaluate(radio, lockedConfig, unlockedProfile);
  if (denied.allowed || denied.denyReason == nullptr ||
      std::string(denied.denyReason).empty()) {
    result.passed = false;
    std::cerr << "FAIL: tx gate should deny when config/profile state is locked\n";
    return;
  }

  radio.transmitGuarded = true;
  const RadioTxGateResult guardDenied = gate.evaluate(radio, lockedConfig, unlockedProfile);
  if (guardDenied.allowed || guardDenied.denyReason == nullptr ||
      std::string(guardDenied.denyReason).empty()) {
    result.passed = false;
    std::cerr << "FAIL: tx gate should provide guard denial reason\n";
    return;
  }

  ConfigService::ConfigStatus unlockedStatus = lockedConfig;
  unlockedStatus.txLocked = false;
  unlockedStatus.signedOff = true;
  radio.transmitGuarded = false;
  const RadioTxGateResult allowed = gate.evaluate(radio, unlockedStatus, unlockedProfile);
  if (!allowed.allowed) {
    result.passed = false;
    std::cerr << "FAIL: tx gate should allow unlocked standalone path with mocked module\n";
    return;
  }

  radio.mocked = false;
  radio.available = true;
  radio.auxBusy = true;
  const RadioTxGateResult busy = gate.evaluate(radio, unlockedStatus, unlockedProfile);
  if (busy.allowed) {
    result.passed = false;
    std::cerr << "FAIL: tx gate should deny when AUX reports busy\n";
    return;
  }

  if (result.passed) {
    std::cout << "PASS: Radio tx gate checks\n";
  }
}

void runPacketModelChecks(TestResult &result) {
  constexpr NodeIdentity sourceIdentity{1, 0x01020304, 2};
  constexpr NodeIdentity destinationIdentity{2, 0x05060708, 3};

  TelemetryModel telemetry{};
  telemetry.sampleIndex = 11;
  telemetry.sampleMs = 1234;
  telemetry.uptimeMs = 98765;
  telemetry.mode = AppMode::Meshtastic;
  telemetry.sensorsValid = true;
  telemetry.batteryVoltage = 4.12f;
  telemetry.batteryPercent = 87.5f;
  telemetry.ambientTemperatureC = 22.3f;
  telemetry.ambientHumidityPercent = 55.2f;
  telemetry.pressureHPa = 1004.5f;
  telemetry.accelX = 1.2f;
  telemetry.accelY = -0.3f;
  telemetry.accelZ = 0.9f;
  telemetry.gpsLatitude = 43.65107f;
  telemetry.gpsLongitude = -79.347015f;
  telemetry.gpsFix = true;
  telemetry.gpsSatellites = 10;

  NativePacket telemetryPacket{};
  if (!makeTelemetryPacket(telemetry, sourceIdentity, destinationIdentity, telemetryPacket)) {
    result.passed = false;
    std::cerr << "FAIL: telemetry packet encoding failed\n";
    return;
  }

  SerializedPacket serialized{};
  if (!serializePacket(telemetryPacket, serialized)) {
    result.passed = false;
    std::cerr << "FAIL: telemetry packet serialize failed\n";
    return;
  }

  NativePacket parsed{};
  if (!parsePacket(serialized.bytes.data(), serialized.size, parsed)) {
    result.passed = false;
    std::cerr << "FAIL: telemetry packet parse failed\n";
    return;
  }

  TelemetryPayload payload{};
  if (!decodeTelemetryPayload(parsed, payload)) {
    result.passed = false;
    std::cerr << "FAIL: telemetry payload decode failed\n";
    return;
  }

  if (payload.sampleIndex != telemetry.sampleIndex || parsed.source != sourceIdentity ||
      parsed.destination != destinationIdentity || parsed.type != PacketType::Telemetry) {
    result.passed = false;
    std::cerr << "FAIL: telemetry packet roundtrip metadata mismatch\n";
    return;
  }

  TextPacketPayload textPayload{};
  if (!buildTextPayload("hello", 2, textPayload)) {
    result.passed = false;
    std::cerr << "FAIL: text payload build failed\n";
    return;
  }
  NativePacket textPacket{};
  if (!makeTextPacket(textPayload, sourceIdentity, destinationIdentity, 12, 1000, textPacket)) {
    result.passed = false;
    std::cerr << "FAIL: text packet build failed\n";
    return;
  }
  if (textPacket.payloadLength != sizeof(TextPacketPayload) ||
      std::strncmp(textPayload.message, reinterpret_cast<char *>(textPacket.payload + 1),
                   sizeof(textPayload.message)) != 0) {
    result.passed = false;
    std::cerr << "FAIL: text packet payload mismatch\n";
    return;
  }

  uint8_t arg[] = {1, 2, 3, 4};
  ControlPacketPayload controlPayload{};
  if (!buildControlPayload(7, arg, 4, controlPayload)) {
    result.passed = false;
    std::cerr << "FAIL: control payload build failed\n";
    return;
  }
  NativePacket controlPacket{};
  if (!makeControlPacket(controlPayload, sourceIdentity, destinationIdentity, 13, 1001, controlPacket) ||
      controlPacket.type != PacketType::Control) {
    result.passed = false;
    std::cerr << "FAIL: control packet build failed\n";
    return;
  }

  DiagnosticPacketPayload diagPayload{};
  if (!buildDiagnosticPayload(2, 0x10AA, "diag", diagPayload)) {
    result.passed = false;
    std::cerr << "FAIL: diagnostic payload build failed\n";
    return;
  }
  NativePacket diagPacket{};
  if (!makeDiagnosticPacket(diagPayload, sourceIdentity, destinationIdentity, 14, 1002, diagPacket) ||
      diagPacket.type != PacketType::Diagnostic) {
    result.passed = false;
    std::cerr << "FAIL: diagnostic packet build failed\n";
    return;
  }

  SerializedPacket badSerialized = serialized;
  if (badSerialized.size > 0) {
    badSerialized.bytes[0] ^= 0xFF;
    NativePacket badPacket{};
    if (parsePacket(badSerialized.bytes.data(), badSerialized.size, badPacket)) {
      result.passed = false;
      std::cerr << "FAIL: corrupted packet passed parse CRC check\n";
      return;
    }
  }

  std::cout << "PASS: packet model checks\n";
}

void runMessageStoreAndAuditChecks(TestResult &result) {
  NodeIdentity nodeA{0xABCD, 1, 1};
  NodeIdentity nodeB{0xBEEF, 2, 2};
  NodeContact contactA{};
  setContactDisplayName(contactA, "NodeA");
  setContactCallsign(contactA, "A1");
  setContactEmail(contactA, "a@example.com");
  contactA.identity = nodeA;

  if (!isValidIdentity(nodeA) || isIdentityZero(nodeA) || !isValidIdentity(nodeB)) {
    result.passed = false;
    std::cerr << "FAIL: node identity validation failed\n";
    return;
  }

  BoundedMessageStore store(3);
  if (store.capacity() != 3) {
    result.passed = false;
    std::cerr << "FAIL: message store capacity not applied\n";
    return;
  }

  NativePacket pkt1{}, pkt2{}, pkt3{}, pkt4{};
  makeDiagnosticPacket(DiagnosticPacketPayload{}, nodeA, nodeB, 1, 1, pkt1);
  makeDiagnosticPacket(DiagnosticPacketPayload{}, nodeA, nodeB, 2, 2, pkt2);
  makeDiagnosticPacket(DiagnosticPacketPayload{}, nodeA, nodeB, 3, 3, pkt3);
  makeDiagnosticPacket(DiagnosticPacketPayload{}, nodeA, nodeB, 4, 4, pkt4);
  store.push(pkt1);
  store.push(pkt2);
  store.push(pkt3);
  if (store.size() != 3) {
    result.passed = false;
    std::cerr << "FAIL: message store size incorrect before overflow\n";
    return;
  }
  if (!store.push(pkt4)) {
    if (store.size() != 3 || store.droppedCount() != 1) {
      result.passed = false;
      std::cerr << "FAIL: message store overflow accounting failed\n";
      return;
    }
  } else {
    result.passed = false;
    std::cerr << "FAIL: message store did not report overflow on bounded append\n";
    return;
  }

  NativePacket first{};
  if (!store.peekOldest(first) || first.sequence != 2) {
    result.passed = false;
    std::cerr << "FAIL: message store oldest element should be sequence 2 after overflow\n";
    return;
  }

  BoundedMessageStore emptyStore(1);
  NativePacket pop{};
  if (emptyStore.pop(pop)) {
    result.passed = false;
    std::cerr << "FAIL: popping empty message store succeeded\n";
    return;
  }

  SafetyAuditLog audit(2);
  if (audit.capacity() != 2) {
    result.passed = false;
    std::cerr << "FAIL: audit log capacity mismatch\n";
    return;
  }
  audit.push(AuditSeverity::Info, AuditEventCode::QueueOverflow, nodeA, 1, "first");
  audit.push(AuditSeverity::Warning, AuditEventCode::PacketOverflow, nodeB, 2, "second");
  audit.push(AuditSeverity::Critical, AuditEventCode::PacketParseFailure, nodeA, 3, "third");
  if (audit.size() != 2 || audit.dropped() != 1) {
    result.passed = false;
    std::cerr << "FAIL: audit log drop count incorrect\n";
    return;
  }
  SafetyAuditEvent latest{};
  if (!audit.latest(latest) || latest.eventMs != 3 || latest.code != AuditEventCode::PacketParseFailure) {
    result.passed = false;
    std::cerr << "FAIL: audit log latest event incorrect\n";
    return;
  }

  SafetyAuditEvent oldest{};
  if (!audit.oldest(oldest) || oldest.eventMs != 2) {
    result.passed = false;
    std::cerr << "FAIL: audit log oldest event incorrect\n";
    return;
  }

  std::cout << "PASS: message store and audit checks\n";
}

}  // namespace

int main(int argc, char **argv) {
  const std::string tsvPath =
      (argc > 1 ? argv[1] : std::string("tests/usb-command-cases.tsv"));
  TestResult result;
  int totalCases = 0;
  runUsbCommandCases(result, tsvPath, totalCases);
  runConfigServiceChecks(result);
  runAppProfileChecks(result);
  runRadioReceiveChecks(result);
  runRadioTxGateChecks(result);
  runPacketModelChecks(result);
  runMessageStoreAndAuditChecks(result);

  std::cout << "Host unit tests complete. Command cases run: " << totalCases << '\n';
  return result.passed ? 0 : 1;
}
