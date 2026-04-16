#include "sensor_task.h"

#include "delay.h"
#include "sht3x.h"
#include "lcd.h"
#include "modbus_rtu.h"
#include "filter.h"
#include "error_check.h"
#include <stdint.h>

#define SHT3X_MEASURE_MEDIUM   (0x240BU)

typedef enum
{
    SENSOR_STATE_TRIGGER = 0,
    SENSOR_STATE_READ
} SensorState_t;

/* Bu degiskenler baska yerde extern degilse burada static kalabilir */
static SensorState_t sensor_state = SENSOR_STATE_TRIGGER;
static uint32_t last_sensor_time = 0U;

static uint16_t t_raw = 0U;
static uint16_t h_raw = 0U;
static int32_t t_x100 = 0;
static int32_t h_x100 = 0;

static MedianFilter_t temp_filter = {0};
static MedianFilter_t hum_filter  = {0};

static void InitFilters(void)
{
    int i;

    temp_filter.index = 0U;
    temp_filter.count = 0U;
    hum_filter.index  = 0U;
    hum_filter.count  = 0U;

    for (i = 0; i < FILTER_SIZE; i++)
    {
        temp_filter.sort_buffer[i] = 0;
        hum_filter.sort_buffer[i]  = 0;
    }
}
void Sensor_Task_Init(void)

{

    sensor_state = SENSOR_STATE_TRIGGER;

    last_sensor_time = 0U;



    t_raw = 0U;

    h_raw = 0U;

    t_x100 = 0;

    h_x100 = 0;



    InitFilters();

}
void Sensor_Task(void)
{
    switch (sensor_state)
    {
        case SENSOR_STATE_TRIGGER:
        {
            /* Ilk açilista hemen, sonra her 1 saniyede bir tetikle */
            if ((last_sensor_time == 0U) || ((GetTick() - last_sensor_time) >= 1000U))
            {
                I2C_StatusType st = SHT3x_SendCommand_(SHT3X_MEASURE_MEDIUM);

                if (st == I2C_OK)
                {
                    I2C_Diag_OnSuccess();
                    last_sensor_time = GetTick();
                    sensor_state = SENSOR_STATE_READ;
                }
                else
                {
                    I2C_Diag_OnError(st);
                    I2C_Diag_Check();

                    if (st == I2C_ERROR_BUS)
                    {
                        last_sensor_time = GetTick() - 950U;   /* ~50 ms sonra tekrar dene */
                    }
                    else
                    {
                        last_sensor_time = GetTick() - 900U;   /* ~100 ms sonra tekrar dene */
                    }
                }
            }
            break;
        }

        case SENSOR_STATE_READ:
        {
            /* Sensörün ölçümü tamamlamasi için gereken ~20ms bekleme süresi */
            if ((GetTick() - last_sensor_time) >= 20U)
            {
                I2C_StatusType st = SHT3x_ReadTempHum_(&t_raw, &h_raw);

                if (st == I2C_OK)
                {
                    int32_t h_input;
                    I2C_Diag_OnSuccess();

                    /* Geçersiz veri kontrolü */
                    if ((h_raw == 0xFFFFU) || (h_raw == 0x0000U) ||
                        (t_raw == 0xFFFFU) || (t_raw == 0x0000U))
                    {
                        sensor_state = SENSOR_STATE_TRIGGER;
                        last_sensor_time = GetTick();
                        break;
                    }

                    /* Filtreleme Islemleri */
                    if (temp_filter.count == 0U)
                    {
                        int i;
                        int32_t first_t = SHT3x_Temp_x100(t_raw);
                        int32_t first_h = SHT3x_Hum_x100(h_raw);

                        for (i = 0; i < FILTER_SIZE; i++)
                        {
                            temp_filter.samples[i] = first_t;
                            hum_filter.samples[i]  = first_h;
                        }
                        temp_filter.count = FILTER_SIZE;
                        hum_filter.count  = FILTER_SIZE;
                        t_x100 = first_t;
                        h_x100 = first_h;
                    }
                    else
                    {
                        t_x100 = ApplyMedianFilter18(&temp_filter, SHT3x_Temp_x100(t_raw));

                        h_input = SHT3x_Hum_x100(h_raw);
                        if (h_input < 0)     h_input = 0;
                        if (h_input > 10000) h_input = 10000;
                        h_x100 = ApplyMedianFilter18(&hum_filter, h_input);
                    }

                    // ==========================================================
                    // 1. MODBUS VERI GÜNCELLEME (Holding & Input Senkron)
                    // ==========================================================
                    uint16_t current_temp = (uint16_t)(t_x100 / 10); // Örn: 25.4C -> 254
                    uint16_t current_gas  = (uint16_t)(h_x100 / 10); // Örn: 60.2% -> 602

                    Modbus_SetInputRegister(REG_TEMP_VALUE, current_temp);
                    Modbus_SetInputRegister(REG_GAS_CONCT_VALUE, current_gas);

                    // ==========================================================
                    // 2. ALARM VE STATUS HESAPLAMA
                    // ==========================================================
                    uint16_t status = 0;
                    uint16_t alarms = 0;

                    // Sicaklik Limit Kontrolü
                    uint16_t t_high = Modbus_GetRegister(REG_TEMP_HIGH_ALARM);
                    uint16_t t_low  = Modbus_GetRegister(REG_TEMP_LOW_ALARM);

                    if(current_temp > t_high || current_temp < t_low) 
                    {
                        status |= 0x01; // Sicaklik hatasi bitini set et
                        if(current_temp > t_high) alarms |= 0x01; // High Alarm biti
                        if(current_temp < t_low)  alarms |= 0x02; // Low Alarm biti
                    }

                    
                    Modbus_SetInputRegister(REG_SENSOR_STATUS, status);
                    Modbus_SetInputRegister(REG_ALARM_FLAGS, alarms);

                    // ==========================================================
                    // 3. LCD YAZDIRMA
                    // ==========================================================
                    LCD_Goto(0, 0);
                    LCD_Puts("T:");
                    LCD_PrintFixed(t_x100, 2);
                    LCD_Puts("C   ");

                    LCD_Goto(1, 0);
                    LCD_Puts("H:");
                    LCD_PrintFixed(h_x100, 2);
                    LCD_Puts("%   ");

                    last_sensor_time = GetTick();
                    sensor_state = SENSOR_STATE_TRIGGER;
                }
                else
                {
                    I2C_Diag_OnError(st);
                    I2C_Diag_Check();
                    sensor_state = SENSOR_STATE_TRIGGER;
                    last_sensor_time = GetTick();
                }
            }
            break;
        }

        default:
            sensor_state = SENSOR_STATE_TRIGGER;
            last_sensor_time = 0U;
            break;
    }
}