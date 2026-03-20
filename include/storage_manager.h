#pragma once
#include <Arduino.h>

class StorageManager {
public:
  bool begin();
  bool appendPoint(double lon, double lat, const char* datetime, int rssi, float snr, const char* mac);
  void resetFile(); // opcional
    void dumpToSerial();

private:
  const char* path = "/cov.csv";
};

void dumpToSerial();
extern StorageManager storageManager;