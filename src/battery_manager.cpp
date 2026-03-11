#include "battery_manager.h"
#include <Wire.h>

bool BatteryManager::begin() {
    pmu = new XPowersAXP2101(Wire, 21, 22);

    if (!pmu) {
        Serial.println("BatteryManager: error creando AXP2101");
        ok = false;
        return false;
    }

    if (!pmu->init()) {
        Serial.println("BatteryManager: AXP2101 init FAIL");
        delete pmu;
        pmu = nullptr;
        ok = false;
        return false;
    }

    // Igual que en el ejemplo oficial:
    // sin esto, la medicion puede salir mal o no actualizarse
    pmu->disableTSPinMeasure();
    pmu->enableBattDetection();
    pmu->enableBattVoltageMeasure();
    pmu->enableVbusVoltageMeasure();
    pmu->enableSystemVoltageMeasure();

    ok = true;
    Serial.println("BatteryManager: AXP2101 init OK");
    update(true);
    return true;
}

void BatteryManager::update(bool force) {
    if (!ok || pmu == nullptr) return;

    unsigned long now = millis();
    if (!force && (now - lastReadMs < READ_PERIOD_MS)) return;
    lastReadMs = now;

    charging = pmu->isCharging();

    if (pmu->isBatteryConnect()) {
        voltage = pmu->getBattVoltage() / 1000.0f;
        percent = pmu->getBatteryPercent();
    } else {
        voltage = 0.0f;
        percent = -1;
        charging = false;
    }


}