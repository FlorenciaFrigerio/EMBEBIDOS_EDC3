/*
 * sensor_luz.c
 *
 *  Created on: May 29, 2026
 *      Author: FLORENCIA
 */
 #include "sensor_luz.h"
 #include "bus_i2c.h"
 #include "parametros.h"
 #include "esp_adc/adc_oneshot.h"
 #include "esp_log.h"
 #include "freertos/FreeRTOS.h"
 #include "freertos/task.h"

 static const char *ETIQUETA = "SENSOR";
 static adc_oneshot_unit_handle_t handle_adc;

 void sensor_iniciar(void)
 {
     adc_oneshot_unit_init_cfg_t cfg_unidad = {
         .unit_id  = UNIDAD_ADC,
         .ulp_mode = ADC_ULP_MODE_DISABLE,
     };
     ESP_ERROR_CHECK(adc_oneshot_new_unit(&cfg_unidad, &handle_adc));

     adc_oneshot_chan_cfg_t cfg_canal = {
         .bitwidth = RESOLUCION_ADC,
         .atten    = ATENUACION_ADC,
     };
     ESP_ERROR_CHECK(adc_oneshot_config_channel(handle_adc, CANAL_ADC_LDR, &cfg_canal));

     ESP_LOGI(ETIQUETA, "ADC inicializado canal=%d", CANAL_ADC_LDR);
 }

 bool sensor_capturar(uint16_t *resultado)
 {
     uint32_t acumulado = 0;
     uint8_t  validas   = 0;
     int      muestra;

     for (uint8_t i = 0; i < CANTIDAD_PROMEDIO; i++) {
         esp_err_t rc = adc_oneshot_read(handle_adc, CANAL_ADC_LDR, &muestra);
         if (rc == ESP_OK &&
             muestra >= ADC_LIMITE_INFERIOR &&
             muestra <= ADC_LIMITE_SUPERIOR)
         {
             acumulado += muestra;
             validas++;
         }
         vTaskDelay(pdMS_TO_TICKS(2));
     }

     if (validas == 0) {
         return false;
     }
     *resultado = (uint16_t)(acumulado / validas);
     return true;
 }

 static void tarea_adquisicion_privada(void *param)
 {
     uint16_t lectura;
     uint16_t ultimo_valido = 0;

     while (1) {
         if (sensor_capturar(&lectura)) {
             ultimo_valido = lectura;
         } else {
             ESP_LOGW(ETIQUETA, "Captura fallida, se mantiene el valor previo");
         }
         bus_i2c_actualizar_medicion(ultimo_valido);
         vTaskDelay(pdMS_TO_TICKS(MS_ENTRE_LECTURAS));
     }
 }

 void sensor_lanzar_tarea_adquisicion(void)
 {
     xTaskCreate(tarea_adquisicion_privada, "adquisicion", 4096, NULL, 5, NULL);
     ESP_LOGI(ETIQUETA, "Tarea de adquisicion lanzada (cada %d ms)", MS_ENTRE_LECTURAS);
 }