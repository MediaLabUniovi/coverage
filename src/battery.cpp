#include "battery.h"
#include <Wire.h>

#define XPOWERS_CHIP_AXP2101
#include "XPowersLib.h"

#ifndef CONFIG_PMU_SDA
#define CONFIG_PMU_SDA 15
#endif
#ifndef CONFIG_PMU_SCL
#define CONFIG_PMU_SCL 7
#endif

static XPowersPMU power;
static bool pmu_ok = false;

bool batteryBegin() {
  // inicia PMU
  bool result = power.begin(Wire, AXP2101_SLAVE_ADDRESS, CONFIG_PMU_SDA, CONFIG_PMU_SCL);
  if (!result) {
    pmu_ok = false;
    return false;
  }

  // habilitar ADCs necesarios
  power.enableBattDetection();
  power.enableBattVoltageMeasure();

  pmu_ok = true;
  return true;
}

float readBatteryVoltage() {
  if (!pmu_ok) return 0.0f;
  // devuelve mV -> V
  return power.getBattVoltage() / 1000.0f;
}

int readBatteryPercent() {
  if (!pmu_ok) return 0;
  if (!power.isBatteryConnect()) return 0;
  return (int)power.getBatteryPercent();
}