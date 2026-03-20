#ifndef LORA_MANAGER_H
#define LORA_MANAGER_H

#include <lmic.h>
#include <hal/hal.h>

typedef struct {
    int rssi;
    float snr;
    int packetCount;
    bool testComplete;
    bool testRunning;
    bool gotDownlink;
} LoRaTestResults;

class LoRaManager {
public:
    LoRaTestResults testResults;
    bool isTestRunning;
    
    // Validar si el DevEUI está autorizado
    bool isNodeAuthorized(const uint8_t* devEui);
    
public:
    LoRaManager();
    bool init();
    void startCoverageTest();
    void update();
    LoRaTestResults getResults() { return testResults; }
    bool isTestingComplete() { return testResults.testComplete; }
    void resetTest();
};

extern LoRaManager loraManager;

// LMIC callbacks (requeridos)
void os_getArtEui(u1_t* buf);
void os_getDevEui(u1_t* buf);
void os_getDevKey(u1_t* buf);
void onEvent(ev_t event);

#endif


