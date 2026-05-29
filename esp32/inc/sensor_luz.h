/*
 * sensor_luz.h
 *
 *  Created on: May 29, 2026
 *      Author: FLORENCIA
 */

 #ifndef SENSOR_LUZ_H
 #define SENSOR_LUZ_H

 #include <stdint.h>
 #include <stdbool.h>

 void sensor_iniciar(void);
 bool sensor_capturar(uint16_t *resultado);
 void sensor_lanzar_tarea_adquisicion(void);

 #endif
