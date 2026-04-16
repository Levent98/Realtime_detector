#ifndef ERROR_CHECK_H
#define ERROR_CHECK_H

#include <stdint.h>
#include "i2c.h"

/* I2C hata/saglik bilgileri */
typedef struct
{
    uint8_t bus_err_count;
    uint8_t timeout_count;
    uint8_t nack_count;
    uint8_t crc_count;

    uint8_t recover_request;   /* main recover yapsin */
    uint8_t recover_done;      /* 1 kere denendi mi */
    uint8_t fatal_fault;
} I2C_Diag_t;

/* Global tanim error_check.c içinde olacak */
extern I2C_Diag_t i2c_diag;

/* Basarili transaction sonrasi sayaçlari temizler */
void I2C_Diag_OnSuccess(void);

/* Hata tipine göre sayaç artirir */
void I2C_Diag_OnError(I2C_StatusType st);

/* Sayaçlara göre recover / fatal fault kararini verir */
void I2C_Diag_Check(void);

/* Istersen ayri resetleme fonksiyonu da kullan */
void I2C_Diag_ResetAll(void);

#endif /* ERROR_CHECK_H */