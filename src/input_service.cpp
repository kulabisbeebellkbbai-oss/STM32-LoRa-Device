#include "input_service.h"

namespace {
bool isPressTransition(bool transitionChanged, bool stableValue) {
  return transitionChanged && stableValue;
}
}

void InputService::configurePin(DebouncedPin &entry, uint8_t pinNumber) {
  entry.pin = pinNumber;
  entry.armed = true;
  pinMode(pinNumber, INPUT_PULLUP);
  entry.pendingValue = false;
  entry.stableValue = false;
  entry.sameCount = 0;
  entry.changedThisPoll = false;
}

bool InputService::readRawActiveLow(uint8_t pin) {
  return digitalRead(pin) == LOW;
}

bool InputService::pollPin(DebouncedPin &entry, unsigned long nowMs) {
  if (!entry.armed) {
    return false;
  }

  const bool raw = readRawActiveLow(entry.pin);
  entry.changedThisPoll = false;

  if (raw == entry.pendingValue) {
    if (entry.sameCount < 255) {
      entry.sameCount++;
    }
  } else {
    entry.pendingValue = raw;
    entry.sameCount = 1;
  }

  if (entry.sameCount >= lora_device::kInputDebounceThreshold &&
      raw != entry.stableValue) {
    entry.stableValue = raw;
    entry.changedThisPoll = true;
    entry.transitionCount++;
    if (raw) {
      entry.pressCount++;
    }
    entry.sameCount = 0;
    entry.lastTransitionMs = nowMs;
  }

  return entry.stableValue;
}

uint8_t InputService::encodeEncoderState(bool encoderAValue, bool encoderBValue) {
  return static_cast<uint8_t>((encoderAValue ? 0x02 : 0x00) | (encoderBValue ? 0x01 : 0x00));
}

int8_t InputService::decodeEncoderDelta(uint8_t previousState, uint8_t currentState) const {
  switch ((previousState << 2) | currentState) {
    case 0x01:  // 00 -> 01
      return 1;
    case 0x07:  // 01 -> 11
      return 1;
    case 0x0E:  // 11 -> 10
      return 1;
    case 0x08:  // 10 -> 00
      return 1;
    case 0x02:  // 00 -> 10
      return -1;
    case 0x04:  // 01 -> 00
      return -1;
    case 0x0B:  // 10 -> 11
      return -1;
    case 0x0D:  // 11 -> 01
      return -1;
    default:
      return 0;
  }
}

void InputService::queueAction(const Action &action, unsigned long nowMs) {
  if (action.type == ActionType::None) {
    return;
  }

  const Action current = pendingAction;
  Action merged = action;
  if (current.type == ActionType::None) {
    merged.eventMs = nowMs;
    pendingAction = merged;
    return;
  }

  if ((current.type == ActionType::NavigateNext || current.type == ActionType::NavigatePrevious) &&
      (action.type == ActionType::NavigateNext || action.type == ActionType::NavigatePrevious)) {
    const int32_t mergedDelta = current.navigationDelta + action.navigationDelta;
    merged.type = (mergedDelta >= 0) ? ActionType::NavigateNext : ActionType::NavigatePrevious;
    merged.navigationDelta = mergedDelta;
    merged.eventMs = nowMs;
    pendingAction = merged;
    return;
  }

  pendingAction = current;
}

void InputService::queueAction(ActionType type, int32_t navigationDelta, unsigned long nowMs) {
  queueAction(Action{type, navigationDelta, nowMs}, nowMs);
}

void InputService::begin() {
  if (configured) {
    return;
  }

  configurePin(encoderA, lora_device::PinConfig::kEncoderA);
  configurePin(encoderB, lora_device::PinConfig::kEncoderB);
  configurePin(encoderSwitch, lora_device::PinConfig::kEncoderSwitch);
  configurePin(backButton, lora_device::PinConfig::kBackButton);
  configurePin(homeButton, lora_device::PinConfig::kHomeButton);
  configurePin(radioEnable, lora_device::PinConfig::kRadioEnableToggle);
  configurePin(profileToggle, lora_device::PinConfig::kProfileToggle);
  configured = true;
}

void InputService::poll(unsigned long nowMs) {
  if (!configured) {
    return;
  }

  tickCounter++;

  const bool encA = pollPin(encoderA, nowMs);
  const bool encB = pollPin(encoderB, nowMs);
  const bool encSw = pollPin(encoderSwitch, nowMs);
  const bool back = pollPin(backButton, nowMs);
  const bool home = pollPin(homeButton, nowMs);
  const bool radio = pollPin(radioEnable, nowMs);
  const bool profile = pollPin(profileToggle, nowMs);

  state.encoderA = encA;
  state.encoderB = encB;
  state.encoderSwitch = encSw;
  state.backPressed = back;
  state.homePressed = home;
  state.radioEnableToggle = radio;
  state.profileToggle = profile;
  state.sampleMs = nowMs;
  const bool encoderTransition = encoderA.changedThisPoll || encoderB.changedThisPoll;
  const bool anyTransition =
      encoderTransition || encoderSwitch.changedThisPoll || backButton.changedThisPoll ||
      homeButton.changedThisPoll || radioEnable.changedThisPoll ||
      profileToggle.changedThisPoll;
  const uint8_t transitionsThisPoll =
      (encoderA.changedThisPoll ? 1 : 0) + (encoderB.changedThisPoll ? 1 : 0) +
      (encoderSwitch.changedThisPoll ? 1 : 0) + (backButton.changedThisPoll ? 1 : 0) +
      (homeButton.changedThisPoll ? 1 : 0) + (radioEnable.changedThisPoll ? 1 : 0) +
      (profileToggle.changedThisPoll ? 1 : 0);
  state.back = {backButton.transitionCount, backButton.pressCount,
               backButton.lastTransitionMs};
  state.home = {homeButton.transitionCount, homeButton.pressCount,
               homeButton.lastTransitionMs};
  state.encoderAInput = {encoderA.transitionCount, encoderA.pressCount,
                        encoderA.lastTransitionMs};
  state.encoderBInput = {encoderB.transitionCount, encoderB.pressCount,
                        encoderB.lastTransitionMs};
  state.encoderSwitchInput = {encoderSwitch.transitionCount, encoderSwitch.pressCount,
                             encoderSwitch.lastTransitionMs};
  state.radioEnableToggleInput = {radioEnable.transitionCount, radioEnable.pressCount,
                                 radioEnable.lastTransitionMs};
  state.profileToggleInput = {profileToggle.transitionCount, profileToggle.pressCount,
                             profileToggle.lastTransitionMs};

  const uint8_t currentEncoderState = encodeEncoderState(encA, encB);
  if (!encoderStateInitialized) {
    previousEncoderState = currentEncoderState;
    encoderStateInitialized = true;
  }

  int8_t encoderStep = 0;
  if (encoderTransition) {
    state.encoderTransitionCount++;
    encoderStep = decodeEncoderDelta(previousEncoderState, currentEncoderState);
    if (encoderStep == 0) {
      state.encoderInvalidTransitions++;
    } else {
      state.encoderDelta += encoderStep;
      state.encoderDetents = state.encoderDelta / 4;
    }

    previousEncoderState = currentEncoderState;
  }

  if (anyTransition) {
    state.lastTransitionMs = nowMs;
  }

  if (anyTransition) {
    state.toggleChanges += transitionsThisPoll;
  }

  pendingAction = {ActionType::None, 0, nowMs};
  if (isPressTransition(backButton.changedThisPoll, state.backPressed)) {
    queueAction(ActionType::Back, 0, nowMs);
  }
  if (isPressTransition(homeButton.changedThisPoll, state.homePressed)) {
    queueAction(ActionType::Home, 0, nowMs);
  }
  if (isPressTransition(encoderSwitch.changedThisPoll, state.encoderSwitch)) {
    queueAction(ActionType::Select, 0, nowMs);
  }
  if (radioEnable.changedThisPoll) {
    queueAction(ActionType::ToggleRadioEnable, 0, nowMs);
  }
  if (profileToggle.changedThisPoll) {
    queueAction(ActionType::ToggleProfileMode, 0, nowMs);
  }
  if (encoderTransition) {
    if (encoderStep > 0) {
      queueAction(ActionType::NavigateNext, 1, nowMs);
    } else if (encoderStep < 0) {
      queueAction(ActionType::NavigatePrevious, -1, nowMs);
    }
  }
}

InputService::Action InputService::consumeAction() {
  const Action action = pendingAction;
  pendingAction = {ActionType::None, 0, 0};
  return action;
}

void InputService::printState(Print &out) const {
  out.print(F("input encoderA="));
  out.print(state.encoderA ? F("on") : F("off"));
  out.print(F(" encoderB="));
  out.print(state.encoderB ? F("on") : F("off"));
  out.print(F(" enc_sw="));
  out.print(state.encoderSwitch ? F("on") : F("off"));
  out.print(F(" back="));
  out.print(state.backPressed ? F("pressed") : F("released"));
  out.print(F(" home="));
  out.print(state.homePressed ? F("pressed") : F("released"));
  out.print(F(" radio_en="));
  out.print(state.radioEnableToggle ? F("on") : F("off"));
  out.print(F(" profile="));
  out.print(state.profileToggle ? F("low-power") : F("normal"));
  out.print(F(" changes="));
  out.print(state.toggleChanges);
  out.print(F(" back_t="));
  out.print(state.back.transitions);
  out.print(F("/"));
  out.print(state.back.pressTransitions);
  out.print(F(" home_t="));
  out.print(state.home.transitions);
  out.print(F("/"));
  out.print(state.home.pressTransitions);
  out.print(F(" encA_t="));
  out.print(state.encoderAInput.transitions);
  out.print(F("/"));
  out.print(state.encoderAInput.pressTransitions);
  out.print(F(" encB_t="));
  out.print(state.encoderBInput.transitions);
  out.print(F("/"));
  out.print(state.encoderBInput.pressTransitions);
  out.print(F(" enc_sw_t="));
  out.print(state.encoderSwitchInput.transitions);
  out.print(F("/"));
  out.print(state.encoderSwitchInput.pressTransitions);
  out.print(F(" radio_t="));
  out.print(state.radioEnableToggleInput.transitions);
  out.print(F("/"));
  out.print(state.radioEnableToggleInput.pressTransitions);
  out.print(F(" profile_t="));
  out.print(state.profileToggleInput.transitions);
  out.print(F("/"));
  out.print(state.profileToggleInput.pressTransitions);
  out.print(F(" enc_trans="));
  out.print(state.encoderTransitionCount);
  out.print(F(" enc_invalid="));
  out.print(state.encoderInvalidTransitions);
  out.print(F(" enc_delta="));
  out.print(state.encoderDelta);
  out.print(F(" enc_detents="));
  out.print(state.encoderDetents);
  out.print(F(" keypad_configured="));
  out.print(state.keypadConfigured ? F("yes") : F("no"));
  out.print(F(" keypad_ready="));
  out.print(state.keypadScanReady ? F("yes") : F("no"));
  out.print(F(" keypad_events="));
  out.print(state.keypadEvents);
  out.print(F(" last_key="));
  if (state.lastKeypadKey == '\0') {
    out.print(F("none"));
  } else {
    out.print(state.lastKeypadKey);
  }
  out.print(F(" last_transition_ms="));
  out.print(state.lastTransitionMs);
}
