#include "delay.h"

#ifndef I2C_H
#define I2C_H

#include <stdint.h>

typedef enum
{
    I2C_OK = 0,
    I2C_ERROR_TIMEOUT,
	  I2C_ERROR_NACK,
    I2C_ERROR_BUS,
	  I2C_ERROR_CRC
} I2C_StatusType;

void I2C1_Config(void);
I2C_StatusType I2C_Start(void);
void I2C1_BusRecover(void);
I2C_StatusType I2C_SendAddress(uint8_t address);
I2C_StatusType I2C_WriteByte(uint8_t data);
I2C_StatusType I2C_ReadByte(uint8_t *data, uint8_t ack);
I2C_StatusType I2C_ReadData(uint8_t , uint8_t*, uint16_t );
I2C_StatusType I2C_WriteData(uint8_t slaveAddr, uint8_t *data, uint16_t size);
I2C_StatusType I2C_Stop(void);
I2C_StatusType I2C_WriteData_Wrapper(uint8_t slaveAddr, uint8_t *data, uint16_t size);
I2C_StatusType I2C_ReadData_Wrapper(uint8_t slaveAddr, uint8_t *buffer, uint16_t size);
#endif