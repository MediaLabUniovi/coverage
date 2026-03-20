#pragma once
#include <Arduino.h>

bool batteryBegin();
float readBatteryVoltage();   // en V
int readBatteryPercent();     // 0..100 (si PMU lo da)