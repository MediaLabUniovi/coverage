#pragma once
#include <Arduino.h>
#include <TinyGPSPlus.h>

struct GpsFix {
    bool valid;
    double lat;
    double lon;
    uint32_t ts;   // 0 si no lo calculas aún

      // ✅ hora local (España)
  bool timeValid;
      uint16_t year;
    uint8_t month;
    uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
};

class GpsManager {
public:
    void begin(int rxPin, int txPin, uint32_t baud);
    void update();
    GpsFix getFix() const { return fix; }

    double lat() const { return fix.lat; }
    double lon() const { return fix.lon; }


    // ✅ DEBUG getters
    int sats();
    double hdop();
    bool locValid() const;
    uint32_t locAge() ;
 

private:
    TinyGPSPlus gps;
    GpsFix fix{false, 0.0, 0.0, false, 0, 0, 0, 0, 0, 0};
};

extern GpsManager gpsManager;