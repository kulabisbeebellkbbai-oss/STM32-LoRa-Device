#include "display_service.h"

#include "device_config.h"

#include <cstring>
#include <SPI.h>

namespace {
constexpr uint8_t kSt7789CommandSoftwareReset = 0x01;
constexpr uint8_t kSt7789CommandSleepOut = 0x11;
constexpr uint8_t kSt7789CommandDisplayFunctionControl = 0xB6;
constexpr uint8_t kSt7789CommandColumnAddressSet = 0x2A;
constexpr uint8_t kSt7789CommandPageAddressSet = 0x2B;
constexpr uint8_t kSt7789CommandMemoryWrite = 0x2C;
constexpr uint8_t kSt7789CommandPixelFormat = 0x3A;
constexpr uint8_t kSt7789CommandDisplayOn = 0x29;

constexpr uint8_t kSt7789PixelFormatRgb565 = 0x55;
constexpr uint16_t kDisplayWidth = 240;
constexpr uint16_t kDisplayHeight = 280;
constexpr uint16_t kBootFrameColor = 0x07E0;  // Green
constexpr uint16_t kColorBlack = 0x0000;
constexpr uint16_t kColorWhite = 0xFFFF;
constexpr uint16_t kColorGreen = 0x07E0;
constexpr uint16_t kColorBlue = 0x001F;
constexpr uint16_t kColorRed = 0xF800;
constexpr uint16_t kColorOrange = 0xFD20;
constexpr uint16_t kColorCyan = 0x07FF;
constexpr uint16_t kColorMagenta = 0xF81F;
constexpr uint16_t kColorYellow = 0xFFE0;
constexpr uint16_t kColorGray = 0x7BEF;
constexpr uint16_t kColorTeal = 0x0410;
constexpr uint16_t kColorNavy = 0x000D;
constexpr uint16_t kColorDarkGreen = 0x03E0;
constexpr uint16_t kColorMaroon = 0x7800;
constexpr uint16_t kColorBurgundy = 0x9001;
constexpr uint16_t kColorDarkBlue = 0x0010;
constexpr const char *kGuiSurfaceNames[] = {"boot", "telemetry", "status", "messages", "config", "profiles"};

constexpr uint16_t kCardMarginX = 2;
constexpr uint16_t kCardGap = 2;
constexpr uint16_t kCardWidth = 117;
constexpr uint16_t kHeaderBandY0 = 0;
constexpr uint16_t kHeaderBandY1 = 23;
constexpr uint16_t kTelemetryRow1Y0 = 24;
constexpr uint16_t kTelemetryRow1Y1 = 99;
constexpr uint16_t kTelemetryRow2Y0 = 101;
constexpr uint16_t kTelemetryRow2Y1 = 176;
constexpr uint16_t kIndicatorY0 = 178;
constexpr uint16_t kIndicatorY1 = 227;
constexpr uint16_t kFooterY0 = 229;
constexpr uint16_t kFooterY1 = 279;

constexpr DisplayService::Region kHeaderBand{0, kHeaderBandY0, kDisplayWidth - 1, kHeaderBandY1};
constexpr DisplayService::Region kHeaderStatusSubLeft{0, kHeaderBandY0, 119, 23};
constexpr DisplayService::Region kHeaderStatusSubRight{120, kHeaderBandY0, kDisplayWidth - 1, 23};
constexpr DisplayService::Region kTelemetryCard1{kCardMarginX, kTelemetryRow1Y0,
                                                kCardMarginX + kCardWidth - 1, kTelemetryRow1Y1};
constexpr DisplayService::Region kTelemetryCard2{kCardMarginX + kCardWidth + kCardGap, kTelemetryRow1Y0,
                                                kCardMarginX + 2U * kCardWidth + kCardGap - 1, kTelemetryRow1Y1};
constexpr DisplayService::Region kTelemetryCard3{kCardMarginX, kTelemetryRow2Y0,
                                                kCardMarginX + kCardWidth - 1, kTelemetryRow2Y1};
constexpr DisplayService::Region kTelemetryCard4{kCardMarginX + kCardWidth + kCardGap, kTelemetryRow2Y0,
                                                kCardMarginX + 2U * kCardWidth + kCardGap - 1, kTelemetryRow2Y1};
constexpr DisplayService::Region kIndicatorInput{0, kIndicatorY0, 119, kIndicatorY1};
constexpr DisplayService::Region kIndicatorRadio{120, kIndicatorY0, kDisplayWidth - 1, kIndicatorY1};
constexpr DisplayService::Region kModeFooter{kCardMarginX, kFooterY0, kDisplayWidth - kCardMarginX - 1, kFooterY1};

uint32_t fold16(const uint32_t value) {
  return ((value << 11) ^ value) ^ (value >> 3);
}
constexpr uint8_t kDisplayResetPulseMs = 8;
constexpr uint16_t kDisplayInitDelayMs = 120;

constexpr uint8_t kGuiSurfaceCount = sizeof(kGuiSurfaceNames) / sizeof(kGuiSurfaceNames[0]);
}  // namespace

DisplayService::DisplayService(bool stubOnly) : diagnostics() {
  diagnostics.stubOnly = stubOnly;
  diagnostics.renderStub = true;
#if defined(RENODE_SIMULATION)
  diagnostics.renodeSimulation = true;
#endif
#if defined(LORA_DISPLAY_ST7789)
  diagnostics.st7789CompileEnabled = true;
#endif
  guiModel_.activeSurface = GuiSurface::Boot;
  guiModel_.previousSurface = GuiSurface::Boot;
  guiModel_.lastTransitionMs = 0;
  guiModel_.lastNavigationDelta = 0;
  guiModel_.messageSequence = 0;
  guiModel_.transitionCount = 0;
  guiModel_.lastTransitionReason[0] = '\0';
  guiModel_.messageTag[0] = '\0';
}

void DisplayService::begin() {
  diagnostics.enabled = true;
  diagnostics.lastRenderMs = 0;
  diagnostics.lastView = "none";
  diagnostics.st7789RenderRequests = 0;
  diagnostics.st7789RenderSkips = 0;
  lastInputs = InputService::State();
  configureDisplayPins();
  beginHardware();
  setGuiSurface(GuiSurface::Boot, "boot", millis());
}

const char *DisplayService::guiSurfaceName(GuiSurface surface) {
  const uint8_t index = static_cast<uint8_t>(surface);
  if (index >= kGuiSurfaceCount) {
    return "telemetry";
  }
  return kGuiSurfaceNames[index];
}

void DisplayService::setGuiSurface(GuiSurface surface, const char *reason, unsigned long nowMs) {
  if (guiModel_.activeSurface != surface) {
    guiModel_.previousSurface = guiModel_.activeSurface;
    guiModel_.activeSurface = surface;
    guiModel_.transitionCount++;
    guiModel_.lastTransitionMs = nowMs;
    guiModel_.lastNavigationDelta = 0;
  }
  if (reason != nullptr) {
    std::strncpy(guiModel_.lastTransitionReason, reason, sizeof(guiModel_.lastTransitionReason) - 1);
    guiModel_.lastTransitionReason[sizeof(guiModel_.lastTransitionReason) - 1] = '\0';
  } else {
    guiModel_.lastTransitionReason[0] = '\0';
  }
}

void DisplayService::cycleGuiSurface(int8_t navigationDelta, const char *reason, unsigned long nowMs) {
  if (navigationDelta == 0) {
    return;
  }

  const int32_t current = static_cast<int32_t>(guiModel_.activeSurface);
  const int32_t count = static_cast<int32_t>(kGuiSurfaceCount);
  const int32_t normalizedDelta = navigationDelta > 0 ? 1 : -1;
  int32_t next = current + normalizedDelta;
  if (next < 0) {
    next = count - 1;
  } else if (next >= count) {
    next = 0;
  }
  guiModel_.lastNavigationDelta = static_cast<int16_t>(normalizedDelta);
  setGuiSurface(static_cast<GuiSurface>(next), reason, nowMs);
}

void DisplayService::setGuiMessage(const char *tag, const char *message) {
  ++guiModel_.messageSequence;
  if (tag != nullptr && *tag != '\0') {
    std::strncpy(guiModel_.messageTag, tag, sizeof(guiModel_.messageTag) - 1);
    guiModel_.messageTag[sizeof(guiModel_.messageTag) - 1] = '\0';
  } else {
    guiModel_.messageTag[0] = '\0';
  }
  (void)message;
}

void DisplayService::configureDisplayPins() {
  // Latch safe levels before enabling outputs so first hardware bring-up does
  // not pulse chip-select, reset, or backlight lines.
  digitalWrite(lora_device::PinConfig::kDisplayCs, HIGH);
  digitalWrite(lora_device::PinConfig::kDisplayDc, LOW);
  digitalWrite(lora_device::PinConfig::kDisplayReset, HIGH);
  digitalWrite(lora_device::PinConfig::kDisplayBacklight, LOW);
#if defined(LORA_DISPLAY_ST7789)
  digitalWrite(lora_device::PinConfig::kDisplaySpiSck, LOW);
  digitalWrite(lora_device::PinConfig::kDisplaySpiMosi, LOW);
#endif

  pinMode(lora_device::PinConfig::kDisplayCs, OUTPUT);
  pinMode(lora_device::PinConfig::kDisplayDc, OUTPUT);
  pinMode(lora_device::PinConfig::kDisplayReset, OUTPUT);
  pinMode(lora_device::PinConfig::kDisplayBacklight, OUTPUT);
#if defined(LORA_DISPLAY_ST7789)
  pinMode(lora_device::PinConfig::kDisplaySpiSck, OUTPUT);
  pinMode(lora_device::PinConfig::kDisplaySpiMosi, OUTPUT);
#endif

  digitalWrite(lora_device::PinConfig::kDisplayCs, HIGH);
  digitalWrite(lora_device::PinConfig::kDisplayDc, LOW);
  digitalWrite(lora_device::PinConfig::kDisplayReset, HIGH);
  digitalWrite(lora_device::PinConfig::kDisplayBacklight, LOW);
#if defined(LORA_DISPLAY_ST7789)
  digitalWrite(lora_device::PinConfig::kDisplaySpiSck, LOW);
  digitalWrite(lora_device::PinConfig::kDisplaySpiMosi, LOW);
#endif

  diagnostics.pinsConfigured = true;
}

void DisplayService::beginHardware() {
#if defined(LORA_DISPLAY_ST7789)
  if (!isSt7789PathActive()) {
    if (diagnostics.stubOnly) {
      LORA_DEVICE_CONSOLE.println(F("DISPLAY: LORA_DISPLAY_ST7789 compile-time path active, but display service is stubOnly"));
    } else if (diagnostics.renodeSimulation) {
      LORA_DEVICE_CONSOLE.println(F("DISPLAY: LORA_DISPLAY_ST7789 compile-time path active in Renode simulation (safe, no SPI writes)"));
    }
    return;
  }

  SPI.begin();
  diagnostics.st7789SpiInitialized = true;

  st7789Reset();
  st7789WriteCommand(kSt7789CommandSoftwareReset);
  delay(kDisplayResetPulseMs);
  st7789WriteCommand(kSt7789CommandSleepOut);
  st7789WriteCommand(kSt7789CommandPixelFormat);
  st7789WriteData(kSt7789PixelFormatRgb565);
  st7789WriteCommand(kSt7789CommandDisplayFunctionControl);
  const uint8_t functionControl[] = {0x0A, 0x82, 0x27, 0x00};
  st7789WriteDataBuffer(functionControl, sizeof(functionControl));
  st7789WriteCommand(kSt7789CommandDisplayOn);
  delay(kDisplayInitDelayMs);
  digitalWrite(lora_device::PinConfig::kDisplayBacklight, HIGH);

  diagnostics.st7789Initialized = true;
  diagnostics.renderStub = false;
  LORA_DEVICE_CONSOLE.println(F("DISPLAY: ST7789 scaffold initialized (command-level, non-driver)"));
#else
  LORA_DEVICE_CONSOLE.println(F("DISPLAY: ST7789 compile-time path not enabled"));
#endif
}

bool DisplayService::isSt7789PathActive() const {
#if defined(LORA_DISPLAY_ST7789)
  return diagnostics.st7789CompileEnabled && diagnostics.pinsConfigured && !diagnostics.stubOnly && !diagnostics.renodeSimulation;
#else
  return false;
#endif
}

const __FlashStringHelper *DisplayService::renderPathLabel() const {
  return diagnostics.renderStub ? F("STUB") : F("ST7789");
}

void DisplayService::recordFrame(const char *view) {
  diagnostics.frameCounter++;
  diagnostics.lastRenderMs = millis();
  diagnostics.lastView = view;
}

DisplayService::DashboardState DisplayService::buildDashboardState(const TelemetryModel &telemetry,
                                                                  const InputService::State &inputs,
                                                                  bool radioOnline) const {
  DashboardState state;
  state.headerColor = headerStatusColor(telemetry, radioOnline);
  state.telemetryBatteryColor = telemetryBatteryColor(telemetry.batteryPercent, telemetry.sensorsValid);
  state.telemetryTemperatureColor = telemetryTemperatureColor(telemetry.ambientTemperatureC, telemetry.sensorsValid);
  state.telemetryPressureColor = telemetryPressureColor(telemetry.pressureHPa, telemetry.sensorsValid);
  state.telemetryGpsColor = telemetryGpsColor(telemetry.gpsFix, telemetry.gpsSatellites, telemetry.sensorsValid);
  state.inputIndicatorColor = inputIndicatorColor(inputs);
  state.radioIndicatorColor = radioIndicatorColor(radioOnline, inputs.radioEnableToggle);
  state.footerColor = modeFooterColor(telemetry.mode);
  return state;
}

uint32_t DisplayService::stateHash(const DashboardState &state) const {
  uint32_t digest = 0x811c9dc5UL;
  const uint16_t fields[] = {state.headerColor, state.telemetryBatteryColor, state.telemetryTemperatureColor,
                            state.telemetryPressureColor, state.telemetryGpsColor, state.inputIndicatorColor,
                            state.radioIndicatorColor, state.footerColor};
  for (uint8_t idx = 0; idx < 8; ++idx) {
    digest ^= static_cast<uint32_t>(fields[idx]);
    digest = (digest * 16777619UL) ^ fold16(digest);
  }
  return digest;
}

void DisplayService::renderDashboard(const DashboardState &state, const TelemetryModel &telemetry) {
  const uint32_t nextHash = stateHash(state);
  diagnostics.st7789RenderRequests++;

  if (!isSt7789PathActive() || !diagnostics.st7789Initialized) {
    diagnostics.st7789RenderSkips++;
    return;
  }

  if (diagnostics.st7789DashboardFrames > 0 && nextHash == diagnostics.st7789StateHash) {
    diagnostics.st7789RenderSkips++;
    return;
  }

  diagnostics.st7789DisplaySequence++;
  diagnostics.st7789DashboardFrames++;

  renderHeaderStatusBand(state);
  renderTelemetryCards(state, telemetry);
  renderIndicators(state);
  renderModeFooter(state);
  diagnostics.st7789StateHash = nextHash;
}

void DisplayService::renderHeaderStatusBand(const DashboardState &state) {
  st7789RenderBlock(kHeaderBand, state.headerColor);
  st7789RenderBlock(kHeaderStatusSubLeft, state.headerColor);
  st7789RenderBlock(kHeaderStatusSubRight, state.footerColor);
  diagnostics.st7789HeaderFrames++;
  diagnostics.st7789RegionRenders += 3U;
  diagnostics.st7789TotalPixels +=
      (kHeaderBand.x1 - kHeaderBand.x0 + 1) * (kHeaderBand.y1 - kHeaderBand.y0 + 1) +
      (kHeaderStatusSubLeft.x1 - kHeaderStatusSubLeft.x0 + 1) * (kHeaderStatusSubLeft.y1 - kHeaderStatusSubLeft.y0 + 1) +
      (kHeaderStatusSubRight.x1 - kHeaderStatusSubRight.x0 + 1) * (kHeaderStatusSubRight.y1 - kHeaderStatusSubRight.y0 + 1);
}

void DisplayService::renderTelemetryCards(const DashboardState &state, const TelemetryModel &telemetry) {
  (void)telemetry;
  st7789RenderBlock(kTelemetryCard1, state.telemetryBatteryColor);
  st7789RenderBlock(kTelemetryCard2, state.telemetryTemperatureColor);
  st7789RenderBlock(kTelemetryCard3, state.telemetryPressureColor);
  st7789RenderBlock(kTelemetryCard4, state.telemetryGpsColor);
  diagnostics.st7789TelemetryFrames += 4U;
  diagnostics.st7789RegionRenders += 4U;
  diagnostics.st7789TotalPixels +=
      (kTelemetryCard1.x1 - kTelemetryCard1.x0 + 1) * (kTelemetryCard1.y1 - kTelemetryCard1.y0 + 1) +
      (kTelemetryCard2.x1 - kTelemetryCard2.x0 + 1) * (kTelemetryCard2.y1 - kTelemetryCard2.y0 + 1) +
      (kTelemetryCard3.x1 - kTelemetryCard3.x0 + 1) * (kTelemetryCard3.y1 - kTelemetryCard3.y0 + 1) +
      (kTelemetryCard4.x1 - kTelemetryCard4.x0 + 1) * (kTelemetryCard4.y1 - kTelemetryCard4.y0 + 1);
}

void DisplayService::renderIndicators(const DashboardState &state) {
  st7789RenderBlock(kIndicatorInput, state.inputIndicatorColor);
  st7789RenderBlock(kIndicatorRadio, state.radioIndicatorColor);
  diagnostics.st7789IndicatorFrames += 2U;
  diagnostics.st7789RegionRenders += 2U;
  diagnostics.st7789TotalPixels +=
      (kIndicatorInput.x1 - kIndicatorInput.x0 + 1) * (kIndicatorInput.y1 - kIndicatorInput.y0 + 1) +
      (kIndicatorRadio.x1 - kIndicatorRadio.x0 + 1) * (kIndicatorRadio.y1 - kIndicatorRadio.y0 + 1);
}

void DisplayService::renderModeFooter(const DashboardState &state) {
  st7789RenderBlock(kModeFooter, state.footerColor);
  diagnostics.st7789FooterFrames++;
  diagnostics.st7789RegionRenders += 1U;
  diagnostics.st7789TotalPixels += (kModeFooter.x1 - kModeFooter.x0 + 1) * (kModeFooter.y1 - kModeFooter.y0 + 1);
  diagnostics.lastFooterColor = state.footerColor;
}

void DisplayService::emitBootFrame(const TelemetryModel &telemetry) {
  if (!isSt7789PathActive() || !diagnostics.st7789Initialized) {
    return;
  }

  renderDashboard(buildDashboardState(telemetry, lastInputs, diagnostics.lastRadioOnline), telemetry);
  diagnostics.st7789BootFrames++;
}

void DisplayService::emitStatusFrame(const TelemetryModel &telemetry, bool radioOnline) {
  if (!isSt7789PathActive() || !diagnostics.st7789Initialized) {
    return;
  }

  diagnostics.lastRadioOnline = radioOnline;
  renderDashboard(buildDashboardState(telemetry, lastInputs, radioOnline), telemetry);
  diagnostics.st7789StatusFrames++;
}

uint16_t DisplayService::telemetryBatteryColor(float batteryPercent, bool sensorsValid) const {
  if (!sensorsValid) {
    return kColorDarkBlue;
  }

  if (batteryPercent >= 75.0f) {
    return kColorGreen;
  }
  if (batteryPercent >= 40.0f) {
    return kColorOrange;
  }
  if (batteryPercent >= 20.0f) {
    return kColorYellow;
  }
  return kColorRed;
}

uint16_t DisplayService::telemetryTemperatureColor(float temperatureC, bool sensorsValid) const {
  if (!sensorsValid) {
    return kColorDarkBlue;
  }

  if (temperatureC < 0.0f) {
    return kColorBlue;
  }
  if (temperatureC < 10.0f) {
    return kColorCyan;
  }
  if (temperatureC < 25.0f) {
    return kColorGreen;
  }
  if (temperatureC < 35.0f) {
    return kColorOrange;
  }
  return kColorRed;
}

uint16_t DisplayService::telemetryPressureColor(float pressureHPa, bool sensorsValid) const {
  if (!sensorsValid) {
    return kColorDarkBlue;
  }

  if (pressureHPa >= 990.0f && pressureHPa <= 1030.0f) {
    return kColorGreen;
  }
  if (pressureHPa >= 960.0f && pressureHPa <= 1050.0f) {
    return kColorYellow;
  }
  return kColorMagenta;
}

uint16_t DisplayService::telemetryGpsColor(bool gpsFix, int satellites, bool sensorsValid) const {
  if (!sensorsValid) {
    return kColorDarkBlue;
  }
  if (!gpsFix) {
    return kColorBurgundy;
  }
  if (satellites >= 8) {
    return kColorGreen;
  }
  if (satellites >= 4) {
    return kColorYellow;
  }
  return kColorOrange;
}

uint16_t DisplayService::headerStatusColor(const TelemetryModel &telemetry, bool radioOnline) const {
  if (!telemetry.sensorsValid) {
    return kColorGray;
  }
  if (!radioOnline) {
    return kColorDarkGreen;
  }
  if (telemetry.mode == AppMode::UsbHostInterface) {
    return kColorTeal;
  }
  return kColorGreen;
}

uint16_t DisplayService::inputIndicatorColor(const InputService::State &inputs) const {
  uint8_t state = 0;
  if (inputs.backPressed) {
    state |= 1;
  }
  if (inputs.homePressed) {
    state |= 2;
  }
  if (inputs.radioEnableToggle) {
    state |= 4;
  }
  if (inputs.profileToggle) {
    state |= 8;
  }
  if (inputs.encoderA) {
    state |= 16;
  }
  if (inputs.encoderB) {
    state |= 32;
  }
  if (inputs.encoderSwitch) {
    state |= 64;
  }

  constexpr uint16_t kInputColors[3] = {kColorNavy, kColorBlue, kColorTeal};
  return kInputColors[state % 3];
}

uint16_t DisplayService::radioIndicatorColor(bool radioOnline, bool radioEnabled) const {
  if (!radioOnline) {
    return kColorRed;
  }
  if (!radioEnabled) {
    return kColorOrange;
  }
  return kColorCyan;
}

uint16_t DisplayService::modeFooterColor(AppMode mode) const {
  switch (mode) {
    case AppMode::Standalone:
      return kColorBlue;
    case AppMode::Meshtastic:
      return kColorGreen;
    case AppMode::MeshCore:
      return kColorOrange;
    case AppMode::LoRaWAN:
      return kColorMagenta;
    case AppMode::UsbHostInterface:
      return kColorYellow;
    default:
      return kColorGray;
  }
}

void DisplayService::st7789Reset() {
  digitalWrite(lora_device::PinConfig::kDisplayReset, LOW);
  delay(kDisplayResetPulseMs);
  digitalWrite(lora_device::PinConfig::kDisplayReset, HIGH);
  delay(kDisplayResetPulseMs);
}

void DisplayService::st7789BeginTransaction() {
#if defined(LORA_DISPLAY_ST7789)
  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
  digitalWrite(lora_device::PinConfig::kDisplayCs, LOW);
#endif
}

void DisplayService::st7789EndTransaction() {
#if defined(LORA_DISPLAY_ST7789)
  digitalWrite(lora_device::PinConfig::kDisplayCs, HIGH);
  SPI.endTransaction();
#endif
}

void DisplayService::st7789WriteCommand(uint8_t command) {
#if defined(LORA_DISPLAY_ST7789)
  if (!isSt7789PathActive() || !diagnostics.st7789SpiInitialized) {
    return;
  }

  st7789BeginTransaction();
  digitalWrite(lora_device::PinConfig::kDisplayDc, LOW);
  SPI.transfer(command);
  diagnostics.st7789Commands++;
  diagnostics.st7789SpiWrites++;
  st7789EndTransaction();
#endif
}

void DisplayService::st7789WriteData(uint8_t value) {
  st7789WriteDataBuffer(&value, 1);
}

void DisplayService::st7789WriteDataBuffer(const uint8_t *data, size_t length) {
#if defined(LORA_DISPLAY_ST7789)
  if (!isSt7789PathActive() || !diagnostics.st7789SpiInitialized || length == 0 || data == nullptr) {
    return;
  }

  st7789BeginTransaction();
  digitalWrite(lora_device::PinConfig::kDisplayDc, HIGH);
  for (size_t idx = 0; idx < length; ++idx) {
    SPI.transfer(data[idx]);
    diagnostics.st7789DataBytes++;
    diagnostics.st7789SpiWrites++;
  }
  st7789EndTransaction();
#endif
}

void DisplayService::st7789SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
#if defined(LORA_DISPLAY_ST7789)
  if (!isSt7789PathActive() || !diagnostics.st7789SpiInitialized) {
    return;
  }

  const uint8_t columnBounds[] = {static_cast<uint8_t>(x0 >> 8), static_cast<uint8_t>(x0 & 0xFF),
                                 static_cast<uint8_t>(x1 >> 8), static_cast<uint8_t>(x1 & 0xFF)};
  const uint8_t rowBounds[] = {static_cast<uint8_t>(y0 >> 8), static_cast<uint8_t>(y0 & 0xFF),
                              static_cast<uint8_t>(y1 >> 8), static_cast<uint8_t>(y1 & 0xFF)};

  st7789WriteCommand(kSt7789CommandColumnAddressSet);
  st7789WriteDataBuffer(columnBounds, sizeof(columnBounds));
  st7789WriteCommand(kSt7789CommandPageAddressSet);
  st7789WriteDataBuffer(rowBounds, sizeof(rowBounds));
#endif
}

void DisplayService::st7789RenderBlock(const Region &rect, uint16_t color565) {
#if defined(LORA_DISPLAY_ST7789)
  if (!isSt7789PathActive() || !diagnostics.st7789SpiInitialized || rect.x0 > rect.x1 || rect.y0 > rect.y1) {
    return;
  }

  if (rect.x1 >= kDisplayWidth || rect.y1 >= kDisplayHeight) {
    return;
  }

  const uint32_t pixelCount =
      static_cast<uint32_t>(rect.x1 - rect.x0 + 1) * static_cast<uint32_t>(rect.y1 - rect.y0 + 1);
  st7789SetAddressWindow(rect.x0, rect.y0, rect.x1, rect.y1);
  st7789WriteCommand(kSt7789CommandMemoryWrite);
  st7789WriteRepeatedColor(color565, pixelCount);
#else
  (void)rect;
  (void)color565;
#endif
}

void DisplayService::st7789WriteRepeatedColor(uint16_t color565, uint32_t pixelCount) {
#if defined(LORA_DISPLAY_ST7789)
  if (!isSt7789PathActive() || pixelCount == 0) {
    return;
  }

  const uint8_t colorHigh = static_cast<uint8_t>(color565 >> 8);
  const uint8_t colorLow = static_cast<uint8_t>(color565 & 0xFF);
  diagnostics.st7789DataBytes += 2U * pixelCount;
  st7789BeginTransaction();
  digitalWrite(lora_device::PinConfig::kDisplayDc, HIGH);
  for (uint32_t idx = 0; idx < pixelCount; ++idx) {
    SPI.transfer(colorHigh);
    SPI.transfer(colorLow);
    diagnostics.st7789SpiWrites += 2U;
  }
  st7789EndTransaction();
#else
  (void)color565;
  (void)pixelCount;
#endif
}

void DisplayService::st7789RenderFrame(const uint16_t color565) {
#if defined(LORA_DISPLAY_ST7789)
  st7789WriteCommand(kSt7789CommandMemoryWrite);
  st7789WriteRepeatedColor(color565, 4U);
#endif
}

void DisplayService::printStatus(Print &out) const {
  out.print(F("gui_surface="));
  out.print(guiSurfaceName(guiModel_.activeSurface));
  out.print(F(" gui_prev="));
  out.print(guiSurfaceName(guiModel_.previousSurface));
  out.print(F(" gui_transitions="));
  out.print(guiModel_.transitionCount);
  out.print(F(" gui_nav_delta="));
  out.print(guiModel_.lastNavigationDelta);
  out.print(F(" gui_msg_seq="));
  out.print(guiModel_.messageSequence);
  out.print(F(" gui_last_reason=\""));
  out.print(guiModel_.lastTransitionReason);
  out.print(F("\" gui_msg_tag=\""));
  out.print(guiModel_.messageTag);
  out.print(F("\" "));
  out.print(F("display enabled="));
  out.print(diagnostics.enabled ? F("yes") : F("no"));
  out.print(F(" stubOnly="));
  out.print(diagnostics.stubOnly ? F("yes") : F("no"));
  out.print(F(" renodeSimulation="));
  out.print(diagnostics.renodeSimulation ? F("yes") : F("no"));
  out.print(F(" renderStub="));
  out.print(diagnostics.renderStub ? F("yes") : F("no"));
  out.print(F(" st7789CompileEnabled="));
  out.print(diagnostics.st7789CompileEnabled ? F("yes") : F("no"));
  out.print(F(" st7789Initialized="));
  out.print(diagnostics.st7789Initialized ? F("yes") : F("no"));
  out.print(F(" st7789SpiInitialized="));
  out.print(diagnostics.st7789SpiInitialized ? F("yes") : F("no"));
  out.print(F(" pinsConfigured="));
  out.print(diagnostics.pinsConfigured ? F("yes") : F("no"));
  out.print(F(" pins cs="));
  out.print(lora_device::PinConfig::kDisplayCs);
  out.print(F(" dc="));
  out.print(lora_device::PinConfig::kDisplayDc);
  out.print(F(" rst="));
  out.print(lora_device::PinConfig::kDisplayReset);
  out.print(F(" bl="));
  out.print(lora_device::PinConfig::kDisplayBacklight);
  out.print(F(" geometry="));
  out.print(kDisplayWidth);
  out.print(F("x"));
  out.print(kDisplayHeight);
  out.print(F(" frameCounter="));
  out.print(diagnostics.frameCounter);
  out.print(F(" st7789Commands="));
  out.print(diagnostics.st7789Commands);
  out.print(F(" st7789DataBytes="));
  out.print(diagnostics.st7789DataBytes);
  out.print(F(" st7789SpiWrites="));
  out.print(diagnostics.st7789SpiWrites);
  out.print(F(" st7789BootFrames="));
  out.print(diagnostics.st7789BootFrames);
  out.print(F(" st7789StatusFrames="));
  out.print(diagnostics.st7789StatusFrames);
  out.print(F(" st7789DashboardFrames="));
  out.print(diagnostics.st7789DashboardFrames);
  out.print(F(" st7789HeaderFrames="));
  out.print(diagnostics.st7789HeaderFrames);
  out.print(F(" st7789TelemetryFrames="));
  out.print(diagnostics.st7789TelemetryFrames);
  out.print(F(" st7789IndicatorFrames="));
  out.print(diagnostics.st7789IndicatorFrames);
  out.print(F(" st7789FooterFrames="));
  out.print(diagnostics.st7789FooterFrames);
  out.print(F(" st7789RegionRenders="));
  out.print(diagnostics.st7789RegionRenders);
  out.print(F(" st7789TotalPixels="));
  out.print(diagnostics.st7789TotalPixels);
  out.print(F(" st7789RenderRequests="));
  out.print(diagnostics.st7789RenderRequests);
  out.print(F(" st7789RenderSkips="));
  out.print(diagnostics.st7789RenderSkips);
  out.print(F(" st7789StateHash="));
  out.print(diagnostics.st7789StateHash);
  out.print(F(" st7789DisplaySequence="));
  out.print(diagnostics.st7789DisplaySequence);
  out.print(F(" lastRadioOnline="));
  out.print(diagnostics.lastRadioOnline ? F("yes") : F("no"));
  out.print(F(" lastRenderMs="));
  out.print(diagnostics.lastRenderMs);
  out.print(F(" lastView="));
  out.print(diagnostics.lastView);
  out.println();
}

void DisplayService::printGuiState(Print &out) const {
  out.print(F("gui_surface="));
  out.print(guiSurfaceName(guiModel_.activeSurface));
  out.print(F(" gui_prev="));
  out.print(guiSurfaceName(guiModel_.previousSurface));
  out.print(F(" gui_transitions="));
  out.print(guiModel_.transitionCount);
  out.print(F(" gui_nav_delta="));
  out.print(guiModel_.lastNavigationDelta);
  out.print(F(" gui_msg_seq="));
  out.print(guiModel_.messageSequence);
  out.print(F(" gui_last_reason=\""));
  out.print(guiModel_.lastTransitionReason);
  out.print(F("\" gui_msg_tag=\""));
  out.print(guiModel_.messageTag);
  out.print(F("\" "));
  if (diagnostics.lastView == nullptr) {
    out.print(F(" lastView=none"));
  } else {
    out.print(F(" lastView="));
    out.print(diagnostics.lastView);
  }
  out.println();
}

void DisplayService::showBootMessage(const TelemetryModel &telemetry) {
  if (!diagnostics.enabled) {
    return;
  }

  recordFrame(guiSurfaceName(guiModel_.activeSurface));
  emitBootFrame(telemetry);

  LORA_DEVICE_CONSOLE.print(F("DISPLAY("));
  LORA_DEVICE_CONSOLE.print(renderPathLabel());
  LORA_DEVICE_CONSOLE.print(F("): boot complete | "));
  LORA_DEVICE_CONSOLE.print(F("view="));
  LORA_DEVICE_CONSOLE.print(guiSurfaceName(guiModel_.activeSurface));
  LORA_DEVICE_CONSOLE.print(F(" "));
  telemetry.printCompactStatus(LORA_DEVICE_CONSOLE);
  LORA_DEVICE_CONSOLE.println();
}

void DisplayService::showTelemetry(const TelemetryModel &telemetry, const InputService::State &inputs) {
  if (!diagnostics.enabled) {
    return;
  }

  recordFrame(guiSurfaceName(guiModel_.activeSurface));
  lastInputs = inputs;
  renderDashboard(buildDashboardState(telemetry, inputs, diagnostics.lastRadioOnline), telemetry);
  LORA_DEVICE_CONSOLE.print(F("DISPLAY("));
  LORA_DEVICE_CONSOLE.print(renderPathLabel());
  LORA_DEVICE_CONSOLE.print(F("): "));
  LORA_DEVICE_CONSOLE.print(guiSurfaceName(guiModel_.activeSurface));
  LORA_DEVICE_CONSOLE.print(F(" "));
  telemetry.printTelemetryLine(LORA_DEVICE_CONSOLE);
  LORA_DEVICE_CONSOLE.print(F(" | btn_back="));
  LORA_DEVICE_CONSOLE.print(inputs.backPressed ? F("1") : F("0"));
  LORA_DEVICE_CONSOLE.print(F(" btn_home="));
  LORA_DEVICE_CONSOLE.print(inputs.homePressed ? F("1") : F("0"));
  LORA_DEVICE_CONSOLE.print(F(" radio_enable="));
  LORA_DEVICE_CONSOLE.print(inputs.radioEnableToggle ? F("1") : F("0"));
  LORA_DEVICE_CONSOLE.print(F(" profile="));
  LORA_DEVICE_CONSOLE.print(inputs.profileToggle ? F("1") : F("0"));
  LORA_DEVICE_CONSOLE.println();
}

void DisplayService::showStatus(const TelemetryModel &telemetry, bool radioOnline) {
  if (!diagnostics.enabled) {
    return;
  }

  recordFrame(guiSurfaceName(guiModel_.activeSurface));
  emitStatusFrame(telemetry, radioOnline);

  LORA_DEVICE_CONSOLE.print(F("DISPLAY("));
  LORA_DEVICE_CONSOLE.print(renderPathLabel());
  LORA_DEVICE_CONSOLE.print(F("): "));
  LORA_DEVICE_CONSOLE.print(guiSurfaceName(guiModel_.activeSurface));
  LORA_DEVICE_CONSOLE.print(F(" seq="));
  LORA_DEVICE_CONSOLE.print(telemetry.sampleIndex);
  LORA_DEVICE_CONSOLE.print(F(" radio="));
  LORA_DEVICE_CONSOLE.print(radioOnline ? F("online") : F("offline"));
  LORA_DEVICE_CONSOLE.print(F(" render_stub="));
  LORA_DEVICE_CONSOLE.print(diagnostics.renderStub ? F("yes") : F("no"));
  LORA_DEVICE_CONSOLE.println();
}
