# LoRa Coverage Meter - T-Beam v1.2

## Descripción General

**LoRa Coverage Meter** es un medidor de cobertura LoRa portable basado en el dispositivo **LilyGO T-Beam v1.2**. Este proyecto permite evaluar y monitorear la calidad de la señal LoRa en diferentes ubicaciones geográficas mediante métricas clave como RSSI (Indicador de Fuerza de Señal Recibida) y SNR (Relación Señal-Ruido).

El dispositivo actúa como una **sonda de monitoreo** que mide la cobertura LoRa disponible en un área específica, ideal para:
- Mapeo de cobertura de redes LoRaWAN
- Validación de ubicaciones para desplegar nodos LoRa
- Diagnóstico de problemas de conectividad
- Optimización de colocación de gateways

---

## Características Principales

### Hardware
- **Procesador**: ESP32 (LilyGO T-Beam v1.2)
- **Módulo LoRa**: SX1262 (frecuencia 868 MHz para Europa)
- **Display**: OLED 0.96" (128x64 píxeles)
- **Botón**: Integrado en el T-Beam para iniciar pruebas
- **GPS**: Disponible en el T-Beam para geolocalización (extensible)

### Software
- **Framework**: Arduino + PlatformIO
- **Librería LoRa**: LMIC-Arduino (LoRaMac-in-C)
- **Librería Display**: Adafruit SSD1306 + GFX
- **Formato**: Configuración por #define para fácil personalización

### Funcionalidades
- Pruebas de cobertura LoRa con un botón
- Visualización en tiempo real de métricas en display OLED
- Medición de RSSI (Received Signal Strength Indicator)
- Cálculo de SNR (Signal-to-Noise Ratio)
- Evaluación de calidad de señal (Excelente/Buena/Aceptable/Pobre)
- Filtro de nodos autorizados por DevEUI
- Salida por puerto serial para depuración
- Máquina de estados para gestión del flujo de pruebas

---

## Arquitectura del Proyecto

### Estructura de Carpetas

```
Probar cobertura/
├── include/
│   ├── config.h              # Configuración centralizada (GPIO, LoRa, nodos)
│   ├── display.h             # Interfaz DisplayManager
│   └── lora_manager.h        # Interfaz LoRaManager
├── lib/
│   └── LMIC-Arduino/         # Librería LoRaMac-in-C
├── src/
│   ├── main.cpp              # Máquina de estados principal
│   ├── display.cpp           # Implementación del display
│   ├── lora_manager.cpp      # Gestión de LoRa y pruebas
│   └── ...
├── platformio.ini            # Configuración de build
└── README.md                 # Este archivo
```

## Métricas de Cobertura

### RSSI (Indicador de Fuerza de Señal Recibida)
Mide la potencia recibida en dBm. Valores típicos:
- **-30 a -50 dBm**: Señal excelente
- **-50 a -80 dBm**: Buena señal
- **-80 a -100 dBm**: Señal aceptable
- **< -100 dBm**: Señal débil/pobre

**Interpretación en el proyecto:**
```
> -80 dBm   → Excelente
> -100 dBm  → Buena
> -120 dBm  → Aceptable
< -120 dBm  → Pobre
```

### SNR (Relación Señal-Ruido)
Mide la diferencia entre la potencia de señal y el ruido en dB:
- **SNR > 10 dB**: Excelente (bajo ruido)
- **SNR 5-10 dB**: Aceptable
- **SNR < 5 dB**: Pobre (señal interferida)

---

## Configuración

### 1. Hardware
Asegurate que tienes:
- LilyGO T-Beam v1.2 (no v1.1)
- Display OLED 0.96" I2C (dirección 0x3C)
- Cable USB para programación y depuración

### 2. Software - Instalación

#### Opción A: PlatformIO (Recomendado)
```bash
# Clonar o descargar el proyecto
cd "Probar cobertura"

# Compilar
pio run -e lilygo-t-beam

# Cargar firmware
pio run -e lilygo-t-beam -t upload

# Monitorear serial
pio device monitor -e lilygo-t-beam -b 115200
```

#### Opción B: Arduino IDE
1. Instalar placa ESP32: Gestor de tarjetas → buscar "esp32"
2. Instalar librerías:
   - Adafruit SSD1306
   - Adafruit GFX Library
   - ArduinoJson
   - LMIC-Arduino
3. Cargar `main.ino`
4. Compilar y descargar

<<<<<<< HEAD


---

## Uso (rápido)

1. Encender el T-Beam → aparece el estado "READY" en la pantalla.
2. Presionar el botón para iniciar la prueba.
3. El dispositivo transmite un paquete y espera confirmación del gateway (ACK o downlink).
4. Si la prueba se confirma, se muestran RSSI, SNR y el contador de paquetes; de lo contrario la medición no se contabiliza.

Presionar el botón para repetir la prueba.
=======

## Uso

### Flujo Básico

1. **Encender el T-Beam**
   - Se muestra splash screen (3 segundos)
   - Display muestra "Status: READY"

2. **Presionar el Botón**
   - Se inicia la prueba de cobertura
   - Display muestra "Status: TESTING..."

3. **Esperar Resultado**
   - El dispositivo transmite un paquete LoRa
   - Captura métricas de la transmisión
   - Display muestra:
     - **RSSI**: Potencia recibida
     - **SNR**: Relación señal-ruido
     - **Packets**: Contador de paquetes
     - **Calidad**: Interpretación visual

---

## Mapa Mental de la Comunicación

```
Dispositivo T-Beam                    Nodos/Gateway
──────────────────────────────────────────────────

[Button Press]
    ↓
[LoRaManager::startCoverageTest()]
    ↓
[LMIC transmite paquete "TEST" en SF12]  ──→ (Transmisión LoRa)
    ↓
[LMIC captura RSSI y SNR]
    ↓
[isNodeAuthorized()?]
    ├─ Sí → Display muestra resultados
    └─ No → Paquete ignorado
    ↓
[DisplayManager::showResults()]
```

---

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
<<<<<<< HEAD
- Las mediciones se consideran válidas solo si el gateway confirma recepción (ACK o downlink).
- No se transmiten datos personales por diseño.
- Respeta las políticas del servidor/gateway y el duty-cycle de la banda ISM 868 MHz.
=======
- Este dispositivo **solo mide cobertura**, no se conecta a TTN
- Los DevEUI autorizados actúan como **filtro local**
- No se transmiten datos personales
- El dispositivo no se autentica contra servidores
- Úsalo responsablemente en la banda ISM 868 MHz
>>>>>>> origin/main

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
=======
**Última actualización**: 16 de febrero 2026
---

## Contribuciones

Las contribuciones son bienvenidas. Para reportar bugs o sugerencias:
1. Verifica el serial output con `-DDEBUG`
2. Incluye valores de RSSI/SNR problemáticos
3. Especifica versión de T-Beam y librerías usadas
