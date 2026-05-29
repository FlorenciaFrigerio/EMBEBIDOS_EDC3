/*
 * bus_i2c.h
 *
 *  Created on: May 29, 2026
 *      Author: FLORENCIA
 */
 #ifndef BUS_I2C_H
 #define BUS_I2C_H

 #include <stdint.h>

 void bus_i2c_iniciar(void);
 void bus_i2c_actualizar_medicion(uint16_t medicion);

 #endif