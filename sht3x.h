#ifndef SHT3X_H
#define SHT3X_H

#include <stdint.h>
#include "i2c.h"

#define SHT3X_ADDR_WRITE     (0x44U << 1)
#define SHT3X_ADDR_READ      ((0x44U << 1) | 1U)

#define SHT3X_MEASURE_MEDIUM (0x240BU)

I2C_StatusType SHT3x_SendCommand(uint16_t cmd);
I2C_StatusType SHT3x_ReadTempHum(uint16_t *temp, uint16_t *hum);
I2C_StatusType SHT3x_ReadTempHum_(uint16_t *temp, uint16_t *hum);
I2C_StatusType SHT3x_SendCommand_(uint16_t cmd);
int32_t SHT3x_Temp_x100(uint16_t raw);
int32_t SHT3x_Hum_x100(uint16_t raw);

#endif
