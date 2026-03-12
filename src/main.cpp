#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <Wire.h>
#include "config.h"
#include "display.h"
#include "lora_manager.h"
#include "gps_manager.h"
#include "storage_manager.h"
#include "uploader.h"
#include "LongPress.h"
#include "battery_manager.h"

// Constants
const unsigned long SPLASH_TIMEOUT_MS = 3000;
const unsigned long TEST_TIMEOUT_MS = 30000;
const unsigned long UPLOAD_MSG_DELAY_MS = 4000;
const int WIFI_MANAGER_PORTAL_TIMEOUT_SEC = 180;
const int WIFI_MANAGER_CONNECT_TIMEOUT_SEC = 20;

char g_mac[18];

// Function declarations
void saveAndShowResults(LoRaTestResults results, bool success);

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

volatile bool buttonPressed = false; // Used if we still need a global flag, though shortPress replaces most of its usage
unsigned long lastButtonPress = 0;
const unsigned long DEBOUNCE_TIME = 200;  // ms

// State machine
enum State {
  STATE_SPLASH,
  STATE_READY,
  STATE_TESTING,
  STATE_RESULTS,
  STATE_UPLOADING
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
// ISR eliminada a favor de checkLongPress en el loop

// ==================== WiFi Manager ====================
static bool wifiConnectWithManager() {
  WiFi.mode(WIFI_STA);

  WiFiManager wm;

  wm.setConfigPortalTimeout(WIFI_MANAGER_PORTAL_TIMEOUT_SEC);
  wm.setConnectTimeout(WIFI_MANAGER_CONNECT_TIMEOUT_SEC);

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
  String macTemplate = WiFi.macAddress();
  strncpy(g_mac, macTemplate.c_str(), sizeof(g_mac));
  g_mac[sizeof(g_mac) - 1] = '\0';
  Serial.print("MAC (STA): ");
  Serial.println(g_mac);

  gpsManager.begin(GPS_RX, GPS_TX, GPS_BAUD);
  delay(1000);

  Serial.println("\n\n==== LoRa Coverage Test - T-Beam ====\n");

  pinMode(BUTTON_PIN, INPUT_PULLUP);
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
    delay(5000);
    ESP.restart();
  }

  // LittleFS
  if (!storageManager.begin()) {
    Serial.println("FATAL: LittleFS no inicia");
    display.showError("LittleFS FAIL");
    delay(5000);
    ESP.restart();
  }

  // Splash
  display.showSplash();
  splashTimer = millis();

  // LoRa
  if (!loraManager.init()) {
    Serial.println("FATAL: No se pudo inicializar LoRa");
    display.showError("LoRa Init Failed");
    delay(5000);
    ESP.restart();
  }

  Serial.println("Sistema listo para pruebas de cobertura\n");
}

void saveAndShowResults(LoRaTestResults results, bool success) {
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

  int rssiToStore = success && results.gotDownlink ? results.rssi : 0;
  float snrToStore = success && results.gotDownlink ? results.snr : 0.0f;
  uint8_t packetCount = success ? results.packetCount : 0;

  if (!success) {
    Serial.println("Timeout en la prueba -> se guardan RSSI=0 y SNR=0");
  } else if (!results.gotDownlink) {
    Serial.println("Sin ACK/downlink -> se guardan RSSI=0 y SNR=0");
  }

  display.showResults(rssiToStore, snrToStore, packetCount,
                      fix.valid, fix.lon, fix.lat,
                      dateStr, g_mac);

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

  if (fix.valid && fix.timeValid) {
    if (!storageManager.appendPoint(fix.lon, fix.lat, dateStr,
                                    rssiToStore, snrToStore,
                                    g_mac)) {
      Serial.println("ERROR: no se pudo guardar en LittleFS");
    }
  } else {
    Serial.println("No guardo punto: falta GPS o fecha");
  }
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
      if (millis() - splashTimer > SPLASH_TIMEOUT_MS) {
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
          WiFi.mode(WIFI_OFF);
          break;
        }

        currentState = STATE_UPLOADING;
      }
      break;

    case STATE_UPLOADING: {
      display.showStatus2("Conectado", "Subiendo cov.csv...");
      Serial.println("Subiendo /cov.csv...");
      bool ok = uploader.uploadCSV();

      if (ok) {
        display.showStatus2("UPLOAD OK", "CSV enviado");
        Serial.println("UPLOAD OK");
        delay(UPLOAD_MSG_DELAY_MS); // Dar un momento para que el usuario lo lea antes de volver a READY
      } else {
        Serial.println("UPLOAD FAIL");
        display.showError("UPLOAD FAIL");
        delay(UPLOAD_MSG_DELAY_MS);
      }

      // Apagamos la radio para ahorrar batería
      WiFi.mode(WIFI_OFF);
      
      currentState = STATE_READY;

      batteryManager.update(true);
      display.showReady(
        batteryManager.getVoltage(),
        batteryManager.getPercent(),
        batteryManager.isCharging()
      );
      batteryUiTimer = millis();

      break;
    }

    case STATE_TESTING: {
      if (loraManager.isTestingComplete()) {

        buttonPressed = false;
        resultsTimer = millis();
        currentState = STATE_RESULTS;

        LoRaTestResults results = loraManager.getResults();
        saveAndShowResults(results, true);
      }

      // Timeout
      if (millis() - testTimer > TEST_TIMEOUT_MS) {
        buttonPressed = false;
        resultsTimer = millis();
        currentState = STATE_RESULTS;

        LoRaTestResults dummyResults;
        saveAndShowResults(dummyResults, false);
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
      if (shortPress) {
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