#pragma once

#include <Arduino.h>
#include "device_config.h"
#include "app_modes.h"

class InputService {
 public:
  enum class ActionType {
    None,
    NavigateNext,
    NavigatePrevious,
    Select,
    Back,
    Home,
    ToggleRadioEnable,
    ToggleProfileMode,
  };

  struct Action {
    ActionType type{ActionType::None};
    int32_t navigationDelta{0};
    unsigned long eventMs{0};
  };

  struct InputTransitionCounter {
    uint32_t transitions{0};
    uint32_t pressTransitions{0};
    unsigned long lastChangeMs{0};
  };

  struct State {
    bool encoderA{false};
    bool encoderB{false};
    bool encoderSwitch{false};
    bool backPressed{false};
    bool homePressed{false};
    bool radioEnableToggle{false};
    bool profileToggle{false};
    unsigned long sampleMs{0};
    unsigned long toggleChanges{0};
    unsigned long lastTransitionMs{0};
    uint32_t encoderTransitionCount{0};
    uint32_t encoderInvalidTransitions{0};
    int32_t encoderDelta{0};
    int32_t encoderDetents{0};
    InputTransitionCounter back;
    InputTransitionCounter home;
    InputTransitionCounter encoderAInput;
    InputTransitionCounter encoderBInput;
    InputTransitionCounter encoderSwitchInput;
    InputTransitionCounter radioEnableToggleInput;
    InputTransitionCounter profileToggleInput;
    bool keypadConfigured{false};
    bool keypadScanReady{false};
    uint32_t keypadEvents{0};
    char lastKeypadKey{'\0'};
  };

  void begin();
  void poll(unsigned long nowMs = millis());

  const State &snapshot() const { return state; }
  bool isRadioEnabled() const { return state.radioEnableToggle; }
  bool isLowPowerProfile() const { return state.profileToggle; }

  void printState(Print &out) const;
  Action consumeAction();

 private:
  struct DebouncedPin {
    uint8_t pin{255};
    bool armed{false};
    bool stableValue{false};
    bool pendingValue{false};
    uint8_t sameCount{0};
    bool changedThisPoll{false};
    uint32_t transitionCount{0};
    uint32_t pressCount{0};
    unsigned long lastTransitionMs{0};
  };

  bool configured{false};
  uint32_t tickCounter{0};
  State state;
  DebouncedPin encoderA;
  DebouncedPin encoderB;
  DebouncedPin encoderSwitch;
  DebouncedPin backButton;
  DebouncedPin homeButton;
  DebouncedPin radioEnable;
  DebouncedPin profileToggle;

  static bool readRawActiveLow(uint8_t pin);
  void configurePin(DebouncedPin &pin, uint8_t pinNumber);
  bool pollPin(DebouncedPin &pin, unsigned long nowMs);
  int8_t decodeEncoderDelta(uint8_t previousState, uint8_t currentState) const;
  static uint8_t encodeEncoderState(bool encoderA, bool encoderB);
  void queueAction(ActionType type, int32_t navigationDelta, unsigned long nowMs);
  void queueAction(const Action &action, unsigned long nowMs);

  bool encoderStateInitialized{false};
  uint8_t previousEncoderState{0};
  Action pendingAction{};
};
