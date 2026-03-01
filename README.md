# LoRa Coverage Meter - T-Beam v1.2

Dispositivo portátil para medir cobertura LoRa con LilyGO T-Beam v1.2.
Mide RSSI y SNR y muestra resultados en OLED.

## Resumen

- **Hardware**: ESP32, SX1262 868 MHz, OLED 0.96".
- **Software**: PlatformIO/Arduino, LMIC-Arduino, Adafruit SSD1306/GFX.
- **Funcionalidad**: botón para iniciar pruebas, métricas en pantalla, serial para depuración.

## Estructura

```
include/      configuración e interfaces
src/          código principal (main.cpp, display, lora_manager)
lib/          dependencias (LMIC-Arduino)
platformio.ini
```

## Configuración y uso

1. Clona el repo y abre en PlatformIO.
2. `pio run -e lilygo-t-beam` para compilar y `-t upload` para cargar.
3. Presiona el botón en el T‑Beam para iniciar una medición.
4. Observa RSSI/SNR en pantalla; el serial muestra detalles a 115200 baud.

## Métricas

- RSSI: <-80 dBm excelente, <-100 dBm buena, <-120 dBm aceptable.
- SNR: >10 dB excelente, 5‑10 dB aceptable, <5 dB pobre.

Más detalles en el código.
