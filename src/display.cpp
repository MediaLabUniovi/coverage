#include "display.h"
#include "config.h"
#include <Wire.h>

DisplayManager::DisplayManager(uint8_t sda_pin, uint8_t scl_pin, uint8_t screen_address)
    : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {
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
    display.println("T-Beam v1.2");
    display.display();
}

void DisplayManager::showReady(float battVoltage, int battPercent, bool charging) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);

    if (battPercent >= 0) {
        display.printf("BAT: %.2fV %d%%", battVoltage, battPercent);
        if (charging) display.print(" C");
        display.println();
    } else {
        display.println("BAT: N/A");
    }

    display.println("--------------------");
    display.println("BOTON CENTRAL ");

    display.println("Pulsa corto-> medir");
     display.println("Pulsa largo-> guardar");
     display.println();
    display.println("BOTON DERECH0 ");

    display.println("resetear");
    display.display();
}

void DisplayManager::showTesting(float battVoltage, int battPercent, bool charging) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);

    if (battPercent >= 0) {
        display.printf("BAT: %.2fV %d%%", battVoltage, battPercent);
        if (charging) display.print(" C");
        display.println();
    } else {
        display.println("BAT: N/A");
    }

    display.println("--------------------");
    
   
    display.println("Escaneando cobertura");
    display.println("por favor espera...");
        display.println("Recuerda colocar");
    display.println("la antena en vertical!!");
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

void DisplayManager::showResults(int rssi, float snr, int packetCount, bool fixValid, double lon, double lat, const char* dateStr, const char* mac) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("=RESULTADOS=");

    display.print("LON: ");
    if (fixValid) display.println(lon, 6);
    else display.println("No disponible");

    display.print("LAT: ");
    if (fixValid) display.println(lat, 6);
    else display.println("Muevete a exterior");

    display.print("T: ");
    display.println(dateStr);

    display.print("RSSI: ");
    display.print(rssi);
    display.println(" dBm");

    display.print("SNR: ");
    display.print(snr, 1);
    display.println(" dB");



    display.println();

    display.print("Calidad: ");
    if (rssi == 0 && snr == 0) {
    display.println("Sin cobertura");
} else if (rssi > -80) {
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