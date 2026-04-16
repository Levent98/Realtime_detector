
#include "delay.h"
#include "sysclock.h"
#include "uart.h"
#include "i2c.h"
#include "sht3x.h"
#include "lcd.h"
#include "pwm.h"
#include "modbus_rtu.h"
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>    /* Deviation: snprintf used for debug output */
#include <string.h>
#include "stm32f410rx.h"
#include "filter.h"
#include "adc.h"



typedef struct
{
    uint8_t bus_err_count;
    uint8_t timeout_count;
    uint8_t nack_count;
    uint8_t crc_count;
    uint8_t recover_request;
    uint8_t recover_done;
    uint8_t fatal_fault;
} I2C_Diag_t;

I2C_Diag_t i2c_diag = {0};

void I2C_Diag_ResetAll(void)
{
    i2c_diag.bus_err_count   = 0U;
    i2c_diag.timeout_count   = 0U;
    i2c_diag.nack_count      = 0U;
    i2c_diag.crc_count       = 0U;
    i2c_diag.recover_request = 0U;
    i2c_diag.recover_done    = 0U;
    i2c_diag.fatal_fault     = 0U;
}
void I2C_Diag_OnSuccess(void)
{
    i2c_diag.bus_err_count = 0U;
    i2c_diag.timeout_count = 0U;
    i2c_diag.nack_count = 0U;
    i2c_diag.crc_count = 0U;
    i2c_diag.recover_request = 0U;
    i2c_diag.recover_done = 0U;
}

void I2C_Diag_OnError(I2C_StatusType st)
{
    switch (st)
    {
        case I2C_ERROR_BUS:
            if (i2c_diag.bus_err_count < 255U) { i2c_diag.bus_err_count++; }
            break;

        case I2C_ERROR_TIMEOUT:
            if (i2c_diag.timeout_count < 255U) { i2c_diag.timeout_count++; }
            break;

        case I2C_ERROR_NACK:
            if (i2c_diag.nack_count < 255U) { i2c_diag.nack_count++; }
            break;

        case I2C_ERROR_CRC:
            if (i2c_diag.crc_count < 255U) { i2c_diag.crc_count++; }
            break;

        default:
            break;
    }
}

void I2C_Diag_Check(void)
{
    uint8_t severe_count;

    severe_count = (uint8_t)(i2c_diag.bus_err_count + i2c_diag.timeout_count);

    if (severe_count >= 3U)
    {
        if (i2c_diag.recover_done == 0U)
        {
            i2c_diag.recover_request = 1U;
        }
        else
        {
            i2c_diag.fatal_fault = 1U;
        }
    }
}