#include "radio_service.h"
#include "device_config.h"
#include <cstring>

namespace {
constexpr unsigned long kAtProbeTimeoutMs = 300;
constexpr unsigned long kCommandModeGuardMs = 1100;
constexpr size_t kAtCommandMaxLen = 32;
constexpr size_t kLastTxDenyReasonBytes = sizeof(RadioStatus::lastTxDenyReason);

constexpr char kAtProbeCommand[] = "AT\r\n";

#if defined(LORA_RADIO_DX_LR03_SERIAL) && !defined(RENODE_SIMULATION)
HardwareSerial dxLr03Serial(lora_device::PinConfig::kRadioUartRx,
                            lora_device::PinConfig::kRadioUartTx);

constexpr char kAtCommandModeSequence[] = "+++";
constexpr size_t kAtCommandModeSequenceLength = sizeof(kAtCommandModeSequence) - 1;
#endif

void copyPrintableAtCommand(char *destination, size_t destinationSize, const char *source) {
  if (destinationSize == 0) {
    return;
  }
  destination[0] = '\0';
  if (source == nullptr) {
    return;
  }

  size_t index = 0;
  while (source[index] != '\0' && source[index] != '\r' && source[index] != '\n' &&
         index < destinationSize - 1) {
    destination[index] = source[index];
    ++index;
  }
  destination[index] = '\0';
}

void clearReasonBuffer(char *reason, size_t reasonLength) {
  if (reasonLength > 0) {
    reason[0] = '\0';
  }
}
}  // namespace


RadioService::RadioService(bool hardwareAvailable_) : hardwareAvailable(hardwareAvailable_) {
  clearReasonBuffer(radioStatus.lastTxDenyReason, sizeof(radioStatus.lastTxDenyReason));
  inboundInbox.clear();
  inboundClassifier.reset();
}

void RadioService::begin(AppMode mode) {
  configurePins();
  radioStatus.initialized = true;
  radioStatus.available = false;
  radioStatus.mocked = true;
  radioStatus.activeMode = mode;
  radioStatus.lastActivityMs = millis();
  radioStatus.transmitGuarded = true;
  radioStatus.lastTxDenyReason[0] = '\0';
  inboundInbox.clear();
  inboundClassifier.reset();
#if defined(LORA_RADIO_DX_LR03_SERIAL)
  radioStatus.dxLr03CompileEnabled = true;
#endif
  beginHardware();

  LORA_DEVICE_CONSOLE.print(F("radio init mode="));
  LORA_DEVICE_CONSOLE.print(appModeName(mode));
  if (radioStatus.serialInitialized) {
    LORA_DEVICE_CONSOLE.println(F(" dx-lr03 serial module initialized; TX guarded"));
  } else {
    LORA_DEVICE_CONSOLE.println(F(" diagnostic/stub mode; no RF transmit path enabled"));
  }
}

void RadioService::configurePins() {
  configureRadioPins();
  radioStatus.pinsConfigured = true;
}

void RadioService::configureRadioPins() {
  digitalWrite(lora_device::PinConfig::kRadioSleepControl, HIGH);
  digitalWrite(lora_device::PinConfig::kRadioModeControl, LOW);
  pinMode(lora_device::PinConfig::kRadioSleepControl, OUTPUT);
  pinMode(lora_device::PinConfig::kRadioModeControl, OUTPUT);
  pinMode(lora_device::PinConfig::kRadioAux, INPUT_PULLDOWN);
}

void RadioService::beginHardware() {
#if defined(LORA_RADIO_DX_LR03_SERIAL) && !defined(RENODE_SIMULATION)
  if (!hardwareAvailable) {
    LORA_DEVICE_CONSOLE.println(F("radio: DX-LR03 serial compile-time path enabled; hardware availability flag is off"));
    clearAtProbeBuffers();
    return;
  }

  radioStatus.moduleInitAttempted = true;
  beginSerialModule();
  radioStatus.available = radioStatus.moduleProbePassed;
  radioStatus.mocked = !radioStatus.moduleProbePassed;

  if (radioStatus.serialInitialized) {
    LORA_DEVICE_CONSOLE.println(F("radio: DX-LR03 UART ready; module probe pending; TX guarded"));
  } else {
    LORA_DEVICE_CONSOLE.println(F("radio: DX-LR03 UART init not available"));
  }
#else
  LORA_DEVICE_CONSOLE.println(F("radio: DX-LR03 compile-time path not enabled"));
#endif
}

void RadioService::beginSerialModule() {
#if defined(LORA_RADIO_DX_LR03_SERIAL) && !defined(RENODE_SIMULATION)
  dxLr03Serial.begin(lora_device::PinConfig::kDxLr03BaudRate);
  radioStatus.serialBaud = lora_device::PinConfig::kDxLr03BaudRate;
  radioStatus.serialInitialized = true;
  radioStatus.moduleProbePassed = false;
  radioStatus.atProbeRequested = false;
  radioStatus.atProbeAttempted = false;
  radioStatus.atProbePassed = false;
  atProbeAffectsOnlineState = false;
  atProbeCommand[0] = '\0';
  radioStatus.lastAtProbeMs = 0;
  radioStatus.lastAtResponse[0] = '\0';
  radioStatus.lastAtError[0] = '\0';
  radioStatus.lastAtCommand[0] = '\0';
  radioStatus.lastAtData[0] = '\0';
  radioStatus.atProbeBytesWritten = 0;
  radioStatus.atProbeBytesRead = 0;
  atProbeAwaitingResponse = false;
  atProbeStage = AtProbeStage::Idle;
  sampleAux();
#else
  radioStatus.serialBaud = 0;
  radioStatus.serialInitialized = false;
  radioStatus.moduleProbePassed = false;
  radioStatus.atProbeRequested = false;
  radioStatus.atProbeAttempted = false;
  radioStatus.atProbePassed = false;
  atProbeAffectsOnlineState = false;
  atProbeCommand[0] = '\0';
  radioStatus.lastAtProbeMs = 0;
  radioStatus.lastAtResponse[0] = '\0';
  radioStatus.lastAtError[0] = '\0';
  radioStatus.lastAtCommand[0] = '\0';
  radioStatus.lastAtData[0] = '\0';
  radioStatus.atProbeBytesWritten = 0;
  radioStatus.atProbeBytesRead = 0;
#endif
}

void RadioService::sampleAux() {
#if defined(LORA_RADIO_DX_LR03_SERIAL) && !defined(RENODE_SIMULATION)
  if (!radioStatus.serialInitialized) {
    radioStatus.auxBusy = false;
    radioStatus.isBusy = false;
    return;
  }
  radioStatus.auxBusy = digitalRead(lora_device::PinConfig::kRadioAux) == HIGH;
  radioStatus.isBusy = radioStatus.auxBusy;
#else
  radioStatus.auxBusy = false;
  radioStatus.isBusy = false;
#endif
}

void RadioService::setMode(AppMode mode) {
  radioStatus.activeMode = mode;
  LORA_DEVICE_CONSOLE.print(F("radio mode -> "));
  LORA_DEVICE_CONSOLE.println(appModeName(mode));
}

void RadioService::tick() {
  if (!radioStatus.initialized) {
    return;
  }

  sampleAux();
  processIncomingSerial();

  const unsigned long now = millis();
  if (radioStatus.atProbeRequested) {
    startAtProbe(now);
  }

  processInboundEvents();

  if (radioStatus.atProbeAttempted && !radioStatus.atProbePassed) {
#if defined(LORA_RADIO_DX_LR03_SERIAL) && !defined(RENODE_SIMULATION)
    if (atProbeStage == AtProbeStage::WaitBeforeAt &&
        (now - radioStatus.lastAtProbeMs >= kCommandModeGuardMs)) {
      const size_t commandLength = strnlen(atProbeCommand, kAtCommandMaxLen);
      const size_t written = dxLr03Serial.write(atProbeCommand, commandLength);
      radioStatus.atProbeBytesWritten += static_cast<uint32_t>(written);
      radioStatus.lastAtProbeMs = now;
      atProbeStage = AtProbeStage::WaitForResponse;
      atProbeAwaitingResponse = true;
      if (written != commandLength) {
        completeAtProbeWithResult(false, nullptr, "at command write incomplete", atProbeAffectsOnlineState);
      }
    }
#endif

    if (atProbeStage == AtProbeStage::WaitForResponse && atProbeAwaitingResponse &&
        (now - radioStatus.lastAtProbeMs >= kAtProbeTimeoutMs)) {
      completeAtProbeWithResult(false, radioStatus.lastAtResponse, "at command timeout", atProbeAffectsOnlineState);
    }
  }

  if (radioStatus.mocked && (now - lastTransmitMs > 60000UL)) {
    LORA_DEVICE_CONSOLE.print(F("radio stub heartbeat mode="));
    LORA_DEVICE_CONSOLE.print(appModeName(radioStatus.activeMode));
    LORA_DEVICE_CONSOLE.println();
    lastTransmitMs = now;
  }
}

void RadioService::printStatus(Print &out) const {
  out.print(F("radio initialized="));
  out.print(radioStatus.initialized ? F("yes") : F("no"));
  out.print(F(" available="));
  out.print(radioStatus.available ? F("yes") : F("no"));
  out.print(F(" mocked="));
  out.print(radioStatus.mocked ? F("yes") : F("no"));
  out.print(F(" pinsConfigured="));
  out.print(radioStatus.pinsConfigured ? F("yes") : F("no"));
  out.print(F(" dxLr03CompileEnabled="));
  out.print(radioStatus.dxLr03CompileEnabled ? F("yes") : F("no"));
  out.print(F(" serialInitialized="));
  out.print(radioStatus.serialInitialized ? F("yes") : F("no"));
  out.print(F(" moduleInitAttempted="));
  out.print(radioStatus.moduleInitAttempted ? F("yes") : F("no"));
  out.print(F(" moduleProbePassed="));
  out.print(radioStatus.moduleProbePassed ? F("yes") : F("no"));
  out.print(F(" atProbeRequested="));
  out.print(radioStatus.atProbeRequested ? F("yes") : F("no"));
  out.print(F(" atProbeAttempted="));
  out.print(radioStatus.atProbeAttempted ? F("yes") : F("no"));
  out.print(F(" atProbePassed="));
  out.print(radioStatus.atProbePassed ? F("yes") : F("no"));
  out.print(F(" atCommand=\""));
  out.print(radioStatus.lastAtCommand);
  out.print(F("\""));
  out.print(F(" lastAtProbeMs="));
  out.print(radioStatus.lastAtProbeMs);
  out.print(F(" atProbeResponse=\""));
  out.print(radioStatus.lastAtResponse);
  out.print(F("\" atProbeError=\""));
  out.print(radioStatus.lastAtError);
  out.print(F("\" atData=\""));
  out.print(radioStatus.lastAtData);
  out.print(F("\" txDenyReason=\""));
  out.print(radioStatus.lastTxDenyReason);
  out.print(F("\" rxQueue="));
  out.print(inboundInbox.count());
  out.print(F(" atProbeBytesRx="));
  out.print(radioStatus.atProbeBytesRead);
  out.print(F(" atProbeBytesTx="));
  out.print(radioStatus.atProbeBytesWritten);
  out.print(F(" serialBaud="));
  out.print(radioStatus.serialBaud);
  out.print(F(" auxBusy="));
  out.print(radioStatus.auxBusy ? F("yes") : F("no"));
  out.print(F(" transmitGuarded="));
  out.print(radioStatus.transmitGuarded ? F("yes") : F("no"));
  out.print(F(" mode="));
  out.print(appModeName(radioStatus.activeMode));
  out.print(F(" txPackets="));
  out.print(radioStatus.txPackets);
  out.print(F(" lastSequence="));
  out.print(radioStatus.lastSequence);
  out.print(F(" lastRssi="));
  out.print(radioStatus.lastRssi);
  out.print(F(" lastActivityMs="));
  out.print(radioStatus.lastActivityMs);
  out.print(F(" pins uart_tx="));
  out.print(lora_device::PinConfig::kRadioUartTx);
  out.print(F(" uart_rx="));
  out.print(lora_device::PinConfig::kRadioUartRx);
  out.print(F(" aux="));
  out.print(lora_device::PinConfig::kRadioAux);
  out.print(F(" m1="));
  out.print(lora_device::PinConfig::kRadioSleepControl);
  out.print(F(" m0="));
  out.print(lora_device::PinConfig::kRadioModeControl);
  out.println();
}

bool RadioService::sendTelemetry(const TelemetryModel &telemetry,
                                const ConfigService::ConfigStatus &configStatus,
                                const AppProfile &selectedProfile) {
  if (!radioStatus.initialized) {
    setLastTxDenyReason("radio send ignored: not initialized");
    LORA_DEVICE_CONSOLE.println(F("radio send ignored; not initialized"));
    return false;
  }

  const RadioTxGateResult gate = txGate.evaluate(radioStatus, configStatus, selectedProfile);
  if (!gate.allowed) {
    setLastTxDenyReason(gate.denyReason != nullptr ? gate.denyReason : "radio tx blocked");
    LORA_DEVICE_CONSOLE.print(F("radio tx blocked by policy: "));
    LORA_DEVICE_CONSOLE.println(radioStatus.lastTxDenyReason);
    return false;
  }
  setLastTxDenyReason(nullptr);

  if (radioStatus.mocked) {
    radioStatus.txPackets++;
    radioStatus.lastSequence = telemetry.sampleIndex;
    radioStatus.lastActivityMs = millis();
    lastTransmitMs = radioStatus.lastActivityMs;
    radioStatus.lastRssi = -127;

    LORA_DEVICE_CONSOLE.print(F("radio(stub) simulated_tx seq="));
    LORA_DEVICE_CONSOLE.print(telemetry.sampleIndex);
    LORA_DEVICE_CONSOLE.print(F(" mode="));
    LORA_DEVICE_CONSOLE.print(appModeName(telemetry.mode));
    LORA_DEVICE_CONSOLE.print(F(" uV="));
    LORA_DEVICE_CONSOLE.print(telemetry.batteryVoltage, 2);
    LORA_DEVICE_CONSOLE.println();
    return true;
  }

  setLastTxDenyReason("radio tx blocked: firmware keeps RF transmit disabled");
  LORA_DEVICE_CONSOLE.print(F("radio dx-lr03 guarded tx blocked seq="));
  LORA_DEVICE_CONSOLE.print(telemetry.sampleIndex);
  LORA_DEVICE_CONSOLE.print(F(" mode="));
  LORA_DEVICE_CONSOLE.println(appModeName(telemetry.mode));
  return false;
}

bool RadioService::requestAtProbe() {
  return requestAtCommand(kAtProbeCommand, true);
}

bool RadioService::requestAtQuery(const char *atCommand) {
  return requestAtCommand(atCommand, false);
}

bool RadioService::pollInboundEvent(RadioInboundEvent &event) {
  return inboundInbox.dequeue(event);
}

bool RadioService::injectInboundPayload(const char *payload, uint8_t length) {
  if (payload == nullptr || length == 0) {
    return false;
  }
  return inboundInbox.injectPayload(payload, length);
}

bool RadioService::requestAtCommand(const char *atCommand, bool affectsOnlineState) {
#if defined(LORA_RADIO_DX_LR03_SERIAL) && !defined(RENODE_SIMULATION)
  if (radioStatus.atProbeRequested || atProbeAwaitingResponse ||
      atProbeStage != AtProbeStage::Idle) {
    setAtError("at command busy");
    return false;
  }

  if (atCommand == nullptr || atCommand[0] == '\0') {
    completeAtProbeWithResult(false, nullptr, "at command invalid: empty command", affectsOnlineState);
    return false;
  }

  if (!hardwareAvailable || !radioStatus.serialInitialized) {
    completeAtProbeWithResult(false, nullptr,
                              !hardwareAvailable ? "at command unavailable: hardware flag off"
                                                : "at command unavailable: serial not initialized",
                              affectsOnlineState);
    return false;
  }

  const size_t commandLength = strnlen(atCommand, kAtCommandMaxLen);
  if (commandLength == 0) {
    completeAtProbeWithResult(false, nullptr, "at command invalid: empty command", affectsOnlineState);
    return false;
  }

  if (commandLength >= sizeof(atProbeCommand)) {
    completeAtProbeWithResult(false, nullptr, "at command too long", affectsOnlineState);
    return false;
  }

  radioStatus.atProbeRequested = true;
  radioStatus.atProbeAttempted = false;
  radioStatus.atProbePassed = false;
  atProbeAffectsOnlineState = affectsOnlineState;
  if (affectsOnlineState) {
    radioStatus.moduleProbePassed = false;
    radioStatus.available = false;
    radioStatus.mocked = true;
  }
  strncpy(atProbeCommand, atCommand, sizeof(atProbeCommand) - 1);
  atProbeCommand[sizeof(atProbeCommand) - 1] = '\0';
  copyPrintableAtCommand(radioStatus.lastAtCommand, sizeof(radioStatus.lastAtCommand), atCommand);
  radioStatus.lastAtResponse[0] = '\0';
  radioStatus.lastAtError[0] = '\0';
  radioStatus.lastAtData[0] = '\0';
  radioStatus.atProbeBytesWritten = 0;
  radioStatus.atProbeBytesRead = 0;
  radioStatus.lastAtProbeMs = 0;
  atProbeAwaitingResponse = false;
  atProbeStage = AtProbeStage::Idle;
  clearAtProbeBuffers();
  return true;
#else
  (void)affectsOnlineState;
  (void)atCommand;
  completeAtProbeWithResult(false, nullptr, "at command unavailable in this build", affectsOnlineState);
  return false;
#endif
}

void RadioService::processIncomingSerial() {
#if defined(LORA_RADIO_DX_LR03_SERIAL) && !defined(RENODE_SIMULATION)
  if (!radioStatus.serialInitialized) {
    return;
  }
  while (dxLr03Serial.available() > 0) {
    const int raw = dxLr03Serial.read();
    if (raw < 0) {
      return;
    }
    ++radioStatus.atProbeBytesRead;
    inboundClassifier.ingestByte(static_cast<uint8_t>(raw), inboundInbox);
    radioStatus.lastActivityMs = millis();
  }
#else
  return;
#endif
}

void RadioService::processInboundEvents() {
  if (!atProbeAwaitingResponse) {
    return;
  }

  RadioInboundEvent event{};
  while (inboundInbox.dequeue(event)) {
    if (processAtProbeEvent(event)) {
      return;
    }
  }
}

bool RadioService::processAtProbeEvent(const RadioInboundEvent &event) {
  if (event.length == 0 || event.payload[0] == '\0') {
    return false;
  }

  const bool looksLikeError = (std::strstr(event.payload, "ERROR") != nullptr);
  const bool isOkResponse = (std::strcmp(event.payload, "OK") == 0);
  const bool isAtLine = (event.kind == RadioInboundEvent::Kind::AtResponse);

  if (!isAtLine) {
    if (looksLikeError && atProbeAffectsOnlineState) {
      completeAtProbeWithResult(false, event.payload, "at probe response error", atProbeAffectsOnlineState);
      return true;
    }
    appendAtDataLine(event.payload);
    return false;
  }

  if (isOkResponse) {
    completeAtProbeWithResult(true, event.payload, nullptr, atProbeAffectsOnlineState);
    return true;
  }
  if (looksLikeError) {
    completeAtProbeWithResult(false, event.payload,
                              atProbeAffectsOnlineState ? "at probe response error" : "at query response error",
                              atProbeAffectsOnlineState);
    return true;
  }
  appendAtDataLine(event.payload);
  return false;
}

void RadioService::startAtProbe(unsigned long nowMs) {
#if defined(LORA_RADIO_DX_LR03_SERIAL) && !defined(RENODE_SIMULATION)
  if (!radioStatus.serialInitialized) {
    completeAtProbeWithResult(false,
                             nullptr,
                             atProbeAffectsOnlineState ? "at probe unavailable: serial not initialized"
                                                      : "at command unavailable: serial not initialized",
                             atProbeAffectsOnlineState);
    return;
  }

  if (radioStatus.auxBusy) {
    completeAtProbeWithResult(false,
                             nullptr,
                             atProbeAffectsOnlineState ? "at probe blocked: aux busy"
                                                      : "at command blocked: aux busy",
                             atProbeAffectsOnlineState);
    return;
  }

  while (dxLr03Serial.available() > 0) {
    (void)dxLr03Serial.read();
  }

  const size_t written = dxLr03Serial.write(kAtCommandModeSequence, kAtCommandModeSequenceLength);
  radioStatus.atProbeBytesWritten += static_cast<uint32_t>(written);
  radioStatus.atProbeRequested = false;
  radioStatus.atProbeAttempted = true;
  atProbeAwaitingResponse = false;
  atProbeStage = AtProbeStage::WaitBeforeAt;
  radioStatus.lastAtProbeMs = nowMs;
  clearAtProbeBuffers();
  if (written != kAtCommandModeSequenceLength) {
    completeAtProbeWithResult(false,
                             nullptr,
                             "at command-mode write incomplete",
                             atProbeAffectsOnlineState);
    return;
  }
#else
  (void)nowMs;
  completeAtProbeWithResult(false, nullptr, "at command unavailable in this build", atProbeAffectsOnlineState);
#endif
}

void RadioService::completeAtProbeWithResult(bool passed, const char *response, const char *error, bool affectsOnlineState) {
  atProbeAwaitingResponse = false;
  atProbeStage = AtProbeStage::Idle;

  if (response != nullptr) {
    const size_t responseLen = strlen(response);
    if (responseLen >= sizeof(radioStatus.lastAtResponse)) {
      memcpy(radioStatus.lastAtResponse, response, sizeof(radioStatus.lastAtResponse) - 1);
      radioStatus.lastAtResponse[sizeof(radioStatus.lastAtResponse) - 1] = '\0';
    } else {
      memcpy(radioStatus.lastAtResponse, response, responseLen + 1);
    }
  } else if (error != nullptr) {
    radioStatus.lastAtResponse[0] = '\0';
  }

  if (error != nullptr) {
    const size_t errorLen = strlen(error);
    if (errorLen >= sizeof(radioStatus.lastAtError)) {
      memcpy(radioStatus.lastAtError, error, sizeof(radioStatus.lastAtError) - 1);
      radioStatus.lastAtError[sizeof(radioStatus.lastAtError) - 1] = '\0';
    } else {
      memcpy(radioStatus.lastAtError, error, errorLen + 1);
    }
  } else {
    radioStatus.lastAtError[0] = '\0';
  }

  if (affectsOnlineState) {
    radioStatus.atProbePassed = passed;
    radioStatus.moduleProbePassed = passed;
    radioStatus.available = passed;
    radioStatus.mocked = !passed;
  }
  radioStatus.atProbeAttempted = true;
  radioStatus.atProbeRequested = false;
  atProbeAffectsOnlineState = false;
  clearAtProbeBuffers();
}

void RadioService::setAtError(const char *error) {
  if (error == nullptr) {
    radioStatus.lastAtError[0] = '\0';
    return;
  }

  const size_t errorLen = strlen(error);
  if (errorLen >= sizeof(radioStatus.lastAtError)) {
    memcpy(radioStatus.lastAtError, error, sizeof(radioStatus.lastAtError) - 1);
    radioStatus.lastAtError[sizeof(radioStatus.lastAtError) - 1] = '\0';
  } else {
    memcpy(radioStatus.lastAtError, error, errorLen + 1);
  }
}

void RadioService::appendAtDataLine(const char *line) {
  if (line == nullptr || line[0] == '\0') {
    return;
  }

  size_t used = strlen(radioStatus.lastAtData);
  if (used >= sizeof(radioStatus.lastAtData) - 1) {
    return;
  }

  if (used > 0) {
    radioStatus.lastAtData[used++] = '|';
    radioStatus.lastAtData[used] = '\0';
  }

  for (size_t sourceIndex = 0; line[sourceIndex] != '\0' && used < sizeof(radioStatus.lastAtData) - 1; ++sourceIndex) {
    const char c = line[sourceIndex];
    radioStatus.lastAtData[used++] = (c == '\r' || c == '\n') ? ' ' : c;
  }
  radioStatus.lastAtData[used] = '\0';
}

void RadioService::clearAtProbeBuffers() {
  inboundClassifier.reset();
  radioStatus.lastAtData[0] = '\0';
}

void RadioService::setLastTxDenyReason(const char *reason) {
  if (reason == nullptr) {
    clearReasonBuffer(radioStatus.lastTxDenyReason, kLastTxDenyReasonBytes);
    return;
  }

  size_t reasonLength = strlen(reason);
  if (reasonLength >= sizeof(radioStatus.lastTxDenyReason)) {
    reasonLength = sizeof(radioStatus.lastTxDenyReason) - 1;
  }
  memcpy(radioStatus.lastTxDenyReason, reason, reasonLength);
  radioStatus.lastTxDenyReason[reasonLength] = '\0';
}
