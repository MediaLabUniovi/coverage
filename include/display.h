#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

class DisplayManager {
private:
    Adafruit_SSD1306 display;
    
public:
    DisplayManager(uint8_t sda_pin, uint8_t scl_pin, uint8_t screen_address);
    bool init();
    void showSplash();
    void showReady();
    void showTesting();
    void showResults(int rssi, float snr, int packetCount,   bool fixValid, double lon, double lat,  const char* dateStr, const char* mac);
    void showError(const char* error);
    void clear();
    void display_update() { display.display(); }
    void showStatus2(const char* line1, const char* line2);
};



#endif
