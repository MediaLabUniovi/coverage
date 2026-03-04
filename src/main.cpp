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

String g_mac;

// Display Manager
DisplayManager display(I2C_SDA, I2C_SCL, SCREEN_ADDRESS);

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

// ✅ Mantener resultados en pantalla X ms
unsigned long resultsTimer = 0;
const unsigned long RESULTS_HOLD_MS = 10000; // 10s

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

  WiFi.mode(WIFI_STA);          // para poder leer MAC STA
  g_mac = WiFi.macAddress();
  Serial.print("MAC (STA): ");
  Serial.println(g_mac);

  gpsManager.begin(GPS_RX, GPS_TX, GPS_BAUD);
  delay(1000);

  Serial.println("\n\n==== LoRa Coverage Test - T-Beam ====\n");

  // Botón
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);
  Serial.println("Botón inicializado en GPIO " + String(BUTTON_PIN));

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
        display.showReady();
        Serial.println("Esperando presión de botón...");
      }
      break;

    case STATE_READY:
      // Pulsación corta => medir
      if (shortPress) {
        currentState = STATE_TESTING;
        display.showTesting();
        loraManager.startCoverageTest();
        testTimer = millis();
        Serial.println("Prueba iniciada...");
      }

      // Pulsación larga => subir CSV (verifica WiFi)
      if (longPress) {
        Serial.println("LONG PRESS -> Upload CSV");
        Serial.println("Conectando WiFi (WiFiManager)...");
       display.showStatus2("UPLOAD", "Conectando WiFi...");
        display.showTesting(); // si quieres luego hacemos un showUploading()

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
          display.showReady();
        } else {
          Serial.println("UPLOAD FAIL");
          display.showError("UPLOAD FAIL");
        }
      }
      break;

    case STATE_TESTING: {
      if (loraManager.isTestingComplete()) {

        // ✅ Entramos en RESULTS y mantenemos pantalla 10s
        buttonPressed = false;        // evita salida inmediata por ISR previa
        resultsTimer = millis();      // arranca contador de hold
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

        if (!results.gotDownlink) {
          display.showError("NO ACK");
          Serial.println("Sin ACK/downlink => no hay dB reales en el nodo");
        } else {
          display.showResults(results.rssi, results.snr, results.packetCount,
                              fix.valid, fix.lon, fix.lat,
                              dateStr, g_mac.c_str());
        }

        Serial.println("\nResultados de cobertura:");
        Serial.print("  RSSI: "); Serial.print(results.rssi); Serial.println(" dBm");
        Serial.print("  SNR:  "); Serial.println(results.snr);
        Serial.print("  MAC:  "); Serial.println(g_mac);
        Serial.print("  T:    "); Serial.println(dateStr);

        if (fix.valid) {
          Serial.print("  LAT: "); Serial.println(fix.lat, 6);
          Serial.print("  LON: "); Serial.println(fix.lon, 6);
        } else {
          Serial.println("  GPS: NO FIX");
        }

        // Guardado
        if (results.gotDownlink && fix.valid && fix.timeValid) {
          if (!storageManager.appendPoint(fix.lon, fix.lat, dateStr,
                                          results.rssi, results.snr,
                                          g_mac.c_str())) {
            Serial.println("ERROR: no se pudo guardar en LittleFS");
          }
        } else {
          Serial.println("No guardo punto: falta ACK o GPS/fecha");
        }
      }

      // Timeout
      if (millis() - testTimer > 30000) {
        buttonPressed = false;
        resultsTimer = millis();
        currentState = STATE_RESULTS;
        Serial.println("Timeout en la prueba");
        display.showError("Test Timeout");
      }
      break;
    }

    case STATE_RESULTS:
      // ✅ Mantener resultados en pantalla 10s, luego volver solo
      if (millis() - resultsTimer > RESULTS_HOLD_MS) {
        loraManager.resetTest();
        currentState = STATE_READY;
        display.showReady();
        Serial.println("\nListo para nueva prueba (auto)");
      }

      // O volver antes si pulsas
      if (buttonPressed) {
        buttonPressed = false;
        loraManager.resetTest();
        currentState = STATE_READY;
        display.showReady();
        Serial.println("\nListo para nueva prueba");
      }
      break;
  }

  delay(10);
}