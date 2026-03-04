#include "display.h"
#include "config.h"
#include <Wire.h>

DisplayManager::DisplayManager(uint8_t sda_pin, uint8_t scl_pin, uint8_t screen_address) 
    : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {
    Wire.begin(sda_pin, scl_pin);
}

bool DisplayManager::init() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println("Error: Display OLED no encontrado");
        return false;
    }
    
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.clearDisplay();
    display.println("Display inicializado");
    display.display();
    
    Serial.println("Display OLED 0.96\" inicializado correctamente");
    return true;
}

void DisplayManager::showSplash() {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.println("LoRa");
    display.println("Coverage");
    display.println("Test");
    display.setTextSize(1);
    display.setCursor(0, 50);
    display.println("T-Beam v1.1");
    display.display();
}

void DisplayManager::showReady() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("=== LoRa Coverage ===");
    display.println();
    display.println("Status: READY");
    display.println();
    display.println("Presiona el boton para");
    display.println("iniciar la prueba");
    display.display();
}

void DisplayManager::showTesting() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("=== LoRa Coverage ===");
    display.println();
    display.println("Status: TESTING...");
    display.println();
    display.println("Escaneando cobertura");
    display.println("por favor espera...");
    display.display();
}

void DisplayManager::showStatus2(const char* line1, const char* line2) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(line1);
  display.println(line2);
  display.display();
}

void DisplayManager::showResults(int rssi, float snr, int packetCount,  bool fixValid, double lon, double lat, const char* dateStr, const char* mac) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("=Test Results+GPS=");
    
    // ORDEN pedido: LON, LAT, TS, RSSI, SNR
    display.print("LON: ");
    if (fixValid) display.println(lon, 6);
    else display.println("NO FIX");

    display.print("LAT: ");
    if (fixValid) display.println(lat, 6);
    else display.println("NO FIX");

display.print("T:");
    
display.println(dateStr);

    
    // RSSI
    display.print("RSSI: ");
    display.print(rssi);
    display.println(" dBm");
    
    // SNR
    display.print("SNR: ");
    display.print(snr, 1);
    display.println(" dB");

         // SNR
    display.print("MAC:");
    display.println(mac);
    
    // Packet Count
    display.print("Packets: ");
    display.println(packetCount);
    
    display.println();
    
    // Signal Quality
    display.print("Calidad: ");
    if (rssi > -80) {
        display.println("Excelente");
    } else if (rssi > -100) {
        display.println("Buena");
    } else if (rssi > -120) {
        display.println("Aceptable");
    } else {
        display.println("Pobre");
    }
    
    display.display();
}

void DisplayManager::showError(const char* error) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("=== ERROR ===");
    display.println();
    display.println(error);
    display.display();
}

void DisplayManager::clear() {
    display.clearDisplay();
    display.display();
}
