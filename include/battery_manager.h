#ifndef BATTERY_MANAGER_H
#define BATTERY_MANAGER_H

#include <Arduino.h>
#include "XPowersLib.h"

class BatteryManager {
private:
    XPowersAXP2101* pmu = nullptr;
    bool ok = false;
    float voltage = 0.0f;
    int percent = -1;
    bool charging = false;

    unsigned long lastReadMs = 0;
    static const unsigned long READ_PERIOD_MS = 3000;

public:
    bool begin();
    void update(bool force = false);

    bool isReady() const { return ok; }
    float getVoltage() const { return voltage; }
    int getPercent() const { return percent; }
    bool isCharging() const { return charging; }
};

#endif