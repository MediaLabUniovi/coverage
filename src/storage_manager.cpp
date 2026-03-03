#include "storage_manager.h"
#include <LittleFS.h>

StorageManager storageManager;

bool StorageManager::begin() {
  if (!LittleFS.begin(true)) {   // true = formatea si falla (para empezar es cómodo)
    Serial.println("LittleFS begin failed");
    return false;
  }

  // Si el archivo no existe, crea cabecera CSV
  if (!LittleFS.exists(path)) {
    File f = LittleFS.open(path, FILE_WRITE);
    if (!f) return false;
    f.println("lon,lat,datetime,rssi,snr,mac");
    f.close();
  }

  Serial.println("LittleFS OK");
  return true;
}

bool StorageManager::appendPoint(double lon, double lat, const char* datetime, int rssi, float snr, const char* mac) {
  File f = LittleFS.open(path, FILE_APPEND);
  if (!f) {
    Serial.println("Open cov.csv failed");
    return false;
  }

  // lon,lat,datetime,rssi,snr
  f.print(lon, 6); f.print(",");
  f.print(lat, 6); f.print(",");
  f.print(datetime); f.print(",");
  f.print(rssi); f.print(",");
  f.print(snr,2); f.print(",");
  f.println(mac);
  

  f.close();
  Serial.println("Point saved to /cov.csv");
  return true;
}

void StorageManager::resetFile() {
  if (LittleFS.exists(path)) LittleFS.remove(path);
  File f = LittleFS.open(path, FILE_WRITE);
  if (f) {
    f.println("lon,lat,datetime,rssi,snr,mac");
    f.close();
  }
}





