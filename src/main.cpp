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

// Medidas múltiples
const unsigned long MULTI_MEASURE_INTERVAL_MS = 10000;   // 10 segundos

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

// State machine
enum State {
  STATE_SPLASH,
  STATE_MENU,
  STATE_MULTI_MENU,
  STATE_TESTING,
  STATE_RESULTS,
  STATE_UPLOADING
};

enum TestMode {
  TEST_MODE_SINGLE,
  TEST_MODE_MULTI
};

enum MainMenuItem {
  MENU_MEASURE_ONE = 0,
  MENU_MEASURE_MULTI = 1,
  MENU_UPLOAD_CSV = 2
};

enum MultiMenuItem {
  MULTI_30S = 0,
  MULTI_1MIN = 1,
  MULTI_5MIN = 2,
  MULTI_10MIN = 3,
  MULTI_20MIN = 4,
  MULTI_BACK = 5
};

const int MAIN_MENU_ITEMS_COUNT = 3;
const int MULTI_MENU_ITEMS_COUNT = 6;

State currentState = STATE_SPLASH;
TestMode currentTestMode = TEST_MODE_SINGLE;

int selectedMainMenuItem = 0;
int selectedMultiMenuItem = 0;

unsigned long splashTimer = 0;
unsigned long testTimer = 0;

// Mantener resultados en pantalla X ms
unsigned long resultsTimer = 0;
const unsigned long RESULTS_HOLD_MS = 10000; // 10s
unsigned long batteryUiTimer = 0;
const unsigned long BATTERY_UI_REFRESH_MS = 3000;

// Variables para sesión de medidas
unsigned long nextMeasurementTime = 0;
int measurementCount = 0;
int targetMeasurements = 1;
bool measurementSessionActive = false;
bool measurementInProgress = false;

// ==================== WiFi Manager ====================
static bool wifiConnectWithManager() {
  WiFi.mode(WIFI_STA);

  WiFiManager wm;
  wm.setConfigPortalTimeout(WIFI_MANAGER_PORTAL_TIMEOUT_SEC);
  wm.setConnectTimeout(WIFI_MANAGER_CONNECT_TIMEOUT_SEC);

  bool ok = wm.autoConnect("TBEAM-COV", "12345678");

  if (!ok) {
    Serial.println("WiFiManager: NO conectado");
    return false;
  }

  Serial.print("WiFi OK. IP=");
  Serial.println(WiFi.localIP());
  return true;
}

// ==================== Helpers ====================
void showMainMenuScreen() {
  batteryManager.update(true);
  display.showMenu(
    selectedMainMenuItem,
    batteryManager.getVoltage(),
    batteryManager.getPercent(),
    batteryManager.isCharging()
  );
  batteryUiTimer = millis();
}

void showMultiMenuScreen() {
  batteryManager.update(true);
  display.showMultiMenu(
    selectedMultiMenuItem,
    batteryManager.getVoltage(),
    batteryManager.getPercent(),
    batteryManager.isCharging()
  );
  batteryUiTimer = millis();
}

void startSingleMeasurement() {
  currentTestMode = TEST_MODE_SINGLE;
  targetMeasurements = 1;
  measurementCount = 0;
  measurementSessionActive = true;
  measurementInProgress = false;
  nextMeasurementTime = millis();

  currentState = STATE_TESTING;

  batteryManager.update(true);
  display.showTesting(
    batteryManager.getVoltage(),
    batteryManager.getPercent(),
    batteryManager.isCharging()
  );

  Serial.println("Modo: medir 1 punto");
}

void startMultiMeasurementByDurationSeconds(int durationSeconds) {
  currentTestMode = TEST_MODE_MULTI;
  targetMeasurements = durationSeconds / (MULTI_MEASURE_INTERVAL_MS / 1000);

  if (targetMeasurements < 1) targetMeasurements = 1;

  measurementCount = 0;
  measurementSessionActive = true;
  measurementInProgress = false;
  nextMeasurementTime = millis();

  currentState = STATE_TESTING;

  batteryManager.update(true);
  display.showTesting(
    batteryManager.getVoltage(),
    batteryManager.getPercent(),
    batteryManager.isCharging()
  );

  Serial.printf("Modo: medir varios puntos. Duracion=%d s, intervalo=10 s, medidas=%d\n",
                durationSeconds, targetMeasurements);
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

  Wire.begin(I2C_SDA, I2C_SCL);

  if (!batteryManager.begin()) {
    Serial.println("WARN: no se pudo inicializar lectura de bateria");
  }

  if (!display.init()) {
    Serial.println("FATAL: No se pudo inicializar el display");
    delay(5000);
    ESP.restart();
  }

  if (!storageManager.begin()) {
    Serial.println("FATAL: LittleFS no inicia");
    display.showError("LittleFS FAIL");
    delay(5000);
    ESP.restart();
  }

  display.showSplash();
  splashTimer = millis();

  if (!loraManager.init()) {
    Serial.println("FATAL: No se pudo inicializar LoRa");
    display.showError("LoRa Init Failed");
    delay(5000);
    ESP.restart();
  }

  Serial.println("Sistema listo para pruebas de cobertura\n");
}

// ==================== GUARDAR + MOSTRAR ====================
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
  LongPressResult res = checkLongPress(lpState, lpCfg);
  bool shortPress = (res == PRESS_CANCELLED);
  bool longPress  = (res == LONG_PRESS_DETECTED);

  loraManager.update();
  gpsManager.update();

  switch (currentState) {

    case STATE_SPLASH:
      if (millis() - splashTimer > SPLASH_TIMEOUT_MS) {
        currentState = STATE_MENU;
        showMainMenuScreen();
        Serial.println("Menu principal listo");
      }
      break;

    case STATE_MENU:
      batteryManager.update();

      if (millis() - batteryUiTimer > BATTERY_UI_REFRESH_MS) {
        showMainMenuScreen();
      }

      if (shortPress) {
        selectedMainMenuItem = (selectedMainMenuItem + 1) % MAIN_MENU_ITEMS_COUNT;
        showMainMenuScreen();
        Serial.printf("Menu principal: opcion %d\n", selectedMainMenuItem);
      }

      if (longPress) {
        switch (selectedMainMenuItem) {
          case MENU_MEASURE_ONE:
            startSingleMeasurement();
            break;

          case MENU_MEASURE_MULTI:
            currentState = STATE_MULTI_MENU;
            selectedMultiMenuItem = 0;
            showMultiMenuScreen();
            Serial.println("Submenu medir varios");
            break;

          case MENU_UPLOAD_CSV:
            Serial.println("LONG PRESS -> Upload CSV");
            Serial.println("Conectando WiFi (WiFiManager)...");
            display.showStatus2("UPLOAD", "Conectando WiFi...");

            if (!wifiConnectWithManager()) {
              display.showError("WiFi FAIL");
              WiFi.mode(WIFI_OFF);
              delay(1500);
              currentState = STATE_MENU;
              showMainMenuScreen();
              break;
            }

            currentState = STATE_UPLOADING;
            break;
        }
      }
      break;

    case STATE_MULTI_MENU:
      batteryManager.update();

      if (millis() - batteryUiTimer > BATTERY_UI_REFRESH_MS) {
        showMultiMenuScreen();
      }

      if (shortPress) {
        selectedMultiMenuItem = (selectedMultiMenuItem + 1) % MULTI_MENU_ITEMS_COUNT;
        showMultiMenuScreen();
        Serial.printf("Submenu varios: opcion %d\n", selectedMultiMenuItem);
      }

      if (longPress) {
        switch (selectedMultiMenuItem) {
          case MULTI_30S:
            startMultiMeasurementByDurationSeconds(30);
            break;
          case MULTI_1MIN:
            startMultiMeasurementByDurationSeconds(60);
            break;
          case MULTI_5MIN:
            startMultiMeasurementByDurationSeconds(300);
            break;
          case MULTI_10MIN:
            startMultiMeasurementByDurationSeconds(600);
            break;
          case MULTI_20MIN:
            startMultiMeasurementByDurationSeconds(1200);
            break;
          case MULTI_BACK:
            currentState = STATE_MENU;
            showMainMenuScreen();
            Serial.println("Volver al menu principal");
            break;
        }
      }
      break;

    case STATE_UPLOADING: {
      display.showStatus2("Conectado", "Subiendo cov.csv...");
      Serial.println("Subiendo /cov.csv...");
      bool ok = uploader.uploadCSV();

      if (ok) {
        display.showStatus2("UPLOAD OK", "CSV enviado");
        Serial.println("UPLOAD OK");
        delay(UPLOAD_MSG_DELAY_MS);
      } else {
        Serial.println("UPLOAD FAIL");
        display.showError("UPLOAD FAIL");
        delay(UPLOAD_MSG_DELAY_MS);
      }

      WiFi.mode(WIFI_OFF);

      currentState = STATE_MENU;
      showMainMenuScreen();
      break;
    }

    case STATE_TESTING: {
      if (measurementSessionActive &&
          !measurementInProgress &&
          measurementCount < targetMeasurements &&
          millis() >= nextMeasurementTime) {

        Serial.printf("Iniciando medida %d de %d...\n", measurementCount + 1, targetMeasurements);

        batteryManager.update(true);
        display.showTesting(
          batteryManager.getVoltage(),
          batteryManager.getPercent(),
          batteryManager.isCharging()
        );

        loraManager.resetTest();
        loraManager.startCoverageTest();
        testTimer = millis();
        measurementInProgress = true;
      }

      if (measurementInProgress && loraManager.isTestingComplete()) {
        LoRaTestResults results = loraManager.getResults();
        saveAndShowResults(results, true);

        measurementCount++;
        measurementInProgress = false;
        loraManager.resetTest();

        Serial.printf("Medida %d/%d guardada\n", measurementCount, targetMeasurements);

        if (measurementCount >= targetMeasurements) {
          measurementSessionActive = false;
          resultsTimer = millis();
          currentState = STATE_RESULTS;
          Serial.println("Sesion de medidas completada");
        } else {
          nextMeasurementTime = millis() + MULTI_MEASURE_INTERVAL_MS;
          Serial.printf("Siguiente medida en %lu ms\n", MULTI_MEASURE_INTERVAL_MS);
        }
      }

      if (measurementInProgress &&
          (millis() - testTimer > TEST_TIMEOUT_MS)) {

        LoRaTestResults dummyResults;
        saveAndShowResults(dummyResults, false);

        measurementCount++;
        measurementInProgress = false;
        loraManager.resetTest();

        Serial.printf("Timeout en medida %d/%d\n", measurementCount, targetMeasurements);

        if (measurementCount >= targetMeasurements) {
          measurementSessionActive = false;
          resultsTimer = millis();
          currentState = STATE_RESULTS;
          Serial.println("Sesion finalizada tras timeout");
        } else {
          nextMeasurementTime = millis() + MULTI_MEASURE_INTERVAL_MS;
          Serial.printf("Siguiente medida en %lu ms\n", MULTI_MEASURE_INTERVAL_MS);
        }
      }

      break;
    }

    case STATE_RESULTS:
      if (millis() - resultsTimer > RESULTS_HOLD_MS) {
        loraManager.resetTest();
        measurementSessionActive = false;
        measurementInProgress = false;
        currentState = STATE_MENU;
        showMainMenuScreen();
        Serial.println("\nVuelta al menu principal");
      }

      if (shortPress) {
        loraManager.resetTest();
        measurementSessionActive = false;
        measurementInProgress = false;
        currentState = STATE_MENU;
        showMainMenuScreen();
        Serial.println("\nVuelta al menu principal");
      }
      break;
  }

  delay(10);
}