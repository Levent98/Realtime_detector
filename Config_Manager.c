#include "config_manager.h"
#include <string.h>
#include "stm32f410rx.h" 
#include "modbus_rtu.h"
#include "appcons.h"
// Global Konfigürasyon Nesnesi
SettingsStruct DeviceConfig;
// Flash'a yazma istegi olup olmadigini tutan global bayrak
volatile uint8_t SaveDevSettings = FALSE; 
volatile uint8_t SaveSensorInfo = FALSE; // Sensör verileri için ayri bayrak
XtmrMedelStruct XtmrModel;
// Flash Ayarlari
#define CONFIG_FLASH_SECTOR  4           
#define FLASH_KEY1           0x45670123U
#define FLASH_KEY2           0xCDEF89ABU
/* --- Düsük Seviye Flash Fonksiyonlari --- */
static uint16_t CRC16(const uint8_t *buf, uint16_t len);

void Flash_Unlock(void) {
    if ((FLASH->CR & FLASH_CR_LOCK) != 0) {
        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;
    }
}

void Flash_Lock(void) {
    FLASH->CR |= FLASH_CR_LOCK;
}

/* --- Düsük Seviye Flash Fonksiyonlari (Düzeltildi) --- */

void Flash_Erase_Sector(uint8_t sector) {
    while (FLASH->SR & FLASH_SR_BSY);
    
    // Hata bayraklarini temizle (OPERR yerine kütüphanendeki karsiliklarini yazdik)
    FLASH->SR |= (FLASH_SR_EOP | FLASH_SR_WRPERR | FLASH_SR_PGAERR | 
                  FLASH_SR_PGPERR | FLASH_SR_PGSERR);

    FLASH->CR &= ~FLASH_CR_SNB; 
    FLASH->CR |= FLASH_CR_SER | (sector << FLASH_CR_SNB_Pos);
    FLASH->CR |= FLASH_CR_STRT;
    
    while (FLASH->SR & FLASH_SR_BSY);
    FLASH->CR &= ~FLASH_CR_SER;
}
void Flash_Write_Word(uint32_t address, uint32_t data) {
    while (FLASH->SR & FLASH_SR_BSY);
    
    FLASH->CR &= ~FLASH_CR_PSIZE;
    // Güvenli mod: 00 -> x8, 01 -> x16. 
    // 3.3V altinda çalisiyorsan x16 (1) en stabilidir.
    FLASH->CR |= (2U << FLASH_CR_PSIZE_Pos);
    FLASH->CR |= FLASH_CR_PG;

//    // 16-bit olarak iki adimda yaz (Daha güvenli)
//    *(__IO uint16_t*)address = (uint16_t)data;
//    while (FLASH->SR & FLASH_SR_BSY);
//    
//    *(__IO uint16_t*)(address + 2) = (uint16_t)(data >> 16);
//    while (FLASH->SR & FLASH_SR_BSY);
    // STM32F4 Word yazma (32-bit erisim)
    *(__IO uint32_t*)address = data;
    while (FLASH->SR & FLASH_SR_BSY);
    FLASH->CR &= ~FLASH_CR_PG;
}


/* --- Konfigürasyon Yönetimi --- */

uint8_t Config_SaveToFlash(void) {
    uint32_t *pData = (uint32_t*)&DeviceConfig;
    // Word sayisini hesapla (4 byte hizalama garantisiyle)
    uint32_t size_in_words = (sizeof(SettingsStruct) + 3) / 4;
    uint32_t current_addr = CONFIG_FLASH_ADDR;

    // Yazmadan önce verify_key güncellemesi (Opsiyonel)
    DeviceConfig.verify_key = CONFIG_MAGIC_NUM;

    Flash_Unlock();
    Flash_Erase_Sector(CONFIG_FLASH_SECTOR);

    for (uint32_t i = 0; i < size_in_words; i++) {
        Flash_Write_Word(current_addr, pData[i]);
        current_addr += 4;
    }

    Flash_Lock();
    
    // Dogrulama: Flash içerigi RAM ile ayni mi?
    if (memcmp(&DeviceConfig, (void*)CONFIG_FLASH_ADDR, sizeof(SettingsStruct)) == 0) {
        return 1; // Basarili
    }
    return 0; // Basarisiz
}

void Config_LoadFromFlash(void) {
    memcpy(&DeviceConfig, (void*)CONFIG_FLASH_ADDR, sizeof(SettingsStruct));

    // Kayitli CRC'yi oku
    uint16_t stored_crc = DeviceConfig.setting_crc;
    
    // Mevcut verilerle tekrar hesapla
    uint16_t calculated_crc = CRC16((uint8_t*)&DeviceConfig, sizeof(SettingsStruct) - 2);
//        DeviceConfig.id = 1;
//        DeviceConfig.baudrate = 9600;                     // Ekledik
    // Eger CRC uyusmuyorsa veya Magic Key hataliysa varsayilanlari yükle
    if (stored_crc != calculated_crc || DeviceConfig.verify_key != CONFIG_MAGIC_NUM) {
        memset(&DeviceConfig, 0, sizeof(SettingsStruct)); // Önce temizle
        DeviceConfig.id = 1;
        DeviceConfig.baudrate = 9600;                     // Ekledik
        DeviceConfig.verify_key = CONFIG_MAGIC_NUM;
        Config_QuickSave(); 
    }
}

/**
 * @brief  : Modbus üzerinden gelen Base Settings paketini isler.
 * @note   : Orijinal SaveBaseSettings lojigine göre data[x] indisleri korunmustur.
 * @param  : data = ModbusHoldingRegister[COMMAND_ARG1] adresi
 */
uint8_t Process_BaseSettings(uint16_t *data, uint8_t len)
{
    uint8_t ok = 1;

    // 1. Device ID (REG_TYPE_REG - 0x00)
    // Not: Tablona göre 0x00 Type'dir. Eger Slave ID 0x00'da tutuluyorsa:
    if((data[REG_TYPE_REG] > 0U) && (data[REG_TYPE_REG] <= 247U)) {
        DeviceConfig.id = data[REG_TYPE_REG];
    } else {
        // Eger 0. register ID degil de model tipiyse burayi lojigine göre ayarla
        DeviceConfig.id = data[REG_TYPE_REG]; 
    }

    // 2. Base Serial Number (REG_BASE_SERIAL_NO - 0x38) -> 8 Byte
    for(int i = 0; i < 4; i++) {
        DeviceConfig.base_ser_num[i * 2]     = (uint8_t)(data[REG_BASE_SERIAL_NO + i] >> 8);
        DeviceConfig.base_ser_num[i * 2 + 1] = (uint8_t)(data[REG_BASE_SERIAL_NO + i] & 0xFF);
    }

    // 3. Location (REG_LOC_STRING_START - 0x40) -> 32 Byte (16 Register)
    // Tablonda 0x40 - 0x4C arasi (13 register) ayrilmis. 13 reg = 26 byte + null.
    uint8_t loc_indx = 0;
    for(int i = 0; i < 13; i++) {
        DeviceConfig.location[loc_indx++] = (uint8_t)(data[REG_LOC_STRING_START + i] >> 8);
        DeviceConfig.location[loc_indx++] = (uint8_t)(data[REG_LOC_STRING_START + i] & 0xFF);
    }
    DeviceConfig.location[loc_indx] = 0x00; // Null terminator

    // 4. Config Flags (REG_BASE_CONFIG - 0x4D)
    // Orijinal kodundaki data[29] (0x1D) Analog Out Level iken, tablanda Config 0x4D'dir.
    DeviceConfig.config_reg = data[REG_BASE_CONFIG];

    // 5. Manufacture Date (REG_DATE_YEAR_MON - 0x0F ve REG_DATE_DAY - 0x10)
    DeviceConfig.man_date_year = (uint8_t)(data[REG_DATE_YEAR_MON] >> 8);
    DeviceConfig.man_date_month = (uint8_t)(data[REG_DATE_YEAR_MON] & 0xFF);
    // Eger struct'ta gün varsa: DeviceConfig.man_date_day = data[REG_DATE_DAY];

    // 6. Analog Output Levels (REG_ANALOG_OUT_LEV1 - 0x1D'den baslar)
    // 0x1D, 0x1E, 0x1F (3 adet tanimli)
    for(int i = 0; i < 3; i++) {
        DeviceConfig.analog_out_lev[i] = (uint8_t)data[REG_ANALOG_OUT_LEV1 + i];
    }

    // 7. Display Settings (REG_DISPLAY_LANGUAGE - 0x31)
    // data[39] yerine REG_DISPLAY_LANGUAGE (0x31) kullanildi.
    DeviceConfig.display = (data[REG_DISPLAY_LANGUAGE] & 0x0F); 

    // 8. Baudrate (REG_BAUD_RATE - 0x32)
    // Modbus'tan gelen 96, 192 gibi degerleri 100 ile çarparak 9600, 19200 yapar.
    DeviceConfig.baudrate = (uint32_t)data[REG_BAUD_RATE] * 100;

    // 9. Alarm Thresholds (Eslesen registerlar: 0x26, 0x27, 0x28)
    DeviceConfig.alarm_threshold[0] = data[REG_TEMP_HIGH_ALARM]; // A1
    DeviceConfig.alarm_threshold[1] = data[REG_TEMP_LOW_ALARM];  // A2
    DeviceConfig.alarm_threshold[2] = data[REG_HUM_HIGH_ALARM];  // A3

    // 10. Hysteresis (REG_ALRM_HYSTERESIS_1 - 0x68)
    DeviceConfig.alarm_off_hysteresis[0] = (uint8_t)(data[REG_ALRM_HYSTERESIS_1] >> 8);   // A1
    DeviceConfig.alarm_off_hysteresis[1] = (uint8_t)(data[REG_ALRM_HYSTERESIS_1] & 0xFF); // A2
    DeviceConfig.alarm_off_hysteresis[2] = (uint8_t)data[REG_ALRM_HYSTERESIS_2];         // A3 (0x69)

#if (USE_HART_SETTINGS == 1)
    // HART verileri genellikle tablonun sonunda veya ayri bloktadir.
    // Eger data buffer'i yeterince büyükse (len kontrolü):
    if(len > 0x50) { // Örnek kontrol
        DeviceConfig.hart.address = (uint8_t)data[REG_CALB_CONC_VAL]; // Örnek eslestirme
    }
#endif

    return ok;
}
/**
 * @brief  : Modbus üzerinden gelen Sensor Settings paketini isler.
 * @note   : Orijinal SaveSensorSettings lojigine göre data[x] indisleri korunmustur.
 */
uint8_t Process_SensorSettings(uint16_t *data, uint8_t len)
{
    uint8_t ok = 1;

    // 1. Alarm Thresholds (data[7, 8, 9])
    DeviceConfig.alarm_threshold[0] = data[7]; // ALARM1
    DeviceConfig.alarm_threshold[1] = data[8]; // ALARM2
    DeviceConfig.alarm_threshold[2] = data[9]; // ALARM3

    // 2. Alarm Config & Off Delays (data[21..26])
    // data[21, 23, 25] -> High Byte: config_status, Low Byte: Rezerve
    // data[22, 24, 26] -> Low Byte: off_delay
    for(int i = 0; i < 3; i++) {
        uint16_t cfg_reg = data[21 + (i * 2)];
        uint16_t dly_reg = data[22 + (i * 2)];
        
        DeviceConfig.alarm_settings[i]  = (uint8_t)(cfg_reg >> 8);
        DeviceConfig.relay_off_delay[i] = (uint8_t)(dly_reg & 0xFF);
    }

    // 3. Hysteresis (data[53, 54, 55])
    if(len >= 57U) {
        DeviceConfig.alarm_off_hysteresis[0] = (uint8_t)data[53];
        DeviceConfig.alarm_off_hysteresis[1] = (uint8_t)data[54];
        DeviceConfig.alarm_off_hysteresis[2] = (uint8_t)data[55];
    }

    // 4. Multiplier Factor (data[63])
    // Not: Orijinal kodda bu veri sensor struct icinde float islemleri icin kullaniliyor.
    // Eger bu veriyi saklaman gerekiyorsa struct'a uint16_t mul_factor eklenmelidir.

    return ok;
}
static uint16_t CRC16(const uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0xFFFFU;
    
    for(uint16_t i = 0; i < len; i++)
    {
        crc ^= buf[i];
        for(uint8_t j = 0; j < 8; j++)
        {
            if(crc & 1U) 
                crc = (crc >> 1U) ^ 0xA001U;  /* Polinom: 0x8005 (ters çevrilmis) */
            else 
                crc >>= 1U;
        }
    }
    return crc;
}
/**
 * @brief  : Modbus RunModbusCmd icindeki SET_PASWORD ve SET_DEVICE_ID case'leri icin
 */
void Config_QuickSave(void) 
{
    // 1. Önce CRC hesapla ve struct içine koy
    DeviceConfig.setting_crc = CRC16((uint8_t*)&DeviceConfig, sizeof(SettingsStruct) - 2);
    
    // 2. Bayraga bakmaksizin hemen yaz (Çünkü manuel tetiklendi)
    Config_SaveToFlash(); 
    
    // 3. Eger bekleyen bir bayrak varsa onu da temizle
    SaveDevSettings = FALSE;
}

uint16_t GetSensorModel(uint16_t indx)
{
	uint16_t m;

	if(indx == XtmrModel.model_indx){
		m = XtmrModel.model;
//		dbiprintf("%s model\r\n",XtmrModel.model_name);
	}else if(indx == 3){
		m = GENERIC_MODEL_DET;
//		dbiprintf("GENERIC model\r\n");
	}else{
//		dbiprintf("UNDEFINED model\r\n");
		m = 0x00;
	}
	return(m);
}

uint16_t CheckSensorType(uint16_t type)
{
	uint16_t sensor_type;

	sensor_type = type;
	switch(type){
	case INFRARED_SENSOR:
//		dboprintf("sensor_type=INFRARED SENSOR\r\n");
		break;
	case ELECTRO_CHEM_SENSOR:
//		dboprintf("sensor_type=ELECTRO_CHEM_SENSOR\r\n");
		break;
	case CATALYTIC_SENSOR:
//		dboprintf("sensor_type=CATALYTIC_SENSOR\r\n");
		break;
	case SEMICONDUCTOR_SENSOR:
//		dboprintf("sensor_type=SEMICONDUCTOR_SENSOR\r\n");
		break;
	case PID_SENSOR:
//		dboprintf("sensor_type = PID_SENSOR\r\n");
		break;
	case IRNET_7S_SENSOR:
//		dboprintf("sensor_type = IRNET_7S_SENSOR\r\n");
		break;
	case IRNET_MPS_SENSOR:
//		dboprintf("sensor_type = IRNET_MPS_SENSOR\r\n");
		break;
	case IR_VOLT_SENSOR:
//		dboprintf("sensor_type = IR_VOLT_SENSOR\r\n");
		break;
	case IR_VOLT_MPS_SENSOR:
//		dboprintf("sensor_type = IR_VOLT_MPS_SENSOR\r\n");
		break;
	default:
		sensor_type = 0;
//		dboprintf("sensor_type=%#x UNKNOWN....\r\n", type);
		break;
	}
	return(sensor_type);
}