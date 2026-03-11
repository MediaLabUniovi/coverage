#include <Arduino.h>
#include "config.h"
#include "display.h"
#include "lora_manager.h"
#include "gps_manager.h"
#include "storage_manager.h"
#include "uploader.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include "LongPress.h"
#include "battery_manager.h"
#include <Wire.h>

String g_mac;

// Display Manager
DisplayManager display(I2C_SDA, I2C_SCL, SCREEN_ADDRESS);
BatteryManager batteryManager;

LongPressConfig lpCfg = {
  .buttonPin = BUTTON_PIN,
  .sampleIntervalMs = 10,
  .stableSamplesRequired = 3,
  .longPressThresholdMs = 1500   // 1.5s
};

LongPressState lpState;

// Button state (para volver desde RESULTS)
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

// Mantener resultados en pantalla X ms
unsigned long resultsTimer = 0;
const unsigned long RESULTS_HOLD_MS = 10000; // 10s
unsigned long batteryUiTimer = 0;
const unsigned long BATTERY_UI_REFRESH_MS = 3000;

// ==================== ISR ====================
void IRAM_ATTR buttonISR() {
  unsigned long now = millis();
  if (now - lastButtonPress > DEBOUNCE_TIME) {
    buttonPressed = true;
    lastButtonPress = now;
  }
}

// ==================== WiFi Manager ====================
static bool wifiConnectWithManager() {
  WiFi.mode(WIFI_STA);

  WiFiManager wm;

  wm.setConfigPortalTimeout(180); // 3 min
  wm.setConnectTimeout(20);

  // AP si no hay credenciales guardadas:
  // SSID: TBEAM-COV
  // PASS: 12345678
  bool ok = wm.autoConnect("TBEAM-COV", "12345678");

  if (!ok) {
    Serial.println("WiFiManager: NO conectado");
    return false;
  }

  Serial.print("WiFi OK. IP=");
  Serial.println(WiFi.localIP());
  return true;
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  g_mac = WiFi.macAddress();
  Serial.print("MAC (STA): ");
  Serial.println(g_mac);

  gpsManager.begin(GPS_RX, GPS_TX, GPS_BAUD);
  delay(1000);

  Serial.println("\n\n==== LoRa Coverage Test - T-Beam ====\n");

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);
  Serial.println("Botón inicializado en GPIO " + String(BUTTON_PIN));

  // I2C una sola vez
  Wire.begin(I2C_SDA, I2C_SCL);

  // PMU / batería
  if (!batteryManager.begin()) {
    Serial.println("WARN: no se pudo inicializar lectura de bateria");
  }

  // Display
  if (!display.init()) {
    Serial.println("FATAL: No se pudo inicializar el display");
    while (1) delay(100);
  }

  // LittleFS
  if (!storageManager.begin()) {
    Serial.println("FATAL: LittleFS no inicia");
    display.showError("LittleFS FAIL");
    while (1) delay(100);
  }

  // Splash
  display.showSplash();
  splashTimer = millis();

  // LoRa
  if (!loraManager.init()) {
    Serial.println("FATAL: No se pudo inicializar LoRa");
    display.showError("LoRa Init Failed");
    while (1) delay(100);
  }

  Serial.println("Sistema listo para pruebas de cobertura\n");
}

// ==================== LOOP ====================
void loop() {

  // Long press / short press
  LongPressResult res = checkLongPress(lpState, lpCfg);
  bool shortPress = (res == PRESS_CANCELLED);
  bool longPress  = (res == LONG_PRESS_DETECTED);

  // Actualizar LoRa + GPS
  loraManager.update();
  gpsManager.update();

  switch (currentState) {

    case STATE_SPLASH:
      if (millis() - splashTimer > 3000) {
        currentState = STATE_READY;

        batteryManager.update(true);
        display.showReady(
          batteryManager.getVoltage(),
          batteryManager.getPercent(),
          batteryManager.isCharging()
        );
        batteryUiTimer = millis();

        Serial.println("Esperando presión de botón...");
      }
      break;

    case STATE_READY:
      batteryManager.update();

      if (millis() - batteryUiTimer > BATTERY_UI_REFRESH_MS) {
        display.showReady(
          batteryManager.getVoltage(),
          batteryManager.getPercent(),
          batteryManager.isCharging()
        );
        batteryUiTimer = millis();
      }

      // Pulsación corta => medir
      if (shortPress) {
        currentState = STATE_TESTING;

        batteryManager.update(true);
        display.showTesting(
          batteryManager.getVoltage(),
          batteryManager.getPercent(),
          batteryManager.isCharging()
        );

        loraManager.startCoverageTest();
        testTimer = millis();
        Serial.println("Prueba iniciada...");
      }

      // Pulsación larga => subir CSV (verifica WiFi)
      if (longPress) {
        Serial.println("LONG PRESS -> Upload CSV");
        Serial.println("Conectando WiFi (WiFiManager)...");
        display.showStatus2("UPLOAD", "Conectando WiFi...");
        display.showTesting(
          batteryManager.getVoltage(),
          batteryManager.getPercent(),
          batteryManager.isCharging()
        );

        if (!wifiConnectWithManager()) {
          display.showError("WiFi FAIL");
          break;
        }

        display.showStatus2("Conectado", "Subiendo cov.csv...");
        Serial.println("Subiendo /cov.csv...");
        bool ok = uploader.uploadCSV();

        if (ok) {
          display.showStatus2("UPLOAD OK", "CSV enviado");
          Serial.println("UPLOAD OK");
          delay(4000);

          batteryManager.update(true);
          display.showReady(
            batteryManager.getVoltage(),
            batteryManager.getPercent(),
            batteryManager.isCharging()
          );
          batteryUiTimer = millis();

        } else {
          Serial.println("UPLOAD FAIL");
          display.showError("UPLOAD FAIL");
        }
      }
      break;

    case STATE_TESTING: {
      if (loraManager.isTestingComplete()) {

        buttonPressed = false;
        resultsTimer = millis();
        currentState = STATE_RESULTS;

        LoRaTestResults results = loraManager.getResults();
        GpsFix fix = gpsManager.getFix();

        char dateStr[24];
        if (fix.timeValid) {
          snprintf(dateStr, sizeof(dateStr),
                   "%04u-%02u-%02u %02u:%02u:%02u",
                   fix.year, fix.month, fix.day,
                   fix.hour, fix.minute, fix.second);
        } else {
          snprintf(dateStr, sizeof(dateStr), "DATE: NO GPS TIME");
        }

        int rssiToStore = results.gotDownlink ? results.rssi : 0;
        float snrToStore = results.gotDownlink ? results.snr : 0.0f;

        if (!results.gotDownlink) {
          Serial.println("Sin ACK/downlink -> se guardan RSSI=0 y SNR=0");
        }

        display.showResults(rssiToStore, snrToStore, results.packetCount,
                            fix.valid, fix.lon, fix.lat,
                            dateStr, g_mac.c_str());

        Serial.println("\nResultados de cobertura:");
        Serial.print("  RSSI: "); Serial.print(rssiToStore); Serial.println(" dBm");
        Serial.print("  SNR:  "); Serial.println(snrToStore);
        Serial.print("  MAC:  "); Serial.println(g_mac);
        Serial.print("  T:    "); Serial.println(dateStr);

        if (fix.valid) {
          Serial.print("  LAT: "); Serial.println(fix.lat, 6);
          Serial.print("  LON: "); Serial.println(fix.lon, 6);
        } else {
          Serial.println("  GPS: NO FIX");
        }

        // Guardado: ahora se guarda aunque no haya ACK, usando 0/0
        if (fix.valid && fix.timeValid) {
          if (!storageManager.appendPoint(fix.lon, fix.lat, dateStr,
                                          rssiToStore, snrToStore,
                                          g_mac.c_str())) {
            Serial.println("ERROR: no se pudo guardar en LittleFS");
          }
        } else {
          Serial.println("No guardo punto: falta GPS o fecha");
        }
      }

      // Timeout
      if (millis() - testTimer > 30000) {
        buttonPressed = false;
        resultsTimer = millis();
        currentState = STATE_RESULTS;

        Serial.println("Timeout en la prueba -> se guardan RSSI=0 y SNR=0");

        GpsFix fix = gpsManager.getFix();

        char dateStr[24];
        if (fix.timeValid) {
          snprintf(dateStr, sizeof(dateStr),
                   "%04u-%02u-%02u %02u:%02u:%02u",
                   fix.year, fix.month, fix.day,
                   fix.hour, fix.minute, fix.second);
        } else {
          snprintf(dateStr, sizeof(dateStr), "DATE: NO GPS TIME");
        }

        display.showResults(0, 0.0f, 0,
                            fix.valid, fix.lon, fix.lat,
                            dateStr, g_mac.c_str());

        if (fix.valid && fix.timeValid) {
          if (!storageManager.appendPoint(fix.lon, fix.lat, dateStr,
                                          0, 0.0f,
                                          g_mac.c_str())) {
            Serial.println("ERROR: no se pudo guardar en LittleFS");
          }
        } else {
          Serial.println("No guardo punto de timeout: falta GPS o fecha");
        }
      }
      break;
    }

    case STATE_RESULTS:
      if (millis() - resultsTimer > RESULTS_HOLD_MS) {
        loraManager.resetTest();
        currentState = STATE_READY;

        batteryManager.update(true);
        display.showReady(
          batteryManager.getVoltage(),
          batteryManager.getPercent(),
          batteryManager.isCharging()
        );
        batteryUiTimer = millis();

        Serial.println("\nListo para nueva prueba (auto)");
      }

      // O volver antes si pulsas
      if (buttonPressed) {
        buttonPressed = false;
        loraManager.resetTest();
        currentState = STATE_READY;

        batteryManager.update(true);
        display.showReady(
          batteryManager.getVoltage(),
          batteryManager.getPercent(),
          batteryManager.isCharging()
        );
        batteryUiTimer = millis();

        Serial.println("\nListo para nueva prueba");
      }
      break;
  }

  delay(10);
}