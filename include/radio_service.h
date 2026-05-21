#pragma once

#include <Arduino.h>
#include "telemetry_model.h"
#include "app_modes.h"
#include "config_service.h"
#include "radio_receive_classifier.h"
#include "radio_receive_inbox.h"
#include "radio_tx_gate.h"

struct AppProfile;

struct RadioStatus {
  bool initialized{false};
  bool available{false};
  bool mocked{true};
  bool pinsConfigured{false};
  bool dxLr03CompileEnabled{false};
  bool serialInitialized{false};
  bool moduleInitAttempted{false};
  bool moduleProbePassed{false};
  bool atProbeRequested{false};
  bool atProbeAttempted{false};
  bool atProbePassed{false};
  uint32_t serialBaud{0};
  unsigned long lastAtProbeMs{0};
  char lastAtResponse[32]{};
  char lastAtError[32]{};
  char lastAtCommand[24]{};
  char lastAtData[96]{};
  uint32_t atProbeBytesWritten{0};
  uint32_t atProbeBytesRead{0};
  bool auxBusy{false};
  bool transmitGuarded{true};
  bool isBusy{false};
  uint32_t txPackets{0};
  uint32_t lastSequence{0};
  int lastRssi{0};
  unsigned long lastActivityMs{0};
  AppMode activeMode{AppMode::Standalone};
  char lastTxDenyReason[48]{};
};

class RadioService {
 public:
  explicit RadioService(bool hardwareAvailable = false);

  void begin(AppMode mode);
  void setMode(AppMode mode);
  void tick();
  bool sendTelemetry(const TelemetryModel &telemetry, const ConfigService::ConfigStatus &configStatus,
                    const AppProfile &selectedProfile);
  bool requestAtProbe();
  bool requestAtQuery(const char *atCommand);
  bool pollInboundEvent(RadioInboundEvent &event);
  bool injectInboundPayload(const char *payload, uint8_t length);
  void printStatus(Print &out) const;
  const RadioStatus &status() const { return radioStatus; }
  bool isOnline() const { return radioStatus.initialized && radioStatus.available; }

 private:
  bool requestAtCommand(const char *atCommand, bool affectsOnlineState);
  void configurePins();
  void beginHardware();
  void configureRadioPins();
  void beginSerialModule();
  void sampleAux();
  void startAtProbe(unsigned long nowMs);
  void completeAtProbeWithResult(bool passed, const char *response, const char *error = nullptr, bool affectsOnlineState = false);
  void setAtError(const char *error);
  void appendAtDataLine(const char *line);
  void clearAtProbeBuffers();
  void processIncomingSerial();
  void processInboundEvents();
  bool processAtProbeEvent(const RadioInboundEvent &event);
  void setLastTxDenyReason(const char *reason);

  enum class AtProbeStage : uint8_t {
    Idle,
    WaitBeforeAt,
    WaitForResponse,
  };

  bool hardwareAvailable{false};
  RadioStatus radioStatus;
  unsigned long lastTransmitMs{0};
  RadioReceiveInbox inboundInbox;
  RadioReceiveClassifier inboundClassifier;
  RadioTxGate txGate;
  bool atProbeAwaitingResponse{false};
  AtProbeStage atProbeStage{AtProbeStage::Idle};
  bool atProbeAffectsOnlineState{false};
  char atProbeCommand[32]{};
};
