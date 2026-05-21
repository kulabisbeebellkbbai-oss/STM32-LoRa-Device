#pragma once

#include <Arduino.h>

#include "device_config.h"
#include "telemetry_model.h"
#include "input_service.h"

class DisplayService {
 public:
  enum class GuiSurface : uint8_t {
    Boot,
    Telemetry,
    Status,
    Messages,
    Config,
    AppProfiles,
  };

  struct GuiModel {
    GuiSurface activeSurface{GuiSurface::Boot};
    GuiSurface previousSurface{GuiSurface::Boot};
    uint32_t transitionCount{0};
    int16_t lastNavigationDelta{0};
    unsigned long lastTransitionMs{0};
    uint32_t messageSequence{0};
    char lastTransitionReason[24]{};
    char messageTag[20]{};
  };

  static const char *guiSurfaceName(GuiSurface surface);

  struct Region {
    uint16_t x0;
    uint16_t y0;
    uint16_t x1;
    uint16_t y1;
  };

  struct Diagnostics {
    bool enabled{false};
    bool stubOnly{true};
    bool renodeSimulation{false};
    bool renderStub{true};
    bool st7789CompileEnabled{false};
    bool st7789Initialized{false};
    bool pinsConfigured{false};
    bool st7789SpiInitialized{false};
    uint32_t frameCounter{0};
    uint32_t st7789Commands{0};
    uint32_t st7789DataBytes{0};
    uint32_t st7789SpiWrites{0};
    uint32_t st7789BootFrames{0};
    uint32_t st7789StatusFrames{0};
    uint32_t st7789DashboardFrames{0};
    uint32_t st7789HeaderFrames{0};
    uint32_t st7789TelemetryFrames{0};
    uint32_t st7789IndicatorFrames{0};
    uint32_t st7789FooterFrames{0};
    uint32_t st7789RegionRenders{0};
    uint32_t st7789TotalPixels{0};
    uint32_t st7789RenderRequests{0};
    uint32_t st7789RenderSkips{0};
    uint32_t st7789StateHash{0};
    uint32_t st7789DisplaySequence{0};
    uint16_t lastFooterColor{0};
    bool lastRadioOnline{false};
    unsigned long lastRenderMs{0};
    const char *lastView{"none"};
  };

  explicit DisplayService(bool stubOnly = true);

  void begin();
  void showBootMessage(const TelemetryModel &telemetry);
  void showTelemetry(const TelemetryModel &telemetry, const InputService::State &inputs);
  void showStatus(const TelemetryModel &telemetry, bool radioOnline);
  void setGuiSurface(GuiSurface surface, const char *reason = nullptr, unsigned long nowMs = millis());
  void cycleGuiSurface(int8_t navigationDelta, const char *reason = nullptr, unsigned long nowMs = millis());
  void setGuiMessage(const char *tag, const char *message);
  void printStatus(Print &out) const;
  void printGuiState(Print &out) const;
  bool isEnabled() const { return diagnostics.enabled; }
  const GuiModel &guiModel() const { return guiModel_; }
  const Diagnostics &getDiagnostics() const { return diagnostics; }

 private:
  struct DashboardState {
    uint16_t headerColor{0};
    uint16_t telemetryBatteryColor{0};
    uint16_t telemetryTemperatureColor{0};
    uint16_t telemetryPressureColor{0};
    uint16_t telemetryGpsColor{0};
    uint16_t inputIndicatorColor{0};
    uint16_t radioIndicatorColor{0};
    uint16_t footerColor{0};
  };

  const __FlashStringHelper *renderPathLabel() const;
  void configureDisplayPins();
  void beginHardware();
  void recordFrame(const char *view);
  void emitBootFrame(const TelemetryModel &telemetry);
  void emitStatusFrame(const TelemetryModel &telemetry, bool radioOnline);
  bool isSt7789PathActive() const;
  DashboardState buildDashboardState(const TelemetryModel &telemetry, const InputService::State &inputs, bool radioOnline) const;
  uint32_t stateHash(const DashboardState &state) const;
  void renderDashboard(const DashboardState &state, const TelemetryModel &telemetry);
  void renderHeaderStatusBand(const DashboardState &state);
  void renderTelemetryCards(const DashboardState &state, const TelemetryModel &telemetry);
  void renderIndicators(const DashboardState &state);
  void renderModeFooter(const DashboardState &state);
  void st7789RenderBlock(const Region &rect, uint16_t color565);
  void st7789Reset();
  void st7789BeginTransaction();
  void st7789EndTransaction();
  void st7789WriteCommand(uint8_t command);
  void st7789WriteData(uint8_t value);
  void st7789WriteDataBuffer(const uint8_t *data, size_t length);
  void st7789SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
  void st7789WriteRepeatedColor(uint16_t color565, uint32_t pixelCount);
  void st7789RenderFrame(const uint16_t color565);
  uint16_t telemetryBatteryColor(float batteryPercent, bool sensorsValid) const;
  uint16_t telemetryTemperatureColor(float temperatureC, bool sensorsValid) const;
  uint16_t telemetryPressureColor(float pressureHPa, bool sensorsValid) const;
  uint16_t telemetryGpsColor(bool gpsFix, int satellites, bool sensorsValid) const;
  uint16_t headerStatusColor(const TelemetryModel &telemetry, bool radioOnline) const;
  uint16_t inputIndicatorColor(const InputService::State &inputs) const;
  uint16_t radioIndicatorColor(bool radioOnline, bool radioEnabled) const;
  uint16_t modeFooterColor(AppMode mode) const;

  Diagnostics diagnostics;
  InputService::State lastInputs;
  GuiModel guiModel_;
};
