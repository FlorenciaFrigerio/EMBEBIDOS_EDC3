#include <stdint.h>
#include "esp_log.h"
#include "sensor_luz.h"
#include "bus_i2c.h"

void app_main(void)
{
    ESP_LOGI("APP", " Examen 3 Embebidos - Monitoreo de luminosidad");

    sensor_iniciar();
    bus_i2c_iniciar();
    sensor_lanzar_tarea_adquisicion();
}