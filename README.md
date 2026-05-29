# Examen 3er EDC 

**Sistemas Embebidos II**
**Variante C: Monitoreo de Luminosidad**
Comunicación I²C entre Raspberry Pi y ESP32
Estudiante: Florencia Frigerio Arízaga

---

## 📌 Resumen del proyecto

Sistema embebido basado en **coprocesamiento** entre una Raspberry Pi y un microcontrolador ESP32, conectados por bus **I²C**. El ESP32 adquiere la luminosidad ambiental con un sensor LDR y la entrega bajo demanda a la Raspberry Pi, que la registra, clasifica y grafica.

El sistema cumple con la **arquitectura de coprocesamiento** exigida por la consigna: la Raspberry Pi **nunca** lee el sensor de forma directa; la totalidad de la adquisición y preprocesamiento ocurre en el ESP32.

```
                          I²C    
│   LDR   ──ADC─►│    ESP32     │◄────────►│  Raspberry Pi  │
│ (3.3 V)        │  (esclavo)   │          │   (maestro)    │
              
     mide          adquiere y prepara          registra, clasifica y grafica
```

---

## 🧱 Arquitectura del sistema

### División de responsabilidades

| Componente | Rol | Responsabilidades |

| **Sensor LDR** | Transductor | Convierte la luz incidente en una tensión analógica |
| **ESP32** | Esclavo I²C | Lee el ADC, filtra, promedia, empaqueta los datos y los entrega bajo demanda |
| **Raspberry Pi** | Maestro I²C | Solicita datos periódicamente, los decodifica, agrega timestamp, clasifica, registra y grafica |

### Diagrama de conexiones

| Sensor LDR | ESP32 |
|---|---|
| VCC | 3V3 |
| GND | GND |
| AO  | GPIO34 (ADC1 canal 6) |

| ESP32 | Raspberry Pi (pin físico) |
|---|---|
| GPIO21 (SDA) | Pin 3 — SDA1 |
| GPIO22 (SCL) | Pin 5 — SCL1 |
| GND | Pin 6 — GND (referencia común) |

---

## 📂 Estructura del repositorio

```
.
├── README.md                    
├── esp32/                        
│   ├── main.c
│   ├── inc/
│   │   ├── parametros.h
│   │   ├── sensor_luz.h
│   │   └── bus_i2c.h
│   ├── src/
│   │   ├── sensor_luz.c
│   │   └── bus_i2c.c
│   └── CMakeLists.txt
├── raspberry_pi/
│   └── monitor_luz.py            
├── datos_generados/
│   ├── registro.csv              
│   └── grafica.png              
└── evidencias/
    ├── i2cdetect.png
    ├── montaje_general.jpg
    ├── monitor_esp32.png
    └── terminal_pi.png
```

---

## ⚙️ Especificaciones técnicas

| Parámetro | Valor |

| Framework ESP32 | ESP-IDF v6.0 (Espressif IDE 4.2.0) |
| Lenguaje Pi | Python 3 |
| Dirección I²C del esclavo | `0x08` |
| Muestras promediadas por lectura | 16 |
| Periodo de adquisición en ESP32 | 200 ms |
| Intervalo de muestreo de Pi | 2 s |
| Muestras mínimas por corrida | 30 |

---

## 🔁 Flujo de datos

### En el ESP32

1. Una **tarea FreeRTOS** lee el sensor cada 200 ms.
2. Por cada lectura: se toman 16 muestras, se descartan las que caen fuera del rango válido y se promedian las restantes (procesamiento básico).
3. Si todas las muestras fallan, se conserva el último valor válido (requisito del examen).
4. El valor (entero 0–4095) se publica en una variable compartida.
5. Cuando la Pi envía una solicitud por I²C, un **callback en contexto ISR** despierta una tarea que empaqueta el valor en **2 bytes** y los escribe en el bus.

### En la Raspberry Pi

1. El script abre el bus I²C-1 con `smbus2`.
2. Cada 2 segundos solicita 2 bytes al esclavo `0x08`.
3. Reconstruye el valor: `valor = (byte_alto << 8) | byte_bajo`.
4. Descarta lecturas fuera del rango ADC.
5. Agrega timestamp, clasifica el estado y escribe la fila en `registro.csv` en **modo append**.
6. Al final de cada corrida regenera `grafica.png` leyendo el **CSV completo** (todas las corridas anteriores acumuladas).

---

## 🌗 Clasificación del estado

El sensor LDR usado tiene **lógica inversa**: a mayor cantidad de luz, **menor** valor del ADC (la resistencia disminuye con la luz, lo que altera el divisor resistivo).

Los umbrales se calibraron empíricamente con tres mediciones del sensor real:

| Condición | Lectura ADC observada |
|---|:---:|
| Sensor tapado | ≈ 3200 |
| Luz ambiente | 800 – 1000 |
| Linterna directa | ≈ 0 |

Esto define los umbrales:

| Rango ADC | Estado |
|---|---|
| `valor ≥ 2000` | **OSCURO** |
| `600 ≤ valor < 2000` | **NORMAL** |
| `valor < 600` | **MUY_ILUMINADO** |

---

## 📑 Archivos generados

### `registro.csv` (modo append)

```csv
fecha_hora,luz,estado
2026-05-29 00:20:32,2456,OSCURO
2026-05-29 00:20:34,1573,NORMAL
2026-05-29 00:20:36,87,MUY_ILUMINADO
...
```

Cada corrida del script agrega 30 muestras al final, sin perder las anteriores. La línea de encabezado se escribe solo la primera vez. Esto permite al docente ver, a partir de los timestamps, **cuántas veces y a qué horas** se ejecutó el sistema.

### `grafica.png`

Se regenera leyendo el CSV completo. Cada ejecución del script actualiza la imagen incluyendo todas las muestras acumuladas. La gráfica incluye:

- Título con la cantidad de muestras
- Ejes etiquetados con unidades
- Líneas punteadas en los umbrales `OSCURO` y `MUY_ILUMINADO`
- Eje Y invertido (arriba = más luz, abajo = más oscuro)

---

## 🛠️ Cómo reproducir el proyecto

### Requisitos

| Recurso | Versión / detalle |
|---|---|
| Raspberry Pi | Cualquiera con I²C (testeada en Pi con Raspbian Trixie) |
| ESP32 | DevKit WROOM-32S (38 pines) |
| Sensor | Módulo LDR de 4 pines (con LM393) |
| Framework | ESP-IDF v6.0 |
| Driver USB-Serial | Silicon Labs CP210x |

### Lado Raspberry Pi

```bash
# Habilitar I2C
sudo raspi-config              # Interface Options → I2C → Enable

# Dependencias
sudo apt update
sudo apt install -y i2c-tools python3-smbus2 python3-matplotlib python3-pandas

# Verificar que el ESP32 está en el bus (debe aparecer 0x08)
i2cdetect -y 1

# Ejecutar el sistema
python3 raspberry_pi/monitor_luz.py
```

### Lado ESP32

1. Instalar **Espressif IDE** con ESP-IDF v6.0.
2. Abrir el proyecto desde la carpeta `esp32/`.
3. Configurar el target: **Set Target → esp32**.
4. Conectar el ESP32 por USB y verificar el puerto COM.
5. Build (🔨) y Run (▶) con la "Run Configuration" tipo *ESP-IDF Application*.

---

## ✅ Verificación del coprocesamiento

Para comprobar que la Raspberry Pi **no** lee el sensor directamente:

| Prueba | Resultado esperado |
|---|---|
| `i2cdetect -y 1` con el ESP32 conectado | Aparece `0x08` |
| `i2cdetect -y 1` con el ESP32 desconectado | Tabla vacía |
| Desconectar el ESP32 durante `monitor_luz.py` | La Pi muestra `OSError: [Errno 121] Remote I/O error` |
| Reconectar el ESP32 | La comunicación se restablece automáticamente |

El sensor está conectado **exclusivamente al ESP32**. Sin el microcontrolador, la Pi no obtiene datos.


| Requisito | Valor en el código |
|---|---|
| Mínimo 30 muestras válidas | 30 por corrida (`MUESTRAS_TOTAL = 30`) |
| Intervalo de muestreo 1–5 s | 2 s (`INTERVALO_S = 2`) |

---

## 🚧 Problemas encontrados y soluciones

| # | Problema | Diagnóstico | Solución |
|:---:|---|---|---|
| 1 | El COM del ESP32 no aparecía en Windows | Driver CP2102 no instalado | Instalación del driver oficial Silicon Labs CP210x |
| 2 | `i2cdetect` mostraba todas las direcciones ocupadas | SDA y SCL invertidos en el cableado | Intercambio físico de los cables SDA/SCL |
| 3 | El driver I²C esclavo "v1" no existe en ESP-IDF v6.0 | API removida según migración v5.4 → v6.0 | Adopción del driver nuevo basado en callbacks (`driver/i2c_slave.h`) |
| 5 | Tendencia a recibir el mismo valor cuando la luz es estable | Promedio sobre 16 muestras (esperado) | Ningún cambio necesario; el sistema funciona como diseñado |

---

## 🎥 Evidencias

| Archivo | Descripción |
|---|---|
| `evidencias/montaje_general.jpg` | Foto del montaje físico completo |
| `evidencias/i2cdetect.png` | Captura mostrando `0x08` en el bus |
| `evidencias/monitor_esp32.png` | Captura del Serial Monitor del ESP32 |
| `evidencias/terminal_pi.png` | Captura del script de la Pi ejecutándose |
| `datos_generados/registro.csv` | CSV con todas las muestras acumuladas |
| `datos_generados/grafica.png` | Gráfica histórica del sistema |

---

## 📚 Referencias técnicas

- [ESP-IDF v6.0 — I²C API](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/i2c.html)
- [ESP-IDF v6.0 — ADC Oneshot](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/adc/adc_oneshot.html)
- [Migration guide ESP-IDF 5.x → 6.0](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/migration-guides/release-6.x/6.0/peripherals.html)
- [smbus2 — Documentation](https://pypi.org/project/smbus2/)

---

Mayo 2026
