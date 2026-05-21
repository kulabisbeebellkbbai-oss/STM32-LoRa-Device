#include <Arduino.h>

#include "app_modes.h"
#include "display_service.h"
#include "device_config.h"
#include "input_service.h"
#include "radio_service.h"
#include "config_service.h"
#include "app_profile_registry.h"
#include "message_store.h"
#include "node_identity.h"
#include "safety_audit_log.h"
#include "sensor_service.h"
#include "telemetry_packet_encoder.h"
#include "usb_command_service.h"

namespace {
using lora_message::AuditEventCode;
using lora_message::AuditSeverity;
using lora_message::BoundedMessageStore;
using lora_message::NativePacket;
using lora_message::NodeIdentity;
using lora_message::SafetyAuditEvent;
using lora_message::SafetyAuditLog;

#if defined(RENODE_SIMULATION)
constexpr uint32_t kSerialWaitMs = 0;
#else
constexpr uint32_t kSerialWaitMs = 200;
#endif

constexpr const char *kRadioConfigAtCommand = "AT+HELP\r\n";
constexpr const char *kFirmwareName = "LoRaWB LoRa Endpoint";
constexpr NodeIdentity kLocalNode{124, 0x00000001UL, 1};
constexpr NodeIdentity kBroadcastNode{124, 0xFFFFFFFFUL, 0};

const uint8_t kAppProfileCount = 5;
const AppMode kAppProfileModes[kAppProfileCount] = {
    AppMode::Standalone, AppMode::Meshtastic, AppMode::MeshCore, AppMode::LoRaWAN,
    AppMode::UsbHostInterface};

AppMode activeMode = AppMode::Standalone;
unsigned long lastTelemetryMs = 0;
unsigned long lastStatusMs = 0;
unsigned long lastRadioMs = 0;
unsigned long lastDisplayMs = 0;

#if defined(LORA_SENSOR_HARDWARE_ACTIVE) && !defined(RENODE_SIMULATION)
constexpr bool kSensorStubOnly = false;
#else
constexpr bool kSensorStubOnly = true;
#endif

#if defined(LORA_DISPLAY_HARDWARE_ACTIVE) && defined(LORA_DISPLAY_ST7789) && !defined(RENODE_SIMULATION)
constexpr bool kDisplayStubOnly = false;
#else
constexpr bool kDisplayStubOnly = true;
#endif

#if defined(LORA_RADIO_HARDWARE_ACTIVE) && defined(LORA_RADIO_DX_LR03_SERIAL) && !defined(RENODE_SIMULATION)
constexpr bool kRadioHardwareAvailable = true;
#else
constexpr bool kRadioHardwareAvailable = false;
#endif

SensorService sensorService(kSensorStubOnly);
InputService inputService;
DisplayService displayService(kDisplayStubOnly);
RadioService radioService(kRadioHardwareAvailable);
ConfigService configService;
AppProfileRegistry appProfileRegistry;
UsbCommandService commandService;
TelemetryModel latestTelemetry;
BoundedMessageStore messageStore(12);
SafetyAuditLog auditLog(16);

void printFirmwareMetadata() {
  LORA_DEVICE_CONSOLE.print(F("firmware_name="));
  LORA_DEVICE_CONSOLE.print(kFirmwareName);
  LORA_DEVICE_CONSOLE.print(F(" version="));
#ifdef LORA_ENDPOINT_FW_VERSION
  LORA_DEVICE_CONSOLE.print(LORA_ENDPOINT_FW_VERSION);
#else
  LORA_DEVICE_CONSOLE.print(F("dev"));
#endif
  LORA_DEVICE_CONSOLE.print(F(" serial_baud="));
  LORA_DEVICE_CONSOLE.print(lora_device::kSerialBaudRate);
  LORA_DEVICE_CONSOLE.print(F(" telemetry_interval_ms="));
  LORA_DEVICE_CONSOLE.print(lora_device::kTelemetryIntervalMs);
  LORA_DEVICE_CONSOLE.print(F(" display_interval_ms="));
  LORA_DEVICE_CONSOLE.print(lora_device::kDisplayIntervalMs);
  LORA_DEVICE_CONSOLE.print(F(" status_interval_ms="));
  LORA_DEVICE_CONSOLE.print(lora_device::kStatusIntervalMs);
  LORA_DEVICE_CONSOLE.print(F(" radio_interval_ms="));
  LORA_DEVICE_CONSOLE.print(lora_device::kRadioIntervalMs);
}

void printBootBanner() {
  LORA_DEVICE_CONSOLE.println();
  LORA_DEVICE_CONSOLE.println(F("LoRaWB LoRa Endpoint boot"));
  LORA_DEVICE_CONSOLE.print(F("Firmware version: "));
#ifdef LORA_ENDPOINT_FW_VERSION
  LORA_DEVICE_CONSOLE.println(LORA_ENDPOINT_FW_VERSION);
#else
  LORA_DEVICE_CONSOLE.println(F("dev"));
#endif
  LORA_DEVICE_CONSOLE.print(F("Firmware: "));
  printFirmwareMetadata();
  LORA_DEVICE_CONSOLE.print(F(" hardware=STM32WB55"));
  LORA_DEVICE_CONSOLE.print(F(" serial_console=1"));
  LORA_DEVICE_CONSOLE.println();
  LORA_DEVICE_CONSOLE.println(F("Hardware drivers are compile-gated; RF transmit remains guarded."));
  LORA_DEVICE_CONSOLE.println(F("Type 'help' for serial commands."));
}

void printHelp() {
  LORA_DEVICE_CONSOLE.println(F("commands:"));
  LORA_DEVICE_CONSOLE.println(F("  help          show this message"));
  LORA_DEVICE_CONSOLE.println(F("  status        show mode, telemetry freshness, and radio state"));
  LORA_DEVICE_CONSOLE.println(F("  mode [name]   print current mode or set by value"));
  LORA_DEVICE_CONSOLE.println(F("  telemetry     print latest telemetry snapshot"));
  LORA_DEVICE_CONSOLE.println(F("  messages      print and label current GUI message model"));
  LORA_DEVICE_CONSOLE.println(F("  display       show display diagnostics state"));
  LORA_DEVICE_CONSOLE.println(F("  radio         show radio diagnostics state"));
  LORA_DEVICE_CONSOLE.println(F("  modem         show project-local serial modem contract"));
  LORA_DEVICE_CONSOLE.println(F("  modem status  show radio diagnostics through modem contract surface"));
  LORA_DEVICE_CONSOLE.println(F("  modem contract show explicit modem contract schema"));
  LORA_DEVICE_CONSOLE.println(F("  apps          print supported application profiles"));
  LORA_DEVICE_CONSOLE.println(F("  app [name]    show profile details and request gated selection"));
  LORA_DEVICE_CONSOLE.println(F("  profile       print profile surface data"));
  LORA_DEVICE_CONSOLE.println(F("  profile export [name]  print profile schema snapshot"));
  LORA_DEVICE_CONSOLE.println(F("  profile import [payload] record profile payload read-only"));
  LORA_DEVICE_CONSOLE.println(F("  radio at      request AT probe on DX-LR03 serial radio"));
  LORA_DEVICE_CONSOLE.println(F("  radio help    request vendor AT help query (AT+HELP)"));
  LORA_DEVICE_CONSOLE.println(F("  radio config  request vendor AT help/config discovery query (AT+HELP)"));
  LORA_DEVICE_CONSOLE.println(F("  config        print regulatory config profile and TX lock state"));
  LORA_DEVICE_CONSOLE.println(F("  config signoff placeholder for config sign-off workflow (no TX unlock in firmware)"));
  LORA_DEVICE_CONSOLE.println(F("  sensors       show sensor and GPS diagnostics state"));
  LORA_DEVICE_CONSOLE.println(F("  i2cscan       run compile-gated sensor I2C scan"));
  LORA_DEVICE_CONSOLE.println(F("  pins          print planned input pin reads"));
  LORA_DEVICE_CONSOLE.println(F("  version       print firmware version"));
}

void printModemStatus() {
  LORA_DEVICE_CONSOLE.println(F("modem status surface"));
  LORA_DEVICE_CONSOLE.print(F("contract=at-uart-v1 profile=serial-only"));
  LORA_DEVICE_CONSOLE.print(F(" tx_locked="));
  LORA_DEVICE_CONSOLE.print(configService.status().txLocked ? F("true") : F("false"));
  LORA_DEVICE_CONSOLE.print(F(" serial_mode=dx-lr03"));
  LORA_DEVICE_CONSOLE.print(F(" serial_baud="));
  LORA_DEVICE_CONSOLE.print(lora_device::kSerialBaudRate);
  LORA_DEVICE_CONSOLE.print(F(" service_ready="));
  LORA_DEVICE_CONSOLE.print(radioService.status().available ? F("yes") : F("no"));
  LORA_DEVICE_CONSOLE.println();
}

void printModemContract() {
  LORA_DEVICE_CONSOLE.println(F("modem_contract profile=v1"));
  LORA_DEVICE_CONSOLE.println(F("interface transport=uart"));
  LORA_DEVICE_CONSOLE.print(F("pins tx="));
  LORA_DEVICE_CONSOLE.print(lora_device::PinConfig::kRadioUartTx);
  LORA_DEVICE_CONSOLE.print(F(" rx="));
  LORA_DEVICE_CONSOLE.print(lora_device::PinConfig::kRadioUartRx);
  LORA_DEVICE_CONSOLE.print(F(" m0="));
  LORA_DEVICE_CONSOLE.print(lora_device::PinConfig::kRadioModeControl);
  LORA_DEVICE_CONSOLE.print(F(" m1="));
  LORA_DEVICE_CONSOLE.print(lora_device::PinConfig::kRadioSleepControl);
  LORA_DEVICE_CONSOLE.print(F(" aux="));
  LORA_DEVICE_CONSOLE.print(lora_device::PinConfig::kRadioAux);
  LORA_DEVICE_CONSOLE.println();
  LORA_DEVICE_CONSOLE.println(F("at_commands_read_only: AT, AT+HELP, AT+PARAM?"));
  LORA_DEVICE_CONSOLE.println(F("framing: ASCII lines terminated by CRLF"));
  LORA_DEVICE_CONSOLE.println(F("scope: project-local status and command contract only"));
  LORA_DEVICE_CONSOLE.println(F("compatibility_claims: no KISS/RNode emulation guarantee"));
}

void printDisplayStatus() {
  LORA_DEVICE_CONSOLE.println(F("display diagnostics surface"));
  displayService.printStatus(LORA_DEVICE_CONSOLE);
}

void printRadioStatus() {
  radioService.printStatus(LORA_DEVICE_CONSOLE);
}

void printConfigStatus() {
  configService.printStatus(LORA_DEVICE_CONSOLE);
}

void printAppProfiles() {
  const AppProfileSelectionContext context{radioService.status(), configService.status()};
  appProfileRegistry.printProfiles(LORA_DEVICE_CONSOLE, activeMode, context);
  LORA_DEVICE_CONSOLE.println(F("surface=app_profiles"));
}

void printProfileExportLine(const AppProfile &profile, const AppProfileSelectionContext &context) {
  const auto selection = appProfileRegistry.selectionInfo(profile, context);
  LORA_DEVICE_CONSOLE.println(F("{"));
  LORA_DEVICE_CONSOLE.print(F("  \"schema\": \"lora-endpoint-profile-v1\","));
  LORA_DEVICE_CONSOLE.print(F(" \"appId\": \""));
  LORA_DEVICE_CONSOLE.print(profile.appId);
  LORA_DEVICE_CONSOLE.print(F("\", \"name\": \""));
  LORA_DEVICE_CONSOLE.print(profile.appName);
  LORA_DEVICE_CONSOLE.print(F("\", \"mode\": \""));
  LORA_DEVICE_CONSOLE.print(appModeName(profile.mode));
  LORA_DEVICE_CONSOLE.print(F("\","));
  LORA_DEVICE_CONSOLE.print(F(" \"requires_radio\": "));
  LORA_DEVICE_CONSOLE.print(profile.requiresRadio ? F("true") : F("false"));
  LORA_DEVICE_CONSOLE.print(F(", \"requires_network_or_peer\": "));
  LORA_DEVICE_CONSOLE.print(profile.requiresNetworkJoinOrPeer ? F("true") : F("false"));
  LORA_DEVICE_CONSOLE.print(F(", \"config_ready\": "));
  LORA_DEVICE_CONSOLE.print(profile.configReady ? F("true") : F("false"));
  LORA_DEVICE_CONSOLE.print(F(", \"tx_locked\": "));
  LORA_DEVICE_CONSOLE.print(profile.txLocked ? F("true") : F("false"));
  LORA_DEVICE_CONSOLE.print(F(", \"selectable_now\": "));
  LORA_DEVICE_CONSOLE.print(selection.canSelectNow ? F("true") : F("false"));
  LORA_DEVICE_CONSOLE.print(F(", \"blocked_reason\": \""));
  if (selection.blockedReason != nullptr) {
    LORA_DEVICE_CONSOLE.print(selection.blockedReason);
  } else {
    LORA_DEVICE_CONSOLE.print(F("ok"));
  }
  LORA_DEVICE_CONSOLE.println(F("\" }"));
}

void printProfileExport(const UsbCommandFrame &frame) {
  const AppProfileSelectionContext context{radioService.status(), configService.status()};
  if (frame.hasModeArgument) {
    const AppProfile &profile = appProfileRegistry.profileFor(frame.requestedMode);
    LORA_DEVICE_CONSOLE.print(F("profile_export mode=\""));
    LORA_DEVICE_CONSOLE.print(appModeName(frame.requestedMode));
    LORA_DEVICE_CONSOLE.println(F("\""));
    printProfileExportLine(profile, context);
    return;
  }

  LORA_DEVICE_CONSOLE.println(F("profile_export mode=all"));
  for (uint8_t idx = 0; idx < kAppProfileCount; ++idx) {
    printProfileExportLine(appProfileRegistry.profileFor(kAppProfileModes[idx]), context);
  }
}

void printProfileImport(const UsbCommandFrame &frame) {
  LORA_DEVICE_CONSOLE.println(F("profile_import"));
  if (!frame.hasPayloadArgument) {
    LORA_DEVICE_CONSOLE.println(F("usage: profile import <json-or-token>"));
    LORA_DEVICE_CONSOLE.println(F("This firmware validates import syntax only; no persistence write is performed."));
    return;
  }
  LORA_DEVICE_CONSOLE.print(F("read_only payload="));
  LORA_DEVICE_CONSOLE.print(frame.argument);
  LORA_DEVICE_CONSOLE.println(F(" (echo-only)"));
  LORA_DEVICE_CONSOLE.println(F("stored_profiles_unchanged=true"));
}

void printMessagesSurface() {
  LORA_DEVICE_CONSOLE.print(F("messages size="));
  LORA_DEVICE_CONSOLE.print(messageStore.size());
  LORA_DEVICE_CONSOLE.print(F(" capacity="));
  LORA_DEVICE_CONSOLE.print(messageStore.capacity());
  LORA_DEVICE_CONSOLE.print(F(" dropped="));
  LORA_DEVICE_CONSOLE.print(messageStore.droppedCount());
  NativePacket newest{};
  if (messageStore.peekNewest(newest)) {
    LORA_DEVICE_CONSOLE.print(F(" newest_type="));
    LORA_DEVICE_CONSOLE.print(static_cast<int>(newest.type));
    LORA_DEVICE_CONSOLE.print(F(" newest_seq="));
    LORA_DEVICE_CONSOLE.print(newest.sequence);
    LORA_DEVICE_CONSOLE.print(F(" payload_len="));
    LORA_DEVICE_CONSOLE.print(newest.payloadLength);
  } else {
    LORA_DEVICE_CONSOLE.print(F(" newest=none"));
  }
  LORA_DEVICE_CONSOLE.println();

  SafetyAuditEvent latest{};
  LORA_DEVICE_CONSOLE.print(F("audit size="));
  LORA_DEVICE_CONSOLE.print(auditLog.size());
  LORA_DEVICE_CONSOLE.print(F(" dropped="));
  LORA_DEVICE_CONSOLE.print(auditLog.dropped());
  if (auditLog.latest(latest)) {
    LORA_DEVICE_CONSOLE.print(F(" latest_code="));
    LORA_DEVICE_CONSOLE.print(static_cast<int>(latest.code));
    LORA_DEVICE_CONSOLE.print(F(" latest_msg=\""));
    LORA_DEVICE_CONSOLE.print(latest.message);
    LORA_DEVICE_CONSOLE.print(F("\""));
  } else {
    LORA_DEVICE_CONSOLE.print(F(" latest=none"));
  }
  LORA_DEVICE_CONSOLE.println();

  LORA_DEVICE_CONSOLE.print(F("messages_surface "));
  displayService.printGuiState(LORA_DEVICE_CONSOLE);
}

void printSensorStatus() {
  sensorService.printStatus(LORA_DEVICE_CONSOLE);
}

void runI2cScan() {
  sensorService.requestI2cScan();
  sensorService.scanI2cBus();
  sensorService.printStatus(LORA_DEVICE_CONSOLE);
}

void printFirmwareVersion() {
  LORA_DEVICE_CONSOLE.print(F("firmware "));
  printFirmwareMetadata();
  LORA_DEVICE_CONSOLE.println();
}

void printStatusLine(const TelemetryModel &telemetry, const InputService::State &inputs) {
  const auto &gui = displayService.guiModel();
  LORA_DEVICE_CONSOLE.print(F("status "));
  printFirmwareMetadata();
  LORA_DEVICE_CONSOLE.print(F(" mode="));
  LORA_DEVICE_CONSOLE.print(appModeName(activeMode));
  LORA_DEVICE_CONSOLE.print(F(" telemetry_seq="));
  LORA_DEVICE_CONSOLE.print(telemetry.sampleIndex);
  LORA_DEVICE_CONSOLE.print(F(" gui="));
  LORA_DEVICE_CONSOLE.print(displayService.guiSurfaceName(gui.activeSurface));
  LORA_DEVICE_CONSOLE.print(F(" radio_online="));
  LORA_DEVICE_CONSOLE.print(radioService.isOnline() ? F("yes") : F("no"));
  LORA_DEVICE_CONSOLE.print(F(" radio_initialized="));
  LORA_DEVICE_CONSOLE.print(radioService.status().initialized ? F("yes") : F("no"));
  LORA_DEVICE_CONSOLE.print(F(" radio_available="));
  LORA_DEVICE_CONSOLE.print(radioService.status().available ? F("yes") : F("no"));
  LORA_DEVICE_CONSOLE.print(F(" radio_mocked="));
  LORA_DEVICE_CONSOLE.print(radioService.status().mocked ? F("yes") : F("no"));
  LORA_DEVICE_CONSOLE.print(F(" messages="));
  LORA_DEVICE_CONSOLE.print(messageStore.size());
  LORA_DEVICE_CONSOLE.print(F(" audit="));
  LORA_DEVICE_CONSOLE.print(auditLog.size());
  LORA_DEVICE_CONSOLE.print(F(" outputs="));
  LORA_DEVICE_CONSOLE.print(inputs.backPressed ? F("back ") : F(""));
  LORA_DEVICE_CONSOLE.print(inputs.homePressed ? F("home ") : F(""));
  LORA_DEVICE_CONSOLE.print(inputs.encoderSwitch ? F("sw ") : F(""));
  LORA_DEVICE_CONSOLE.print(inputs.encoderA ? F("enc_a ") : F(""));
  LORA_DEVICE_CONSOLE.print(inputs.encoderB ? F("enc_b ") : F(""));
  LORA_DEVICE_CONSOLE.print(F("uptime_ms="));
  LORA_DEVICE_CONSOLE.print(telemetry.uptimeMs);
  LORA_DEVICE_CONSOLE.println();
}

void printPins() {
  LORA_DEVICE_CONSOLE.print(F("pins encoderA="));
  LORA_DEVICE_CONSOLE.print(lora_device::PinConfig::kEncoderA);
  LORA_DEVICE_CONSOLE.print(F(" encoderB="));
  LORA_DEVICE_CONSOLE.print(lora_device::PinConfig::kEncoderB);
  LORA_DEVICE_CONSOLE.print(F(" enc_sw="));
  LORA_DEVICE_CONSOLE.print(lora_device::PinConfig::kEncoderSwitch);
  LORA_DEVICE_CONSOLE.print(F(" back="));
  LORA_DEVICE_CONSOLE.print(lora_device::PinConfig::kBackButton);
  LORA_DEVICE_CONSOLE.print(F(" home="));
  LORA_DEVICE_CONSOLE.print(lora_device::PinConfig::kHomeButton);
  LORA_DEVICE_CONSOLE.print(F(" radio_toggle="));
  LORA_DEVICE_CONSOLE.print(lora_device::PinConfig::kRadioEnableToggle);
  LORA_DEVICE_CONSOLE.print(F(" profile_toggle="));
  LORA_DEVICE_CONSOLE.print(lora_device::PinConfig::kProfileToggle);
  LORA_DEVICE_CONSOLE.println();

  const auto &inputs = inputService.snapshot();
  LORA_DEVICE_CONSOLE.print(F("pinstates back="));
  LORA_DEVICE_CONSOLE.print(inputs.backPressed ? F("1") : F("0"));
  LORA_DEVICE_CONSOLE.print(F(" home="));
  LORA_DEVICE_CONSOLE.print(inputs.homePressed ? F("1") : F("0"));
  LORA_DEVICE_CONSOLE.print(F(" encA="));
  LORA_DEVICE_CONSOLE.print(inputs.encoderA ? F("1") : F("0"));
  LORA_DEVICE_CONSOLE.print(F(" encB="));
  LORA_DEVICE_CONSOLE.print(inputs.encoderB ? F("1") : F("0"));
  LORA_DEVICE_CONSOLE.print(F(" encSw="));
  LORA_DEVICE_CONSOLE.print(inputs.encoderSwitch ? F("1") : F("0"));
  LORA_DEVICE_CONSOLE.print(F(" radio_en="));
  LORA_DEVICE_CONSOLE.print(inputs.radioEnableToggle ? F("1") : F("0"));
  LORA_DEVICE_CONSOLE.print(F(" profile="));
  LORA_DEVICE_CONSOLE.print(inputs.profileToggle ? F("low") : F("normal"));
  LORA_DEVICE_CONSOLE.println();

  inputService.printState(LORA_DEVICE_CONSOLE);
  LORA_DEVICE_CONSOLE.println();
}

void setActiveMode(AppMode newMode) {
  if (activeMode == newMode) {
    LORA_DEVICE_CONSOLE.print(F("mode already "));
    LORA_DEVICE_CONSOLE.println(appModeName(activeMode));
    return;
  }

  activeMode = newMode;
  radioService.setMode(newMode);
  LORA_DEVICE_CONSOLE.print(F("mode set "));
  LORA_DEVICE_CONSOLE.println(appModeName(activeMode));
}

void applyModeFromProfile(const AppProfile &profile) {
  const AppProfileSelectionContext context{radioService.status(), configService.status()};
  const auto selection = appProfileRegistry.selectionInfo(profile, context);
  if (!selection.canSelectNow) {
    LORA_DEVICE_CONSOLE.print(F("app profile not selectable now: "));
    LORA_DEVICE_CONSOLE.println(selection.blockedReason);
    return;
  }
  setActiveMode(profile.mode);
}

void printSelectedAppProfile(const AppProfile &profile) {
  const AppProfileSelectionContext context{radioService.status(), configService.status()};
  appProfileRegistry.printProfileLine(LORA_DEVICE_CONSOLE, profile, activeMode, context);
}

void handleProfileSelection(AppMode newMode) {
  const AppProfile &profile = appProfileRegistry.profileFor(newMode);
  applyModeFromProfile(profile);
}

void printTelemetrySnapshot() {
  latestTelemetry.printTelemetryLine(LORA_DEVICE_CONSOLE);
  LORA_DEVICE_CONSOLE.println();
}

void recordTelemetryPacket(const TelemetryModel &telemetry) {
  NativePacket packet{};
  if (!lora_message::makeTelemetryPacket(telemetry, kLocalNode, kBroadcastNode, packet)) {
    auditLog.push(AuditSeverity::Error, AuditEventCode::PacketParseFailure, kLocalNode,
                  static_cast<uint32_t>(telemetry.sampleMs), "telemetry encode failed");
    return;
  }

  if (!messageStore.push(packet)) {
    auditLog.push(AuditSeverity::Warning, AuditEventCode::QueueOverflow, kLocalNode,
                  static_cast<uint32_t>(telemetry.sampleMs), "message store overflow");
  }
}

void recordRadioInboundEvents(unsigned long nowMs) {
  RadioInboundEvent event{};
  while (radioService.pollInboundEvent(event)) {
    const char *kind = "unknown";
    switch (event.kind) {
      case RadioInboundEvent::Kind::AtResponse:
        kind = "at";
        break;
      case RadioInboundEvent::Kind::ModemStatus:
        kind = "status";
        break;
      case RadioInboundEvent::Kind::DataFrame:
        kind = "data";
        break;
      case RadioInboundEvent::Kind::Unknown:
        kind = "unknown";
        break;
    }

    char message[61]{};
    snprintf(message, sizeof(message), "rx kind=%s len=%u", kind, static_cast<unsigned>(event.length));
    lora_message::DiagnosticPacketPayload payload{};
    if (!lora_message::buildDiagnosticPayload(1, static_cast<uint16_t>(event.kind), message, payload)) {
      continue;
    }

    NativePacket packet{};
    if (lora_message::makeDiagnosticPacket(payload, kLocalNode, kBroadcastNode,
                                           latestTelemetry.sampleIndex, static_cast<uint32_t>(nowMs), packet) &&
        !messageStore.push(packet)) {
      auditLog.push(AuditSeverity::Warning, AuditEventCode::QueueOverflow, kLocalNode,
                    static_cast<uint32_t>(nowMs), "inbound message overflow");
    }
  }
}

void applyInputAction(const InputService::Action &action, unsigned long nowMs) {
  if (action.type == InputService::ActionType::None) {
    return;
  }

  switch (action.type) {
    case InputService::ActionType::NavigateNext:
      displayService.cycleGuiSurface(1, "encoder_next", nowMs);
      break;
    case InputService::ActionType::NavigatePrevious:
      displayService.cycleGuiSurface(-1, "encoder_prev", nowMs);
      break;
    case InputService::ActionType::Select:
      displayService.setGuiSurface(DisplayService::GuiSurface::Messages, "encoder_select", nowMs);
      displayService.setGuiMessage("input", "select");
      break;
    case InputService::ActionType::Back:
      displayService.setGuiSurface(DisplayService::GuiSurface::Telemetry, "back_button", nowMs);
      break;
    case InputService::ActionType::Home:
      displayService.setGuiSurface(DisplayService::GuiSurface::Status, "home_button", nowMs);
      break;
    case InputService::ActionType::ToggleRadioEnable:
      displayService.setGuiSurface(DisplayService::GuiSurface::Config, "radio_toggle", nowMs);
      displayService.setGuiMessage("radio_toggle", inputService.isRadioEnabled() ? "enabled" : "disabled");
      break;
    case InputService::ActionType::ToggleProfileMode:
      displayService.setGuiSurface(DisplayService::GuiSurface::AppProfiles, "profile_toggle", nowMs);
      displayService.setGuiMessage("profile_toggle", inputService.isLowPowerProfile() ? "low_power" : "normal");
      break;
    case InputService::ActionType::None:
      break;
  }
}

void printGuiSurfaceSnapshot() {
  const auto &gui = displayService.guiModel();
  LORA_DEVICE_CONSOLE.print(F("surface "));
  LORA_DEVICE_CONSOLE.print(displayService.guiSurfaceName(gui.activeSurface));
  LORA_DEVICE_CONSOLE.print(F(" prev="));
  LORA_DEVICE_CONSOLE.print(displayService.guiSurfaceName(gui.previousSurface));
  LORA_DEVICE_CONSOLE.print(F(" transitions="));
  LORA_DEVICE_CONSOLE.print(gui.transitionCount);
  LORA_DEVICE_CONSOLE.print(F(" nav="));
  LORA_DEVICE_CONSOLE.print(gui.lastNavigationDelta);
  LORA_DEVICE_CONSOLE.print(F(" reason=\""));
  LORA_DEVICE_CONSOLE.print(gui.lastTransitionReason);
  LORA_DEVICE_CONSOLE.print(F("\""));
  LORA_DEVICE_CONSOLE.println();
}

void renderActiveDisplaySurface(const TelemetryModel &telemetry, const InputService::State &inputs) {
  switch (displayService.guiModel().activeSurface) {
    case DisplayService::GuiSurface::Telemetry:
      displayService.showTelemetry(telemetry, inputs);
      break;
    case DisplayService::GuiSurface::Status:
      displayService.showStatus(telemetry, radioService.isOnline());
      break;
    case DisplayService::GuiSurface::Messages:
      printMessagesSurface();
      break;
    case DisplayService::GuiSurface::Config:
      printConfigStatus();
      break;
    case DisplayService::GuiSurface::AppProfiles:
      printAppProfiles();
      break;
    case DisplayService::GuiSurface::Boot:
      displayService.showBootMessage(telemetry);
      break;
    default:
      displayService.showTelemetry(telemetry, inputs);
      break;
  }
}

void printUnknownCommand(const UsbCommandFrame &frame) {
  LORA_DEVICE_CONSOLE.print(F("unknown command"));
  if (frame.raw[0] != '\0') {
    LORA_DEVICE_CONSOLE.print(F(" ("));
    LORA_DEVICE_CONSOLE.print(frame.raw);
    LORA_DEVICE_CONSOLE.print(F(")"));
  }
  LORA_DEVICE_CONSOLE.println();
  printHelp();
}

void handleCommand(const UsbCommandFrame &frame) {
  switch (frame.command) {
    case UsbCommand::Help:
      printHelp();
      break;
    case UsbCommand::Status:
      displayService.setGuiSurface(DisplayService::GuiSurface::Status, "status_cmd", millis());
      printStatusLine(latestTelemetry, inputService.snapshot());
      printGuiSurfaceSnapshot();
      break;
    case UsbCommand::Telemetry:
      displayService.setGuiSurface(DisplayService::GuiSurface::Telemetry, "telemetry_cmd", millis());
      printTelemetrySnapshot();
      break;
    case UsbCommand::Messages:
      displayService.setGuiSurface(DisplayService::GuiSurface::Messages, "messages_cmd", millis());
      printMessagesSurface();
      break;
    case UsbCommand::Pins:
      printPins();
      break;
    case UsbCommand::Display:
      displayService.setGuiSurface(DisplayService::GuiSurface::Status, "display_cmd", millis());
      printDisplayStatus();
      break;
    case UsbCommand::Radio:
      displayService.setGuiSurface(DisplayService::GuiSurface::Config, "radio_cmd", millis());
      printRadioStatus();
      break;
    case UsbCommand::Modem:
      displayService.setGuiSurface(DisplayService::GuiSurface::Config, "modem_cmd", millis());
      printModemStatus();
      break;
    case UsbCommand::ModemStatus:
      displayService.setGuiSurface(DisplayService::GuiSurface::Config, "modem_status_cmd", millis());
      printModemStatus();
      break;
    case UsbCommand::ModemContract:
      displayService.setGuiSurface(DisplayService::GuiSurface::Config, "modem_contract_cmd", millis());
      printModemContract();
      break;
    case UsbCommand::RadioAt: {
      LORA_DEVICE_CONSOLE.println(F("radio at probe deferred: requires hardware bring-up; no AT write queued"));
      break;
    }
    case UsbCommand::RadioHelp:
    case UsbCommand::RadioConfig: {
      (void)kRadioConfigAtCommand;
      LORA_DEVICE_CONSOLE.println(F("radio AT query deferred: requires hardware bring-up; no AT write queued"));
      break;
    }
    case UsbCommand::Config:
      displayService.setGuiSurface(DisplayService::GuiSurface::Config, "config_cmd", millis());
      printConfigStatus();
      break;
    case UsbCommand::ConfigSignoff:
      displayService.setGuiSurface(DisplayService::GuiSurface::Config, "config_signoff_cmd", millis());
      configService.requestSignoffPlaceholder();
      configService.printSignoffAcknowledgement(LORA_DEVICE_CONSOLE);
      break;
    case UsbCommand::Sensors:
      printSensorStatus();
      break;
    case UsbCommand::I2cScan:
      runI2cScan();
      break;
    case UsbCommand::Version:
      printFirmwareVersion();
      break;
    case UsbCommand::Apps:
      displayService.setGuiSurface(DisplayService::GuiSurface::AppProfiles, "apps_cmd", millis());
      printAppProfiles();
      break;
    case UsbCommand::Profile:
      displayService.setGuiSurface(DisplayService::GuiSurface::AppProfiles, "profile_cmd", millis());
      printAppProfiles();
      break;
    case UsbCommand::App:
      displayService.setGuiSurface(DisplayService::GuiSurface::AppProfiles, "app_cmd", millis());
      if (frame.hasModeArgument) {
        const AppProfile &profile = appProfileRegistry.profileFor(frame.requestedMode);
        printSelectedAppProfile(profile);
        applyModeFromProfile(profile);
      } else {
        printAppProfiles();
      }
      break;
    case UsbCommand::ProfileExport:
      displayService.setGuiSurface(DisplayService::GuiSurface::AppProfiles, "profile_export_cmd", millis());
      printProfileExport(frame);
      break;
    case UsbCommand::ProfileImport:
      displayService.setGuiSurface(DisplayService::GuiSurface::AppProfiles, "profile_import_cmd", millis());
      printProfileImport(frame);
      break;
    case UsbCommand::Mode:
      displayService.setGuiSurface(DisplayService::GuiSurface::Status, "mode_cmd", millis());
      if (frame.hasModeArgument) {
        handleProfileSelection(frame.requestedMode);
      } else {
        LORA_DEVICE_CONSOLE.print(F("current mode: "));
        LORA_DEVICE_CONSOLE.println(appModeName(activeMode));
        LORA_DEVICE_CONSOLE.println(F("Usage: mode [standalone|meshtastic|meshcore|lorawan|usb-host-interface]"));
      }
      break;
    case UsbCommand::Unknown:
      printUnknownCommand(frame);
      break;
    case UsbCommand::None:
      break;
  }
}
}  // namespace

void setup() {
  LORA_DEVICE_CONSOLE.begin(lora_device::kSerialBaudRate);
  if (kSerialWaitMs > 0) {
    delay(kSerialWaitMs);
  }
  printBootBanner();

  sensorService.begin();
  inputService.begin();
  displayService.begin();
  radioService.begin(activeMode);
  commandService.begin();

  displayService.showBootMessage(latestTelemetry);
}

void loop() {
  const unsigned long now = millis();
  inputService.poll(now);
  applyInputAction(inputService.consumeAction(), now);

  UsbCommandFrame frame;
  if (commandService.poll(frame)) {
    handleCommand(frame);
  }

  if (now - lastTelemetryMs >= lora_device::kTelemetryIntervalMs) {
    lastTelemetryMs = now;
    latestTelemetry = sensorService.sampleNow(activeMode, now);
    recordTelemetryPacket(latestTelemetry);
    printTelemetrySnapshot();
    latestTelemetry.printCompactStatus(LORA_DEVICE_CONSOLE);
    LORA_DEVICE_CONSOLE.print(F(" radio="));
    LORA_DEVICE_CONSOLE.println(radioService.status().available ? F("available") : F("stub"));
  }

  if (now - lastStatusMs >= lora_device::kStatusIntervalMs) {
    lastStatusMs = now;
    printStatusLine(latestTelemetry, inputService.snapshot());
  }

  if (now - lastRadioMs >= lora_device::kRadioIntervalMs) {
    lastRadioMs = now;
    if (!radioService.sendTelemetry(latestTelemetry, configService.status(), appProfileRegistry.profileFor(activeMode))) {
      auditLog.push(AuditSeverity::Warning, AuditEventCode::ConfigBlocked, kLocalNode,
                    static_cast<uint32_t>(now), radioService.status().lastTxDenyReason);
    }
  }

  if (now - lastDisplayMs >= lora_device::kDisplayIntervalMs) {
    lastDisplayMs = now;
    renderActiveDisplaySurface(latestTelemetry, inputService.snapshot());
  }

  if (latestTelemetry.uptimeMs == 0) {
    latestTelemetry.uptimeMs = now;
  } else if (latestTelemetry.mode != activeMode) {
    latestTelemetry.mode = activeMode;
  }

  radioService.tick();
  recordRadioInboundEvents(now);
}
