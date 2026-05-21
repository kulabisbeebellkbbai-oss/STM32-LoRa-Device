#include "telemetry_model.h"

void TelemetryModel::printTelemetryLine(Print &out) const {
  out.print(F("telemetry seq="));
  out.print(sampleIndex);
  out.print(F(" mode="));
  out.print(appModeName(mode));
  out.print(F(" uptime_ms="));
  out.print(uptimeMs);
  out.print(F(" temp_c="));
  out.print(ambientTemperatureC, 1);
  out.print(F(" humidity_pct="));
  out.print(ambientHumidityPercent, 1);
  out.print(F(" pressure_hpa="));
  out.print(pressureHPa, 1);
  out.print(F(" batt_v="));
  out.print(batteryVoltage, 2);
  out.print(F(" gps_fix="));
  out.print(gpsFix ? F("yes") : F("no"));
}

void TelemetryModel::printCompactStatus(Print &out) const {
  out.print(F("telemetry sample "));
  out.print(sampleIndex);
  out.print(F(" mode="));
  out.print(appModeName(mode));
  out.print(F(" sensors="));
  out.print(sensorsValid ? F("ok") : F("stub"));
  out.print(F(" lat="));
  out.print(gpsLatitude, 5);
  out.print(F(" lon="));
  out.print(gpsLongitude, 5);
  out.print(F(" batt="));
  out.print(batteryPercent, 1);
  out.print(F("%"));
}
