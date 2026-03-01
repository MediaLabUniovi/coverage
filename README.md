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

### Componentes Principales

<<<<<<< HEAD
#### 1. `config.h` - Centro de Configuración
Define pines y parámetros básicos del sistema (pines I2C/SPI, frecuencia LoRa, timeouts).

#### 2. **LoRaManager** - Gestión de Comunicación LoRa
- Inicializa LMIC y el radio
- Envía paquetes de prueba y captura métricas (RSSI/SNR)
=======
#### 1. **config.h** - Centro de Configuración
Define todos los parámetros del sistema:

```cpp
// Pines del T-Beam v1.2
#define BUTTON_PIN 38            // Botón
#define I2C_SDA 21, I2C_SCL 22   // Display OLED
#define LORA_SCK 5, LORA_MISO 19, etc.  // SPI para LoRa

// Nodos autorizados (filtro)
#define AUTHORIZED_NODES { {0xCC, 0xFC, 0x06, 0xD0, 0x7E, 0xD5, 0xB3, 0x70} }
#define AUTHORIZED_NODES_COUNT 1
```

#### 2. **LoRaManager** - Gestión de Comunicación LoRa
- Inicializa el módulo SX1262 mediante LMIC
- Transmite paquetes de prueba
- Captura métricas de RSSI y SNR
- Valida nodos autorizados mediante DevEUI
>>>>>>> origin/main

```cpp
class LoRaManager {
    void startCoverageTest();      // Inicia prueba de cobertura
    void update();                 // Actualiza estado LMIC
    LoRaTestResults getResults();  // Obtiene RSSI, SNR, contador
<<<<<<< HEAD
=======
    bool isNodeAuthorized();       // Valida DevEUI del nodo
>>>>>>> origin/main
};
```

#### 3. **DisplayManager** - Interfaz Visual
<<<<<<< HEAD
- Splash, estado listo, prueba en curso y resultados

#### 4. **main.cpp** - Máquina de Estados
Orquesta el flujo del dispositivo:
=======
- Muestra splash screen al arrancar
- Pantalla de espera de interacción
- Indicador de prueba en progreso
- Resultados con interpretación de calidad

#### 4. **main.cpp** - Máquina de Estados
Orquesta el flujo completo del dispositivo:
>>>>>>> origin/main

```
STATE_SPLASH (3s) → STATE_READY (espera botón) → STATE_TESTING (mide) → STATE_RESULTS (muestra datos)
```

---

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
### 3. Configuración de Nodos Autorizados

En [include/config.h](include/config.h), actualizar:

```cpp
#define AUTHORIZED_NODES {                              \
    {0xCC, 0xFC, 0x06, 0xD0, 0x7E, 0xD5, 0xB3, 0x70}, \
    {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11}, \
    /* Agregar más DevEUI en formato little-endian */ \
}
#define AUTHORIZED_NODES_COUNT 2
```

**Nota**: Los DevEUI deben estar en formato **little-endian** (menos significativo primero). Si tu nodo usa `70 B3 D5 7E D0 06 FC CC`, invierte el orden en la configuración.

---

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

4. **Nuevas Pruebas**
   - Presionar botón nuevamente para repetir
>>>>>>> origin/main

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
<<<<<<< HEAD
Paquete transmitido (confirmed), esperando ACK/downlink...
=======
Paquete transmitido, esperando resultado...
>>>>>>> origin/main

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

<<<<<<< HEAD
=======
**Última actualización**: 16 de febrero de 2026

>>>>>>> origin/main
---

## Contribuciones

Las contribuciones son bienvenidas. Para reportar bugs o sugerencias:
1. Verifica el serial output con `-DDEBUG`
2. Incluye valores de RSSI/SNR problemáticos
3. Especifica versión de T-Beam y librerías usadas
