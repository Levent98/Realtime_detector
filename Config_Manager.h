#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <stdint.h>
#include <stdint.h>
#include <stdbool.h>

/* ============================ MACROS ====================================== */
/* ============================ MACROS ====================================== */
// STM32F410 128KB Flash için Sector 4 (Son Sektör) en güvenli yerdir.
#define CONFIG_FLASH_ADDR    0x08010000  
#define CONFIG_FLASH_SECTOR  4           // Sektör numarasi (Erase islemi için)
#define CONFIG_MAGIC_NUM     0xABCD1234
extern volatile uint8_t SaveDevSettings; 
extern volatile uint8_t SaveSensorInfo; // Sensör verileri için ayri bayrak

#include <stdint.h>

#pragma pack(push, 1)

/**
 * @brief HART Cihaz Ayarlari (Modbus paketindeki HART blogu ile uyumlu)
 */
typedef struct {
    uint8_t  address;           // HART Adresi
    uint8_t  multidrop_mode;    // Mode ayari
    uint16_t cnfg_chng_cntr;    // Sayaç
    uint8_t  tagDescDate[21];   // Tag(8)+Desc(12)+Date(3) = 21 Byte
    uint8_t  assmbly_number[3]; // Final Assembly No
    uint8_t  long_tag[32];      // Long Tag alani
    uint8_t  message[24];       // Mesaj alani
    uint16_t dmy1;              // Hizalama
    uint16_t signature;         // HART Checksum
} __attribute__( ( packed ) ) hart_device_stgn;

/**
 * @brief Ana Ayar Yapisi (SettingsStruct)
 * SETTING_DATA_PADING_SIZE = 8'e göre hizalanmistir.
 */
typedef struct __attribute__( ( packed, aligned(4) ) )
{
    // [Offset 0-7] Dolgu Alani
    uint8_t  dmy_fill[8];       
    
    // [Offset 8] Güvenlik Anahtari
    uint32_t verify_key;        // Dogrulama Key (4 Byte)
    
    // [Offset 12] Temel Bilgiler
    uint16_t id;                // data[0] -> Modbus Slave ID
    uint16_t hart_config;       // HART aktif/pasif bayraklari
    uint8_t  password[4];       // Cihaz sifresi
    uint8_t  base_ser_num[8];   // data[1..4] -> Seri Numarasi
    uint8_t  location[32];      // data[13..24] -> Lokasyon (32 byte rezerve)
    
    // [Offset 60] Ekran ve Sistem
    uint16_t display;           // data[39] & data[40] -> Ekran ayarlari
    uint16_t config_reg;        // data[29, 30, 41..] -> Sistem Flagleri
    
    // [Offset 64] Operasyonel Veriler
    uint8_t  analog_out_lev[8]; // data[33+i] -> Analog seviyeler
    uint16_t alarm_threshold[4];// data[7, 8, 9] -> Alarm esikleri
    uint8_t  alarm_off_hysteresis[4]; // data[53, 54, 55] -> Histerezis
    uint8_t  alarm_settings[4]; // data[21, 23, 25] -> Alarm modlari
    uint8_t  relay_off_delay[4];// data[22, 24, 26] -> Röle gecikmeleri
    
    // [Offset 88] Tarih ve Baudrate
    uint16_t man_date_year;     // data[31] -> Üretim yili
    uint16_t man_date_month;    // data[32] -> Üretim ayi
    uint32_t baudrate;          // data[43..44] -> 32-bit Baudrate
    
    // [Offset 96] HART Blogu
    hart_device_stgn hart;      
    
    // [Offset 186] Kapanis
    uint16_t dmy1;              // Padding
    uint16_t setting_crc;       // [Son 2 Byte] Veri Bütünlük Kontrolü (CRC16)
} SettingsStruct;
typedef struct
{
	uint16_t model;
	uint8_t model_indx;
	uint8_t dmy1;
	char *model_name;
}__attribute__( ( packed ) )XtmrMedelStruct;

#pragma pack(pop)

extern SettingsStruct DeviceConfig;

// Fonksiyon Prototipleri
void Config_Init(void);

/* ============================ FUNCTION PROTOTYPES ========================= */

/**
 * @brief Cihaz ilk açildiginda Flash'tan verileri RAM'e (DeviceConfig) yükler.
 * Veri bozuksa varsayilan degerleri atar.
 */
void Config_LoadFromFlash(void);

/**
 * @brief Mevcut DeviceConfig struct'ini Flash bellege kalici olarak yazar.
 */
uint8_t Config_SaveToFlash(void);

/**
 * @brief Tüm ayarlari fabrika ayarlarina döndürür ve Flash'i günceller.
 */
void Config_SetDefaults(void);

/**
 * @brief Modbus üzerinden gelen Base Settings paketini (FC16) isler ve kaydeder.
 * @param data Modbus register verileri (uint16_t dizisi)
 * @param len Register sayisi
 */
uint8_t Process_BaseSettings(uint16_t *data, uint8_t len);

/**
 * @brief Modbus üzerinden gelen Sensor Settings paketini (FC16) isler ve kaydeder.
 * @param data Modbus register verileri (uint16_t dizisi)
 * @param len Register sayisi
 */
uint8_t Process_SensorSettings(uint16_t *data, uint8_t len);

/**
 * @brief Tekil bir parametre degistiginde (FC06) checksum hesaplar ve kaydeder.
 */
void Config_QuickSave(void);
 void Internal_Process_Base(uint16_t *data);
// void Internal_Process_Sensor(uint16_t *data, uint8_t len);
 void Internal_Process_Sensor(uint16_t *data, uint16_t start_addr, uint16_t qty);
 uint16_t GetSensorModel(uint16_t indx);
 uint16_t CheckSensorType(uint16_t type);
#endif