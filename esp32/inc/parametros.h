/*
 * parametros.h
 *
 *  Created on: May 29, 2026
 *      Author: FLORENCIA
 */

 #ifndef PARAMETROS_H
 #define PARAMETROS_H

 #include "driver/gpio.h"
 #include "driver/i2c_types.h"
 #include "esp_adc/adc_oneshot.h"

 #define DIRECCION_ESCLAVO     0x08
 #define PUERTO_I2C            I2C_NUM_0
 #define PIN_SDA               GPIO_NUM_21
 #define PIN_SCL               GPIO_NUM_22
 #define BUFFER_TX_I2C         32
 #define BUFFER_RX_I2C         32

 #define UNIDAD_ADC            ADC_UNIT_1
 #define CANAL_ADC_LDR         ADC_CHANNEL_6
 #define ATENUACION_ADC        ADC_ATTEN_DB_12
 #define RESOLUCION_ADC        ADC_BITWIDTH_12

 #define CANTIDAD_PROMEDIO     16
 #define MS_ENTRE_LECTURAS     200
 #define ADC_LIMITE_INFERIOR   0
 #define ADC_LIMITE_SUPERIOR   4095

 #endif