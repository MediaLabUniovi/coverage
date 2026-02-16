#ifndef CONFIG_H
#define CONFIG_H

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

// TTN Keys (usar valores dummy para prueba simple)
#define APPEUI_DUMMY { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define DEVEUI_DUMMY { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define APPKEY_DUMMY { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }

// Test Parameters
#define TEST_TIMEOUT 10000       // Timeout en ms

// Authorized Nodes (para filtrar cobertura)
// Coloca aquí los DevEUI de tus nodos autorizados
#define AUTHORIZED_NODES {                                    \
    {0xCC, 0xFC, 0x06, 0xD0, 0x7E, 0xD5, 0xB3, 0x70},       \
}
#define AUTHORIZED_NODES_COUNT 1

#endif


