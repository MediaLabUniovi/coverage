
# LoRa Coverage Meter - T-Beam v1.2


Herramienta portátil para medir la cobertura LoRa con un LilyGO T-Beam v1.2.
La medición de cobertura **solo se considera válida si el gateway responde con un ACK o un paquete de downlink** tras el envío del paquete confirmado. Si no hay respuesta, la medición se descarta.

## Rápido resumen de uso

- Compila con PlatformIO (`pio run -e lilygo-t-beam`).
- Carga al T‑Beam (`pio run -e lilygo-t-beam -t upload`).
- Pulsa el botón del T‑Beam para iniciar una prueba.
- Observa resultados en el OLED y en el puerto serial (115200 baud).

## Métricas de cobertura

- RSSI: <-80 dBm excelente, <-100 dBm buena, <-120 dBm aceptable.
- SNR: >10 dB excelente, 5 – 10 dB aceptable, <5 dB pobre.

## Estructura

```
include/   configuraciones e interfaces
src/       implementación (main, lora_manager, display)
lib/       dependencia LMIC-Arduino
platformio.ini
README.md
```

Más detalles en el código fuente (especialmente `lora_manager.cpp`).
4. Si la prueba se confirma, se muestran RSSI, SNR y el contador de paquetes; de lo contrario la medición no se contabiliza.

Presionar el botón para repetir la prueba.

### Depuración Serial

Abre la consola serial a 115200 baud:

```
==== LoRa Coverage Test - T-Beam ====

Botón inicializado en GPIO 38
Display OLED 0.96" inicializado correctamente
Inicializando LMIC-Arduino...
LMIC inicializado correctamente
Frecuencia: 868 MHz (EU868)
Spreading Factor: 12
Sistema listo para pruebas de cobertura

=== Iniciando prueba de cobertura LoRa ===
Paquete transmitido (confirmed), esperando ACK/downlink...

=== Resultados de Cobertura ===
RSSI: -95 dBm
SNR: 7 dB
Packets: 1
```

---

## Especificaciones Técnicas

### Parámetros LoRa

| Parámetro | Valor | Descripción |
|-----------|-------|-------------|
| Frecuencia | 868 MHz | Banda ISM Europa (EU868) |
| Spreading Factor | 12 | SF máximo para máximo rango |
| Transmit Power | 14 dBm | Potencia de transmisión |
| Bandwidth | 125 kHz | Ancho de banda estándar |
| Coding Rate | 4/5 | Tasa de codificación |
| Canales | Solo canal 0 | Optimizado para pruebas |

### Tiempos

| Elemento | Tiempo |
|----------|--------|
| Splash Screen | 3 segundos |
| Timeout de Prueba | 10 segundos |
| Debounce Botón | 200 ms |
| Refresco Display | Continuo |

### Consumo de Potencia (Estimado)

- Espera (display + CPU): ~50 mA
- Transmisión LoRa: ~500 mA pico
- Promedio en operación: ~100-150 mA

---
## Mapa mental de la comunicación

```
┌────────────┐      ┌─────────────┐      ┌─────────────┐
│  Usuario   │      │   T-Beam    │      │   Gateway   │
└─────┬──────┘      └─────┬───────┘      └─────┬───────┘
    │  (pulsa botón)     │                  │
    │───────────────────▶│                  │
    │                    │-- uplink confirmado -->│
    │                    │                  │
    │                    │<-- downlink/ACK --│
    │                    │                  │
    │                    │-- mide RSSI/SNR --│
    │                    │                  │
    │                    │-- solo si recibe ACK/downlink, la medición se considera válida --│
    │                    │                  │
    │                    │-- muestra resultados en OLED/serial --▶│
    │◀───────────────────│                  │
```

## Troubleshooting

### "FATAL: Display OLED no encontrado"
- Verificar conexión I2C (pines 21/22)
- Comprobar dirección I2C: usar scanner I2C
- Revisar voltaje en display (3.3V)

### "FATAL: No se pudo inicializar LoRa"
- Verificar pines SPI (5, 19, 27, 18)
- Comprobar reset pin (23)
- Revisar antena LoRa conectada
- Validar frecuencia (868 MHz para EU868)

### Sin resultados después de presionar botón
- Timeout puede estar activado (10 segundos)
- Verificar serial para eventos LMIC
- Revisar si hay gateways LoRa nearby
- Comprobar configuración de canales

### Valores RSSI anómalos
- Asegurar que antena está correctamente conectada
- Verificar que no hay interferencia de 2.4 GHz
- Comprobar orientación y altura del dispositivo

---

## Seguridad y Privacidad

**Nota Importante**: 
- Las mediciones se consideran válidas solo si el gateway confirma recepción (ACK o downlink).
- No se transmiten datos personales por diseño.
- Respeta las políticas del servidor/gateway y el duty-cycle de la banda ISM 868 MHz.

---

## Referencias

- [LilyGO T-Beam GitHub](https://github.com/LilyGO/LilyGO-T-Beam)
- [LMIC-Arduino Documentation](https://github.com/mcci-catena/arduino-lmic)
- [LoRa Specification](https://lora-alliance.org/)
- [Adafruit SSD1306 Library](https://github.com/adafruit/Adafruit_SSD1306)

---

## Licencia

Este proyecto está disponible bajo licencia MIT. Ver LICENSE para detalles.

---

## Autor

Proyecto desarrollado como herramienta de diagnóstico y validación de cobertura LoRa en redes locales.

---

## Contribuciones

Las contribuciones son bienvenidas. Para reportar bugs o sugerencias:
1. Verifica el serial output con `-DDEBUG`
2. Incluye valores de RSSI/SNR problemáticos
3. Especifica versión de T-Beam y librerías usadas
