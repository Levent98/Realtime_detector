#include "sht3x.h"
#include "stm32f410rx.h"
#include "system_stm32f4xx.h"
#include <stddef.h>
#define SHT3X_ADDR_WRITE     (0x44U << 1)
#define SHT3X_ADDR_READ      ((0x44U << 1) | 1U)

/*
 * SHT30 sensörüne 16-bit komut gönderir
 * Komutlar :
 *  - Ölçüm baslatma
 *  - Periyodik ölçüm
 *  - Soft reset vb.
*/

#define SHT31_ADDR_W (0x44U << 1)
#define SHT31_ADDR_R ((0x44U << 1) | 1U)
#define SHT3X_ADDR     (0x44U)

/* ================= COMMAND ================= */
I2C_StatusType SHT3x_SendCommand(uint16_t cmd)
{
    I2C_StatusType ret;

    ret = I2C_Start();
    if (ret) return ret;

    ret = I2C_SendAddress(SHT31_ADDR_W);
    if (ret) return ret;

    ret = I2C_WriteByte((uint8_t)(cmd >> 8));
    if (ret) return ret;

    ret = I2C_WriteByte((uint8_t)(cmd & 0xFFU));
    if (ret) return ret;

    I2C_Stop();
    return I2C_OK;
}

I2C_StatusType SHT3x_ReadTempHum(uint16_t *temp, uint16_t *hum)
{
    uint8_t buf[6];
    I2C_StatusType ret;

    ret = I2C_Start();
    if (ret) return ret;

    ret = I2C_SendAddress(SHT31_ADDR_R);
    if (ret) return ret;

    /* Ilk 5 byte ACK */
    for (uint8_t i = 0; i < 5; i++)
    {
        ret = I2C_ReadByte(&buf[i], 1);
        if (ret) return ret;
    }

    ret = I2C_ReadByte(&buf[5], 0);
    if (ret) return ret;

    *temp = ((uint16_t)buf[0] << 8) | buf[1];
    *hum  = ((uint16_t)buf[3] << 8) | buf[4];

    return I2C_OK;
}
static uint8_t SHT3x_CRC8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0xFF;

    for (uint8_t i = 0; i < len; i++)
    {
        crc ^= data[i];

        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x31;
            else
                crc <<= 1;
        }
    }

    return crc;
}

I2C_StatusType SHT3x_ReadTempHum_(uint16_t *temp, uint16_t *hum)
{
	  if ((temp == NULL) || (hum == NULL))
    return I2C_ERROR_BUS;
    uint8_t buf[6];
    I2C_StatusType ret;

//    /* 1. Ölçüm baslat */
//    ret = SHT3x_SendCommand_(SHT3X_MEASURE_MEDIUM);
//    if (ret != I2C_OK)
//        return ret;

//    Delay_ms(15);   // datasheet: max 15ms

    /* 2. 6 byte oku */
    ret = I2C_ReadData(SHT3X_ADDR, buf, 6);
    if (ret != I2C_OK)
        return ret;

    /* 3. CRC kontrolü */
    if (SHT3x_CRC8(&buf[0], 2) != buf[2])
        return I2C_ERROR_CRC;

    if (SHT3x_CRC8(&buf[3], 2) != buf[5])
        return I2C_ERROR_CRC;

    /* 4. Raw degerleri çikar */
    *temp = ((uint16_t)buf[0] << 8) | buf[1];
    *hum  = ((uint16_t)buf[3] << 8) | buf[4];

    return I2C_OK;
}

I2C_StatusType SHT3x_SendCommand_(uint16_t cmd)
{
    uint8_t data[2];

    data[0] = (uint8_t)(cmd >> 8);
    data[1] = (uint8_t)(cmd & 0xFF);

    return I2C_WriteData(SHT3X_ADDR, data, 2);
}


int32_t SHT3x_Temp_x100(uint16_t raw)
{
    /* T = -45.00 + 175.00 * raw / 65535 */
    int32_t t;

    t = (17500L * (int32_t)raw) / 65535L;
    t -= 4500L;

    return t;   /* örn: -1234 => -12.34 °C */
}

int32_t SHT3x_Hum_x100(uint16_t raw) // int32_t olarak dönmeli
{
    return (int32_t)((10000L * (uint32_t)raw) / 65535UL);
}
