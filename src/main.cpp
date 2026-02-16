#include <Arduino.h>
#include "config.h"
#include "display.h"
#include "lora_manager.h"

// Display Manager
DisplayManager display(I2C_SDA, I2C_SCL, SCREEN_ADDRESS);

// Button state
volatile bool buttonPressed = false;
unsigned long lastButtonPress = 0;
const unsigned long DEBOUNCE_TIME = 200;  // ms

// State machine
enum State {
    STATE_SPLASH,
    STATE_READY,
    STATE_TESTING,
    STATE_RESULTS
};

State currentState = STATE_SPLASH;
unsigned long splashTimer = 0;
unsigned long testTimer = 0;

// ==================== ISR ====================
void IRAM_ATTR buttonISR() {
    unsigned long now = millis();
    if (now - lastButtonPress > DEBOUNCE_TIME) {
        buttonPressed = true;
        lastButtonPress = now;
    }
}

// ==================== SETUP ====================
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n==== LoRa Coverage Test - T-Beam ====\n");
    
    // Inicializar botón
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);
    Serial.println("Botón inicializado en GPIO " + String(BUTTON_PIN));
    
    // Inicializar display
    if (!display.init()) {
        Serial.println("FATAL: No se pudo inicializar el display");
        while(1) delay(100);
    }
    
    // Mostrar splash screen
    display.showSplash();
    splashTimer = millis();
    
    // Inicializar LoRa
    if (!loraManager.init()) {
        Serial.println("FATAL: No se pudo inicializar LoRa");
        display.showError("LoRa Init Failed");
        while(1) delay(100);
    }
    
    Serial.println("Sistema listo para pruebas de cobertura\n");
}

// ==================== LOOP ====================
void loop() {
    // Actualizar LoRa
    loraManager.update();
    
    // State machine
    switch(currentState) {
        case STATE_SPLASH:
            if (millis() - splashTimer > 3000) {
                currentState = STATE_READY;
                display.showReady();
                Serial.println("Esperando presión de botón...");
            }
            break;
            
        case STATE_READY:
            if (buttonPressed) {
                buttonPressed = false;
                currentState = STATE_TESTING;
                display.showTesting();
                loraManager.startCoverageTest();
                testTimer = millis();
                Serial.println("Prueba iniciada...");
            }
            break;
            
        case STATE_TESTING:
            // Esperar a que la prueba se complete
            if (loraManager.isTestingComplete()) {
                currentState = STATE_RESULTS;
                LoRaTestResults results = loraManager.getResults();
                display.showResults(results.rssi, results.snr, results.packetCount);
                Serial.println("\nResultados de cobertura:");
                Serial.print("  RSSI: "); Serial.print(results.rssi); Serial.println(" dBm");
                Serial.print("  SNR: "); Serial.print(results.snr, 1); Serial.println(" dB");
                Serial.print("  Packets: "); Serial.println(results.packetCount);
            }
            
            // Timeout de seguridad (10 segundos)
            if (millis() - testTimer > 10000) {
                currentState = STATE_RESULTS;
                Serial.println("Timeout en la prueba");
                display.showError("Test Timeout");
            }
            break;
            
        case STATE_RESULTS:
            if (buttonPressed) {
                buttonPressed = false;
                loraManager.resetTest();
                currentState = STATE_READY;
                display.showReady();
                Serial.println("\nListo para nueva prueba");
            }
            break;
    }
    
    delay(10);  // Pequeño delay para evitar saturar el loop
}