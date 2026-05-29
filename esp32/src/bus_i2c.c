/*
 * bus_i2c.c
 *
 *  Created on: May 29, 2026
 *      Author: FLORENCIA
 */

 #include "bus_i2c.h"
 #include "parametros.h"
 #include "driver/i2c_slave.h"
 #include "esp_log.h"
 #include "freertos/FreeRTOS.h"
 #include "freertos/task.h"
 #include "freertos/queue.h"

 static const char *ETIQUETA = "I2C-ESC";

 static i2c_slave_dev_handle_t handle_esclavo;
 static QueueHandle_t cola_eventos = NULL;
 static volatile uint16_t medicion_a_enviar = 0;

 static IRAM_ATTR bool al_recibir_request(i2c_slave_dev_handle_t handle,
                                          const i2c_slave_request_event_data_t *evt,
                                          void *usuario)
 {
     BaseType_t despertar = pdFALSE;
     uint8_t senal = 1;
     xQueueSendFromISR(cola_eventos, &senal, &despertar);
     return despertar == pdTRUE;
 }

 static void tarea_enviar_respuesta(void *param)
 {
     uint8_t  senal;
     uint8_t  payload[2];
     uint32_t escritos;

     while (1) {
         if (xQueueReceive(cola_eventos, &senal, portMAX_DELAY) == pdTRUE) {
             uint16_t medicion = medicion_a_enviar;
             payload[0] = (medicion >> 8) & 0xFF;
             payload[1] = medicion        & 0xFF;

             esp_err_t rc = i2c_slave_write(handle_esclavo, payload, 2,
                                             &escritos, 1000);
             if (rc != ESP_OK) {
                 ESP_LOGW(ETIQUETA, "Fallo de escritura: %s", esp_err_to_name(rc));
             }
         }
     }
 }

 void bus_i2c_iniciar(void)
 {
     cola_eventos = xQueueCreate(10, sizeof(uint8_t));

     i2c_slave_config_t cfg = {
         .i2c_port          = PUERTO_I2C,
         .sda_io_num        = PIN_SDA,
         .scl_io_num        = PIN_SCL,
         .clk_source        = I2C_CLK_SRC_DEFAULT,
         .send_buf_depth    = BUFFER_TX_I2C,
         .receive_buf_depth = BUFFER_RX_I2C,
         .slave_addr        = DIRECCION_ESCLAVO,
         .addr_bit_len      = I2C_ADDR_BIT_LEN_7,
     };
     ESP_ERROR_CHECK(i2c_new_slave_device(&cfg, &handle_esclavo));

     i2c_slave_event_callbacks_t callbacks = {
         .on_request = al_recibir_request,
     };
     ESP_ERROR_CHECK(i2c_slave_register_event_callbacks(handle_esclavo,
                                                         &callbacks, NULL));

     xTaskCreate(tarea_enviar_respuesta, "i2c_tx", 4096, NULL, 10, NULL);

     ESP_LOGI(ETIQUETA, "Esclavo I2C listo dir=0x%02X SDA=GPIO%d SCL=GPIO%d",
              DIRECCION_ESCLAVO, PIN_SDA, PIN_SCL);
 }

 void bus_i2c_actualizar_medicion(uint16_t medicion)
 {
     medicion_a_enviar = medicion;
 }f