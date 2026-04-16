#include "modbus_rtu.h"
#include "delay.h"
#include "uart.h"
#include "config_manager.h"
#include "appcons.h"
#include "apptdef.h"
#include <string.h>
#include <stdint.h>
#define INPUT_REG_SIZE HOLDING_REG_SIZE
/*============================ STATIC DEGISKENLER =========================*/
static uint16_t holding_regs[HOLDING_REG_SIZE];
static uint16_t input_regs[INPUT_REG_SIZE];
static frame_queue_t frame_queue;

extern volatile uint8_t rtu_frame_ready;
extern volatile uint8_t uart_tx_busy;
extern volatile uint16_t baud_rate;

/* Test / simülasyon amaçli örnek degerler */
static int16_t temp_val = 250;   /* 25.0 C */
static int16_t hum_val  = 600;   /* 60.0 % */

/*============================ STATIC FONKSIYONLAR ========================*/
static uint16_t CRC16(const uint8_t *buf, uint16_t len);
static uint8_t ProcessReadHoldingRegisters(const frame_item_t *frame);
static uint8_t ProcessReadInputRegisters(const frame_item_t *frame);
static uint8_t ProcessWriteSingleRegister(const frame_item_t *frame);
static uint8_t ProcessWriteMultipleRegisters(const frame_item_t *frame);

static void Modbus_SetHoldingRegisterInternal(uint16_t reg, uint16_t value);
static uint16_t Modbus_GetHoldingRegisterInternal(uint16_t reg);
static void Modbus_SetInputRegisterInternal(uint16_t reg, uint16_t value);
static uint16_t Modbus_GetInputRegisterInternal(uint16_t reg);

/**
 * @brief CRC16 hesaplama (Modbus RTU standardi)
 */
static uint16_t CRC16(const uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0xFFFFU;

    for (uint16_t i = 0U; i < len; i++)
    {
        crc ^= buf[i];

        for (uint8_t j = 0U; j < 8U; j++)
        {
            if ((crc & 1U) != 0U)
            {
                crc = (crc >> 1U) ^ 0xA001U;
            }
            else
            {
                crc >>= 1U;
            }
        }
    }

    return crc;
}

/*============================ REGISTER ERISIM ============================*/
static void Modbus_SetHoldingRegisterInternal(uint16_t reg, uint16_t value)
{
    if (reg < HOLDING_REG_SIZE)
    {
        holding_regs[reg] = value;
    }
}

static uint16_t Modbus_GetHoldingRegisterInternal(uint16_t reg)
{
    if (reg < HOLDING_REG_SIZE)
    {
        /* Dinamik register örnegi */
        if (reg == REG_BAUD_RATE)
        {
            holding_regs[REG_BAUD_RATE] = (uint16_t)(baud_rate / 100U);
        }

        return holding_regs[reg];
    }

    return 0U;
}
void Modbus_SetInputRegister(uint16_t reg, uint16_t value)
{
    if (reg < INPUT_REG_SIZE)
    {
        input_regs[reg] = value;
    }
}
static void Modbus_SetInputRegisterInternal(uint16_t reg, uint16_t value)
{
    if (reg < INPUT_REG_SIZE)
    {
        input_regs[reg] = value;
    }
}

static uint16_t Modbus_GetInputRegisterInternal(uint16_t reg)
{
    if (reg < INPUT_REG_SIZE)
    {
        return input_regs[reg];
    }

    return 0U;
}

/*============================ PUBLIC SET/GET =============================*/
void Modbus_SetRegister(uint16_t reg, uint16_t value)
{
    Modbus_SetHoldingRegisterInternal(reg, value);
}

uint16_t Modbus_GetRegister(uint16_t reg)
{
    return Modbus_GetHoldingRegisterInternal(reg);
}

uint16_t Modbus_ReadRegister(uint16_t addr)
{
    return Modbus_GetHoldingRegisterInternal(addr);
}

void Modbus_Init(void)
{
    memset(holding_regs, 0, sizeof(holding_regs));
    memset(input_regs, 0, sizeof(input_regs));
    memset(&frame_queue, 0, sizeof(frame_queue));

    Config_LoadFromFlash();
    
    // Manuel tek tek atama yapmak yerine yukaridaki toplu fonksiyonu çagir:
    Modbus_SyncAll(); 
}
void Modbus_SyncAll(void) {
    uint8_t *p_str;
    uint16_t val;

    // --- 1. Temel Cihaz Bilgileri (Holding & Input) ---
    holding_regs[REG_TYPE_REG]     = 0x0021U;
    holding_regs[REG_DET_MODEL]    = 0x5202U;
    holding_regs[REG_FW_VERSION]   = 0x0103U;
    holding_regs[REG_FW_DATE]      = (uint16_t)((26U << 8) | 4U);
    holding_regs[REG_FW_DATE_2] =  0x0100U;
	  
    // Ayni verileri Input Registerlara da kopyala (Read-Only ayna)
    input_regs[REG_TYPE_REG]       = holding_regs[REG_TYPE_REG];
    input_regs[REG_DET_MODEL]      = holding_regs[REG_DET_MODEL];
    input_regs[REG_FW_VERSION]     = holding_regs[REG_FW_VERSION];
    input_regs[REG_FW_DATE]        = holding_regs[REG_FW_DATE];
    input_regs[REG_FW_DATE_2]=holding_regs[REG_FW_DATE_2]; // ust byte day yazilir
    // --- 2. Seri Numarasi (base_ser_num[8] -> 4 Register) ---
    for (uint8_t i = 0; i < 4U; i++) {
        val = ((uint16_t)DeviceConfig.base_ser_num[i * 2] << 8) | 
              (uint16_t)DeviceConfig.base_ser_num[i * 2 + 1];
        holding_regs[REG_BASE_SERIAL_NO + i] = val;
        input_regs[REG_BASE_SERIAL_NO + i]   = val;
    }

    // --- 3. Lokasyon Bilgisi (location[32] -> 16 Register) ---
    for (uint8_t i = 0; i < 16U; i++) {
        val = ((uint16_t)DeviceConfig.location[i * 2] << 8) | 
              (uint16_t)DeviceConfig.location[i * 2 + 1];
        holding_regs[REG_LOC_STRING_START + i] = val;
			  input_regs[REG_LOC_STRING_START + i]=val;
    }

    // --- 4. Alarm Esikleri (alarm_threshold[4]) ---
    holding_regs[REG_TEMP_HIGH_ALARM] = DeviceConfig.alarm_threshold[0];
    holding_regs[REG_TEMP_LOW_ALARM]  = DeviceConfig.alarm_threshold[1];
    holding_regs[REG_HUM_HIGH_ALARM]  = DeviceConfig.alarm_threshold[2];
    
    // Aktif esikleri Input tarafinda da göster
    input_regs[REG_TEMP_HIGH_ALARM]   = DeviceConfig.alarm_threshold[0];
    input_regs[REG_TEMP_LOW_ALARM]    = DeviceConfig.alarm_threshold[1];
    input_regs[REG_HUM_HIGH_ALARM]    = DeviceConfig.alarm_threshold[2];
    // --- 5. Alarm Modlari ve Gecikmeler (Paketli Byte) ---
    for (uint8_t i = 0; i < 3U; i++) {
         val=
            ((uint16_t)DeviceConfig.alarm_settings[i] << 8) | 
             (uint16_t)DeviceConfig.relay_off_delay[i];
			  input_regs[REG_ALARM1_CONFIG + i] = val;
			  holding_regs[REG_ALARM1_CONFIG + i] = val;
            
    }

    // --- 6. Histerezis (alarm_off_hysteresis[4]) ---
    holding_regs[REG_ALRM_HYSTERESIS_1] = 
        ((uint16_t)DeviceConfig.alarm_off_hysteresis[0] << 8) | 
         (uint16_t)DeviceConfig.alarm_off_hysteresis[1];
    holding_regs[REG_ALRM_HYSTERESIS_2] = (uint16_t)DeviceConfig.alarm_off_hysteresis[2];
    input_regs[REG_ALRM_HYSTERESIS_1] =holding_regs[REG_ALRM_HYSTERESIS_1];
    input_regs[REG_ALRM_HYSTERESIS_2]=holding_regs[REG_ALRM_HYSTERESIS_2];
    // --- 7. Analog Çikis Seviyeleri (analog_out_lev[8]) ---
    for (uint8_t i = 0; i < 6U; i++) {
        holding_regs[REG_ANALOG_OUT_LEV1 + i] = (uint16_t)DeviceConfig.analog_out_lev[i];
    }

    // --- 8. Sistem Ayarlari (Baud, Dil, Tarih, Config) ---
    holding_regs[REG_BAUD_RATE]        = (uint16_t)(DeviceConfig.baudrate / 100U);
    holding_regs[REG_DISPLAY_LANGUAGE] = DeviceConfig.display;
    holding_regs[REG_BASE_CONFIG]      = DeviceConfig.config_reg;
    holding_regs[REG_DATE_YEAR_MON]    = ((uint16_t)DeviceConfig.man_date_year << 8) | 
                                          (uint16_t)DeviceConfig.man_date_month;
    
    // Input taraflari
    input_regs[REG_BASE_CONFIG]        = DeviceConfig.config_reg;
    input_regs[REG_DATE_YEAR_MON]      = holding_regs[REG_DATE_YEAR_MON];
    input_regs[REG_GAS_TYPE]           = 0x31U;
    input_regs[REG_SCALING_FACTOR]     = 10U;
}
/*============================ INPUT UPDATE ===============================*/
void Sensor_Update(void)
{
    uint16_t status = 0U;
    uint16_t alarms = 0U;

    /* Canli veriler input register tarafina yazilir */
    Modbus_SetInputRegisterInternal(REG_TEMP_VALUE, (uint16_t)temp_val);
    Modbus_SetInputRegisterInternal(0x20U, (uint16_t)hum_val); /* Nem */

    if ((temp_val > (int16_t)Modbus_GetHoldingRegisterInternal(REG_TEMP_HIGH_ALARM)) ||
        (temp_val < (int16_t)Modbus_GetHoldingRegisterInternal(REG_TEMP_LOW_ALARM)))
    {
        status |= 0x01U;

        if (temp_val > (int16_t)Modbus_GetHoldingRegisterInternal(REG_TEMP_HIGH_ALARM))
        {
            alarms |= 0x01U;
        }

        if (temp_val < (int16_t)Modbus_GetHoldingRegisterInternal(REG_TEMP_LOW_ALARM))
        {
            alarms |= 0x02U;
        }
    }

    /* Humidity low alarm için REG_SCALING_FACTOR yerine gerçek ayri register
       yoksa burada örnek amaçli 0 kullanilabilir. Simdilik mevcut yapina zarar
       vermemek için sadece high alarm kontrolü birakildi. */
    if (hum_val > (int16_t)Modbus_GetHoldingRegisterInternal(REG_HUM_HIGH_ALARM))
    {
        status |= 0x02U;
        alarms |= 0x04U;
    }

    Modbus_SetInputRegisterInternal(REG_SENSOR_STATUS, status);
    Modbus_SetInputRegisterInternal(REG_ALARM_FLAGS, alarms);
}

uint8_t Modbus_SaveBaseSettings(uint16_t *data, uint8_t len)
{
    uint8_t ok = 1;
    uint8_t *p_str;
    uint32_t b_rate;

//    dbprintf("Base setup data len=%d\r\n", len);

    // data[0] -> Modbus Slave ID (ID)
    if((data[0] > 0U) && (data[0] <= 247U)){
        DeviceConfig.id = data[0]; // Struct'indaki id alanina atandi
//        NewSlaveAddr = data[0];
    } else {
        ok = FALSE;
    }

    // data[1..4] -> Seri Numarasi (base_ser_num)
    p_str = (uint8_t *)&DeviceConfig.base_ser_num[0];
    for(int i=0; i<4; i++) {
        p_str[i*2]   = (uint8_t)(data[1+i] >> 8);
        p_str[i*2+1] = (uint8_t)(data[1+i] & 0xFFU);
    }

    //data[9..12] -> RTC Tarih/Saat
    SysDate.Year  = data[9] >> 8;
    SysDate.Month = data[9] & 0xFFU;
    SysDate.Date  = data[10] >> 8;
    SysTime.Hours = data[11] >> 8;
    SysTime.Minutes = data[11] & 0xFFU;
    SysTime.Seconds = data[12] >> 8;
    SetSysDateTime(&SysDate, &SysTime);

    // data[13..24] -> Lokasyon (location - 32 byte alanin 24 byte'i dolar)
    p_str = (uint8_t *)&DeviceConfig.location[0];
    for(int i=0; i < 12; i++) {
        p_str[i*2]   = (data[13 + i] >> 8);
        p_str[i*2+1] = (data[13 + i] & 0xFFU);
    }
    p_str[24] = 0x00; // String sonlandirici

    // data[29..30] -> Config Bayraklari
    DeviceConfig.config_reg &= ~(TEST_DUE_FLT_INHIBIT | ZERO_SUPPRESSION_EN);
    if(data[29] == 0x01U) DeviceConfig.config_reg |= TEST_DUE_FLT_INHIBIT;
    if(data[30] == 0x01U) DeviceConfig.config_reg |= ZERO_SUPPRESSION_EN;

    // data[31..32] -> Üretim Tarihi
    DeviceConfig.man_date_year = data[31];
    DeviceConfig.man_date_month = data[32];

    // data[33..38] -> Analog Seviyeler (8 byte'lik diziye 6 adet yazilir)
    for(int i=0; i < 6; i++) {
        DeviceConfig.analog_out_lev[i] = (uint8_t)data[33+i];
    }

    // data[39..40] -> Ekran (display)
    DeviceConfig.display = data[39] & 0x0F; // Dil
    if(data[40] == 1U) DeviceConfig.display |= DISPLAY_ON;

    // data[43..44] -> Baudrate
    DeviceConfig.baudrate = ((uint32_t)data[43] << 16U) | data[44];

    if(ok) {
        SaveDevSettings = TRUE;
    }
    return ok;
}
uint8_t Modbus_SaveSensorSettings(uint16_t *data, uint8_t len)
{
    static SensorStruct settings;
    uint8_t ok = 0;
    uint16_t factor;

//    settings = Sensor; // Mevcut degerleri kopyala
    
    // data[63] -> mul_factor (Ölçekleme)
    factor = (len >= 64 && data[63] > 0) ? data[63] : 10;
    settings.mul_factor = factor;

    // data[0] -> MODEL (Senin belirttigin 00 02)
    settings.model = GetSensorModel(data[0]);
    if(settings.model == 0) ok = 2;

    // data[1..4] -> Tip, Gaz, Birim, Range
    settings.type = CheckSensorType(data[1]);
    settings.gas = data[2];
    settings.display_unit = data[3];
    settings.gas_conc_range = (double)data[4] / factor;

    // data[7, 8, 9] -> Alarm Esikleri (SettingsStruct içindeki diziye)
    DeviceConfig.alarm_threshold[0] = data[7];
    DeviceConfig.alarm_threshold[1] = data[8];
    DeviceConfig.alarm_threshold[2] = data[9];

    // data[13..16] -> Seri Numarasi (Sensor serial_number dizisine)
    for(int i=0; i<4; i++) {
        settings.serial_number[i*2]   = data[13+i] >> 8;
        settings.serial_number[i*2+1] = data[13+i] & 0xFFU;
    }

    // data[21..26] -> Alarm Modlari ve Gecikmeler
    for(int i=0; i<3; i++) {
        // data[21, 23, 25] -> Alarm Settings (Modlar)
        DeviceConfig.alarm_settings[i] = (uint8_t)data[21 + (i*2)]; 
        // data[22, 24, 26] -> Röle Off Delay
        DeviceConfig.relay_off_delay[i] = (uint8_t)data[22 + (i*2)];
    }

    // data[53..55] -> Histerezis
    if(len >= 56) {
        DeviceConfig.alarm_off_hysteresis[0] = (uint8_t)data[53];
        DeviceConfig.alarm_off_hysteresis[1] = (uint8_t)data[54];
        DeviceConfig.alarm_off_hysteresis[2] = (uint8_t)data[55];
    }

    if(ok == 0) {
//        Sensor = settings;
        SaveSensorInfo = TRUE;
        SaveDevSettings = TRUE;
        return 1;
    }
    return 0;
}

uint8_t Modbus_RunCommand(uint16_t start_addr, uint16_t qty)
{
    uint8_t ack = 1;
    
    // Yazilan alan Komut Register'ini kapsiyor mu?
    if(start_addr <= COMMAND_CODE && (start_addr + qty) > COMMAND_CODE)
    {
        uint16_t cmd = holding_regs[COMMAND_CODE];
        uint16_t *args = &holding_regs[COMMAND_ARG1];

        switch(cmd)
        {
            case SET_SENSOR_DATA:
                ack = Modbus_SaveSensorSettings(args, (uint8_t)qty - 2);
                break;

            case SET_DETECTOR_DATA:
                ack = Modbus_SaveBaseSettings(args, (uint8_t)qty - 2);
                break;

            case SET_TIME_DATE:
                // RTC Güncelleme fonksiyonu
                ack = 1;
                break;

            case RESET_DETECTOR:
                // NVIC_SystemReset();
                ack = 1;
                break;

            default:
                ack = 0;
                break;
        }

       
        
            Config_QuickSave();
        

        holding_regs[COMMAND_CODE] = 0; // Komutu temizle
    }

    return ack;
}

/**
 * @brief  FC04 okumadan önce canli verileri (Sicaklik, Nem, Durum) günceller
 */
void Modbus_UpdateLiveInputs(void)
{
//    input_regs[REG_TEMP_VALUE] = (uint16_t)temp_val;
    input_regs[0x20U] = (uint16_t)hum_val;
    
    // Status ve Alarm bayraklari
    uint16_t status = 0;
    if(temp_val > (int16_t)holding_regs[REG_TEMP_HIGH_ALARM]) status |= 0x01;
    input_regs[REG_SENSOR_STATUS] = status;
    
}
/*============================ MODBUS PROTOKOL HANDLERS ===================*/

static uint8_t ProcessReadHoldingRegisters(const frame_item_t *frame)
{
    uint16_t addr = (uint16_t)(((uint16_t)frame->data[2] << 8) | frame->data[3]);
    uint16_t qty  = (uint16_t)(((uint16_t)frame->data[4] << 8) | frame->data[5]);

    if ((qty == 0U) || (qty > 125U) || ((addr + qty) > HOLDING_REG_SIZE)) {
        Modbus_SendErrorResponse(FC_READ_HOLDING, MODBUS_ERR_ILLEGAL_DATA_ADDR);
        return 0U;
    }

    uint8_t resp[256];
    uint16_t idx = 0;
    resp[idx++] = MODBUS_SLAVE_ID;
    resp[idx++] = FC_READ_HOLDING;
    resp[idx++] = (uint8_t)(qty * 2U);

    for (uint16_t i = 0U; i < qty; i++) {
        uint16_t val = Modbus_GetHoldingRegisterInternal(addr + i);
        resp[idx++] = (uint8_t)(val >> 8);
        resp[idx++] = (uint8_t)(val & 0xFF);
    }

    uint16_t crc = CRC16(resp, idx);
    resp[idx++] = (uint8_t)(crc & 0xFF);
    resp[idx++] = (uint8_t)(crc >> 8);
    if (!uart_tx_busy) UART_Send(resp, idx);
    return 1;
}

static uint8_t ProcessReadInputRegisters(const frame_item_t *frame)
{
    uint16_t addr = (uint16_t)(((uint16_t)frame->data[2] << 8) | frame->data[3]);
    uint16_t qty  = (uint16_t)(((uint16_t)frame->data[4] << 8) | frame->data[5]);

    if ((qty == 0U) || (qty > 125U) || ((addr + qty) > HOLDING_REG_SIZE)) {
        Modbus_SendErrorResponse(FC_READ_INPUT, MODBUS_ERR_ILLEGAL_DATA_ADDR);
        return 0U;
    }

    //Modbus_UpdateLiveInputs(); // OKUMADAN ÖNCE GÜNCELLE

    uint8_t resp[256];
    uint16_t idx = 0;
    resp[idx++] = MODBUS_SLAVE_ID;
    resp[idx++] = FC_READ_INPUT;
    resp[idx++] = (uint8_t)(qty * 2U);

    for (uint16_t i = 0U; i < qty; i++) {
        uint16_t val = Modbus_GetInputRegisterInternal(addr + i);
        resp[idx++] = (uint8_t)(val >> 8);
        resp[idx++] = (uint8_t)(val & 0xFF);
    }

    uint16_t crc = CRC16(resp, idx);
    resp[idx++] = (uint8_t)(crc & 0xFF);
    resp[idx++] = (uint8_t)(crc >> 8);
    if (!uart_tx_busy) UART_Send(resp, idx);
    return 1;
}

static uint8_t ProcessWriteMultipleRegisters(const frame_item_t *frame)
{
    uint16_t addr = (uint16_t)(((uint16_t)frame->data[2] << 8) | frame->data[3]);
    uint16_t qty  = (uint16_t)(((uint16_t)frame->data[4] << 8) | frame->data[5]);
    uint8_t byte_cnt = frame->data[6];

    // 1. Standart Modbus Sinir Kontrolü
    if ((qty == 0U) || (qty > 123U) || (byte_cnt != (uint8_t)(qty * 2U)) || ((addr + qty) > HOLDING_REG_SIZE)) {
        Modbus_SendErrorResponse(FC_WRITE_MULTIPLE_REGS, MODBUS_ERR_ILLEGAL_DATA_ADDR);
        return 0U;
    }

    // 2. Verileri Holding Register'lara kopyala
    for (uint16_t i = 0U; i < qty; i++) {
        holding_regs[addr + i] = (uint16_t)(((uint16_t)frame->data[7 + (i*2)] << 8) | frame->data[8 + (i*2)]);
    }

    // 3. Önce cevabi hazirla ve gönder (Master timeout'a düsmesin)
    uint8_t resp[8];
    memcpy(resp, frame->data, 6U);
    uint16_t crc = CRC16(resp, 6U);
    resp[6] = (uint8_t)(crc & 0xFF);
    resp[7] = (uint8_t)(crc >> 8);
    
    if (!uart_tx_busy) UART_Send(resp, 8U);

    // 4. Cevap gittikten sonra agir isleri yap (Flash yazma vb.)
    // UART gönderiminin bitmesini bekle (opsiyonel ama güvenli)
    uint32_t timeout = 1000; 
    while(uart_tx_busy && --timeout); 

    Modbus_RunCommand(addr, qty); 

    return 1;
}
/*============================ WRITE CHECK ================================*/
uint8_t Modbus_Is_Writable(uint16_t addr)
{
    if (addr < HOLDING_REG_SIZE)
    {
        return 1U;
    }

    return 0U;
}

///*============================ FC06 WRITE SINGLE ==========================*/
static uint8_t ProcessWriteSingleRegister(const frame_item_t *frame)
{
    if ((frame == NULL) || (frame->len < 8U))
    {
        return 0U;
    }

    uint16_t addr = (uint16_t)(((uint16_t)frame->data[2] << 8) | frame->data[3]);
    uint16_t val  = (uint16_t)(((uint16_t)frame->data[4] << 8) | frame->data[5]);

    if (!Modbus_Is_Writable(addr))
    {
        Modbus_SendErrorResponse(FC_WRITE_SINGLE_REG, MODBUS_ERR_ILLEGAL_DATA_ADDR);
        return 0U;
    }

    Modbus_SetHoldingRegisterInternal(addr, val);

    {
        uint8_t resp[8];
        memcpy(resp, frame->data, 6U);

        uint16_t crc = CRC16(resp, 6U);
        resp[6] = (uint8_t)(crc & 0x00FFU);
        resp[7] = (uint8_t)(crc >> 8);

        if (!uart_tx_busy)
        {
            UART_Send(resp, 8U);
        }
    }

    return 1U;
}


/*============================ ERROR RESPONSE =============================*/
void Modbus_SendErrorResponse(uint8_t function, uint8_t error_code)
{
    uint8_t resp[5];

    resp[0] = MODBUS_SLAVE_ID;
    resp[1] = (uint8_t)(function | 0x80U);
    resp[2] = error_code;

    uint16_t crc = CRC16(resp, 3U);
    resp[3] = (uint8_t)(crc & 0x00FFU);
    resp[4] = (uint8_t)(crc >> 8);

    if (!uart_tx_busy)
    {
        UART_Send(resp, 5U);
    }
}

/*============================ FRAME PROCESS ==============================*/
uint8_t Modbus_ProcessFrame(const frame_item_t *frame)
{
    if ((frame == NULL) || (frame->len < 4U))
    {
        return 0U;
    }

    uint16_t crc_rx = (uint16_t)(frame->data[frame->len - 2U] |
                      ((uint16_t)frame->data[frame->len - 1U] << 8));

    if (CRC16(frame->data, (uint16_t)(frame->len - 2U)) != crc_rx)
    {
        return 0U;
    }

    if ((frame->data[0] != MODBUS_SLAVE_ID) && (frame->data[0] != 0U))
    {
        return 0U;
    }

    uint8_t result = 0U;

    switch (frame->data[1])
    {
        case FC_READ_HOLDING:
            result = ProcessReadHoldingRegisters(frame);
            break;

        case FC_READ_INPUT:
            result = ProcessReadInputRegisters(frame);
            break;

        case FC_WRITE_SINGLE_REG:
            result = ProcessWriteSingleRegister(frame);
            break;

        case FC_WRITE_MULTIPLE_REGS:
            result = ProcessWriteMultipleRegisters(frame);
            break;

        default:
            if (frame->data[0] != 0U)
            {
                Modbus_SendErrorResponse(frame->data[1], MODBUS_ERR_ILLEGAL_FUNCTION);
            }
            result = 0U;
            break;
    }

    return result;
}

/*============================ QUEUE ======================================*/
uint8_t Modbus_QueueFrame(uint8_t *data, uint16_t len)
{
    uint8_t success = 0U;

    if (data == NULL)
    {
        return 0U;
    }

    if (len > 256U)
    {
        len = 256U;
    }

    __disable_irq();

    if (frame_queue.count < FRAME_QUEUE_SIZE)
    {
        memcpy(frame_queue.frames[frame_queue.head].data, data, len);
        frame_queue.frames[frame_queue.head].len = len;
        frame_queue.frames[frame_queue.head].timestamp = GetTime_us();

        frame_queue.head++;
        if (frame_queue.head >= FRAME_QUEUE_SIZE)
        {
            frame_queue.head = 0U;
        }

        frame_queue.count++;
        success = 1U;
    }
    else
    {
        frame_queue.overflow_count++;
    }

    __enable_irq();

    return success;
}

uint8_t Modbus_DequeueFrame(frame_item_t *frame)
{
    uint8_t success = 0U;

    if (frame == NULL)
    {
        return 0U;
    }

    __disable_irq();

    if (frame_queue.count > 0U)
    {
        memcpy(frame->data,
               frame_queue.frames[frame_queue.tail].data,
               frame_queue.frames[frame_queue.tail].len);

        frame->len = frame_queue.frames[frame_queue.tail].len;
        frame->timestamp = frame_queue.frames[frame_queue.tail].timestamp;

        frame_queue.tail++;
        if (frame_queue.tail >= FRAME_QUEUE_SIZE)
        {
            frame_queue.tail = 0U;
        }

        frame_queue.count--;
        success = 1U;
    }

    __enable_irq();

    return success;
}

const frame_item_t *Modbus_PeekFrame(void)
{
    const frame_item_t *frame = NULL;

    __disable_irq();

    if (frame_queue.count > 0U)
    {
        frame = &frame_queue.frames[frame_queue.tail];
    }

    __enable_irq();

    return frame;
}

void Modbus_PopFrame(void)
{
    __disable_irq();

    if (frame_queue.count > 0U)
    {
        frame_queue.tail++;

        if (frame_queue.tail >= FRAME_QUEUE_SIZE)
        {
            frame_queue.tail = 0U;
        }

        frame_queue.count--;
    }

    __enable_irq();
}

uint8_t Modbus_GetQueueCount(void)
{
    uint8_t count;

    __disable_irq();
    count = frame_queue.count;
    __enable_irq();

    return count;
}

uint8_t Modbus_IsQueueFull(void)
{
    uint8_t full;

    __disable_irq();
    full = (frame_queue.count >= FRAME_QUEUE_SIZE) ? 1U : 0U;
    __enable_irq();

    return full;
}

uint8_t Modbus_IsQueueEmpty(void)
{
    uint8_t empty;

    __disable_irq();
    empty = (frame_queue.count == 0U) ? 1U : 0U;
    __enable_irq();

    return empty;
}

void Modbus_ClearQueue(void)
{
    __disable_irq();

    frame_queue.head = 0U;
    frame_queue.tail = 0U;
    frame_queue.count = 0U;

    __enable_irq();
}

uint32_t Modbus_GetOverflowCount(void)
{
    uint32_t count;

    __disable_irq();
    count = frame_queue.overflow_count;
    __enable_irq();

    return count;
}

/*============================ BULK UPDATE ================================*/
void Modbus_UpdateRegisters(uint16_t start_reg, uint16_t *data, uint16_t count)
{
    if (data == NULL)
    {
        return;
    }

    for (uint16_t i = 0U; i < count; i++)
    {
        if ((start_reg + i) < HOLDING_REG_SIZE)
        {
            holding_regs[start_reg + i] = data[i];
        }
    }
}

/*============================ TASK =======================================*/
void Modbus_Task(void)
{
    if (UART_FrameReady())
    {
        if (frame_queue.count < FRAME_QUEUE_SIZE)
        {
            uint8_t *target_ptr = frame_queue.frames[frame_queue.head].data;
            uint16_t len = UART_GetFrame(target_ptr, 256U);

            if (len > 0U)
            {
                __disable_irq();

                frame_queue.frames[frame_queue.head].len = len;
                frame_queue.frames[frame_queue.head].timestamp = GetTick();

                frame_queue.head++;
                if (frame_queue.head >= FRAME_QUEUE_SIZE)
                {
                    frame_queue.head = 0U;
                }

                frame_queue.count++;
                rtu_frame_ready = 0U;

                __enable_irq();
            }
        }
        else
        {
            UART_ClearFrameFlag();
            frame_queue.overflow_count++;
        }
    }

    const frame_item_t *current_frame = Modbus_PeekFrame();

    if (current_frame != NULL)
    {
        (void)Modbus_ProcessFrame(current_frame);
        Modbus_PopFrame();
    }
}