#include <Arduino.h>
#include "lora_manager.h"
#include "config.h"
#include <SPI.h>

LoRaManager loraManager;

// LMIC pin mapping para T-Beam
const lmic_pinmap lmic_pins = {
    .nss = LORA_CS,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = LORA_RST,
    .dio = {LORA_DIO0, LORA_DIO1, LORA_DIO2},
};

// LMIC callbacks (requeridos por la librería)
void os_getArtEui(u1_t* buf) {
    memcpy_P(buf, APPEUI, 8);
}

void os_getDevEui(u1_t* buf) {
    memcpy_P(buf, DEVEUI, 8);
}

void os_getDevKey(u1_t* buf) {
    memcpy_P(buf, APPKEY, 16);
}


// Event handler para LMIC
void onEvent(ev_t event) {
    switch(event) {
        case EV_SCAN_TIMEOUT:
            Serial.println("EV_SCAN_TIMEOUT");
            break;
        case EV_BEACON_FOUND:
            Serial.println("EV_BEACON_FOUND");
            break;
        case EV_BEACON_MISSED:
            Serial.println("EV_BEACON_MISSED");
            break;
        case EV_BEACON_TRACKED:
            Serial.println("EV_BEACON_TRACKED");
            break;
        case EV_JOINING:
            Serial.println("EV_JOINING");
            break;
        case EV_JOINED:
            Serial.println("EV_JOINED");
            break;
        case EV_RFU1:
            Serial.println("EV_RFU1");
            break;
        case EV_JOIN_FAILED:
            Serial.println("EV_JOIN_FAILED");
            break;
        case EV_REJOIN_FAILED:
            Serial.println("EV_REJOIN_FAILED");
            break;
        case EV_TXCOMPLETE: {
             
            Serial.println("EV_TXCOMPLETE");
             bool gotAck = (LMIC.txrxFlags & TXRX_ACK);
    bool gotDl  = (LMIC.dataLen > 0);

    loraManager.testResults.gotDownlink = gotAck || gotDl;

    if (loraManager.testResults.gotDownlink) {

           // ✅ 1) Ver el RSSI tal cual lo da LMIC (crudo)
    Serial.print("LMIC.rssi raw = ");
    Serial.println(LMIC.rssi);





        loraManager.testResults.rssi = (int)LMIC.rssi-91;
        loraManager.testResults.snr  = LMIC.snr / 4.0f;   // si snr es float
        Serial.printf("ACK/DL OK -> RSSI=%d dBm SNR=%.2f dB\n",
                      loraManager.testResults.rssi,
                      loraManager.testResults.snr);
    } else {
        Serial.println("NO ACK / NO DOWNLINK");
    }

    loraManager.testResults.packetCount++;
    loraManager.testResults.testComplete = true;
    loraManager.testResults.testRunning  = false;
    loraManager.isTestRunning = false;
            break;
}
        case EV_LOST_TSYNC:
            Serial.println("EV_LOST_TSYNC");
            break;
        case EV_RESET:
            Serial.println("EV_RESET");
            break;
        case EV_RXCOMPLETE:
            Serial.println("EV_RXCOMPLETE");
            break;
        case EV_LINK_DEAD:
            Serial.println("EV_LINK_DEAD");
            break;
        case EV_LINK_ALIVE:
            Serial.println("EV_LINK_ALIVE");
            break;
        default:
            Serial.print("Unknown event: ");
            Serial.println((unsigned) event);
            break;
    }
}

// Constructor
LoRaManager::LoRaManager() : isTestRunning(false) {
    testResults = {0, 0.0f, 0, false, false, false};
}

bool LoRaManager::init() {
    Serial.println("Inicializando LMIC-Arduino...");
    
    // Inicializar SPI
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
    
    // Inicializar LMIC
    os_init();
    
    // Reset LMIC
    LMIC_reset();
    LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100); // 1% (ESP32)
LMIC.dn2Dr = DR_SF9;                           // RX2 típico TTN EU868

    
    // Set data rate y transmit power para coverage test
    LMIC_setDrTxpow(DR_SF12, 14);
    
    // Desactivar todos los canales excepto el 0
   // for (int channel = 1; channel < 72; ++channel) {
     //   LMIC_disableChannel(channel);
    //}
    
    // Desactivar link check
    LMIC_setLinkCheckMode(0);
    
    Serial.println("LMIC inicializado correctamente");
    Serial.println("Frecuencia: 868 MHz (EU868)");
    Serial.println("Spreading Factor: 12");
    
    LMIC_startJoining();

    return true;
}

void LoRaManager::startCoverageTest() {
    if (isTestRunning) return;
    
    Serial.println("\n=== Iniciando prueba de cobertura LoRa ===");
    isTestRunning = true;
    testResults.testRunning = true;
    testResults.testComplete = false;
    
    // Crear payload simple de prueba
    uint8_t testData[] = "TEST";
    
    // Establecer transmisión sin confirmación
    LMIC_setTxData2(1, testData, sizeof(testData)-1, 1);
    Serial.println("Paquete transmitido, esperando resultado...");
}

void LoRaManager::update() {
    // Ejecutar LMIC
    os_runloop_once();
    
   
}

void LoRaManager::resetTest() {
    testResults = {0, 0, 0, false, false};
    isTestRunning = false;
}

// Validar si el DevEUI está en la lista de nodos autorizados
bool LoRaManager::isNodeAuthorized(const uint8_t* devEui) {
    const uint8_t authorizedNodes[][8] = AUTHORIZED_NODES;
    
    for (int i = 0; i < AUTHORIZED_NODES_COUNT; i++) {
        if (memcmp(devEui, authorizedNodes[i], 8) == 0) {
            Serial.println("✓ Nodo autorizado detectado");
            return true;
        }
    }
    
    Serial.print("✗ Nodo NO autorizado: ");
    for (int i = 0; i < 8; i++) {
        if (devEui[i] < 0x10) Serial.print("0");
        Serial.print(devEui[i], HEX);
    }
    Serial.println();
    return false;
}



