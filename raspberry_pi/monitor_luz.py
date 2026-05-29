import smbus2
import time
import csv
import os
from datetime import datetime
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.dates as mdates

DIR_ESP32        = 0x08
BUS_I2C          = 1
INTERVALO_S      = 2
MUESTRAS_TOTAL   = 30

UMBRAL_OSCURO          = 2000
UMBRAL_MUY_ILUMINADO   = 600

ARCHIVO_CSV = "registro.csv"
ARCHIVO_PNG = "grafica.png"


def clasificar(valor):
    if valor >= UMBRAL_OSCURO:
        return "OSCURO"
    elif valor < UMBRAL_MUY_ILUMINADO:
        return "MUY_ILUMINADO"
    else:
        return "NORMAL"


def leer_esp32(bus):
    datos = bus.read_i2c_block_data(DIR_ESP32, 0, 2)
    valor = (datos[0] << 8) | datos[1]
    return valor


def es_valido(valor):
    return 0 <= valor <= 4095


def adquirir_muestras(bus):
    archivo_existe = os.path.exists(ARCHIVO_CSV)
    modo = "a" if archivo_existe else "w"

    with open(ARCHIVO_CSV, mode=modo, newline="") as f:
        writer = csv.writer(f)
        if not archivo_existe:
            writer.writerow(["fecha_hora", "luz", "estado"])
            print(f"Archivo nuevo creado: {ARCHIVO_CSV}")
        else:
            print(f"Archivo existente, agregando muestras a: {ARCHIVO_CSV}")

        muestras_validas = 0
        while muestras_validas < MUESTRAS_TOTAL:
            try:
                valor = leer_esp32(bus)

                if not es_valido(valor):
                    print(f"  [!] Lectura invalida descartada: {valor}")
                    time.sleep(INTERVALO_S)
                    continue

                ahora = datetime.now()
                fecha_hora_str = ahora.strftime("%Y-%m-%d %H:%M:%S")
                estado = clasificar(valor)

                writer.writerow([fecha_hora_str, valor, estado])
                f.flush()

                muestras_validas += 1
                print(f"  [{muestras_validas:02d}/{MUESTRAS_TOTAL}]  "
                      f"{fecha_hora_str}  luz={valor:4d}  estado={estado}")

            except OSError as e:
                print(f"  [!] Error I2C: {e}. Reintentando...")
            except Exception as e:
                print(f"  [!] Error: {e}")

            time.sleep(INTERVALO_S)


def generar_grafica():
    df = pd.read_csv(ARCHIVO_CSV)
    df["fecha_hora"] = pd.to_datetime(df["fecha_hora"])

    fig, ax = plt.subplots(figsize=(12, 5))
    ax.plot(df["fecha_hora"], df["luz"],
            marker="o", linestyle="-",
            color="#1f77b4", linewidth=1.2, markersize=3,
            label="Nivel de luz (LDR)")

    ax.axhline(UMBRAL_OSCURO, color="#555", linestyle="--",
               linewidth=0.8, label=f"Umbral OSCURO ({UMBRAL_OSCURO})")
    ax.axhline(UMBRAL_MUY_ILUMINADO, color="#999", linestyle="--",
               linewidth=0.8, label=f"Umbral MUY_ILUMINADO ({UMBRAL_MUY_ILUMINADO})")

    ax.set_title(f"Monitoreo de luminosidad - Historial acumulado ({len(df)} muestras)")
    ax.set_xlabel("Fecha y hora")
    ax.set_ylabel("Nivel de luz (valor ADC, 0-4095)")
    ax.xaxis.set_major_formatter(mdates.DateFormatter("%d/%m %H:%M:%S"))
    fig.autofmt_xdate()
    ax.grid(True, linestyle=":", alpha=0.6)
    ax.legend(loc="best")
    ax.invert_yaxis()

    plt.tight_layout()
    plt.savefig(ARCHIVO_PNG, dpi=120)
    plt.close(fig)
    print(f"Grafica regenerada: {ARCHIVO_PNG} ({len(df)} muestras totales)")


def main():
    print("=" * 55)
    print(" Monitoreo de luminosidad - Coprocesamiento ESP32+Pi")
    print("=" * 55)
    print(f"Direccion I2C del esclavo: 0x{DIR_ESP32:02X}")
    print(f"Muestras a registrar en esta corrida: {MUESTRAS_TOTAL}")
    print(f"Intervalo de muestreo: {INTERVALO_S} s")
    print(f"Archivo de datos: {ARCHIVO_CSV} (modo append)")
    print("-" * 55)

    bus = smbus2.SMBus(BUS_I2C)
    time.sleep(0.5)

    try:
        adquirir_muestras(bus)
    finally:
        bus.close()

    print("-" * 55)
    print(f"Corrida finalizada. Regenerando grafica con TODAS las muestras...")
    generar_grafica()
    print("Listo.")


if __name__ == "__main__":
    main()