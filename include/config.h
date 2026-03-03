#ifndef CONFIG_H
#define CONFIG_H

#include <lmic.h>



#define UP_HOST "medialab-uniovi.es"
#define UP_PORT 443
#define UP_PATH "/bike_signal/upload.php"

// T-Beam GPIO Configuration (from utilities.h)
#define BUTTON_PIN 38            // Botón incorporado del T-Beam
#define I2C_SDA 21               // SDA para I2C
#define I2C_SCL 22               // SCL para I2C

// Display OLED 0.96" Configuration
#define SCREEN_WIDTH 128         // Ancho del display en pixels
#define SCREEN_HEIGHT 64         // Alto del display en pixels
#define OLED_RESET -1            // Reset pin (no usado)
#define SCREEN_ADDRESS 0x3C      // Dirección I2C del display (típicamente 0x3C)

// LoRa Configuration (LMIC - pines de T-Beam)
#define LORA_SCK 5               // CLK
#define LORA_MISO 19             // MISO
#define LORA_MOSI 27             // MOSI
#define LORA_CS 18               // CS (NSS)
#define LORA_RST 23              // Reset
#define LORA_DIO0 26             // DIO0 (INT)
#define LORA_DIO1 33             // DIO1
#define LORA_DIO2 32             // DIO2

// Frequency and region
#define LORA_FREQ 868E6          // 868 MHz para Europa
#define USE_EU868

#define GPS_RX   34
#define GPS_TX   12
#define GPS_BAUD 9600


    // Application EUI (AppEUI) - 8 bytes en formato LSB
    static const u1_t PROGMEM APPEUI[8] = {
        0x00, 0x00, 0x00, 0x00,  // TODO: Reemplaza con valores reales
        0x00, 0x00, 0x00, 0x00
    };
    
    // Device EUI (DevEUI) - 8 bytes en formato LSB
    static const u1_t PROGMEM DEVEUI[8] = {
       0x48, 0x5D, 0x07, 0xD0, 0x7E, 0xD5, 0xB3, 0x70
    };
    
    // Application Key (AppKey) - 16 bytes en formato MSB
    static const u1_t PROGMEM APPKEY[16] = {
    0x0A, 0x6C, 0x1D, 0x3A, 0xC3, 0xF1, 0xEC, 0x8B, 0x44, 0xA2, 0x56, 0x93, 0xFF, 0x03, 0xEF, 0xFD
    };
// Test Parameters
#define TEST_TIMEOUT 10000       // Timeout en ms

// Authorized Nodes (para filtrar cobertura)
// Coloca aquí los DevEUI de tus nodos autorizados
#define AUTHORIZED_NODES {                                    \
    {0xCC, 0xFC, 0x06, 0xD0, 0x7E, 0xD5, 0xB3, 0x70},       \
}
#define AUTHORIZED_NODES_COUNT 1

#endif


