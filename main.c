/******************************************************************************
 * File:    main.c
 * Brief:   SHT3x temperature & humidity measurement with UART DMA output
 * Standard: MISRA C:2012 (with documented deviations)
 ******************************************************************************/

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
#include "error_check.h"
#include "sensor_task.h"
#include "watchdog.h"


/*============================ MACROS ======================================*/
#define SHT3X_MEASURE_MEDIUM       (0x240BU)
#define MEASURE_DELAY_MS           (1000U)

extern volatile uint8_t rx_done;
volatile uint32_t last_time = 0;
uint32_t start_c, end_c, diff_c;
volatile uint32_t max_cycles = 0; // Global tanimla

// Global tanimlamalar
static MedianFilter_t temp_filter = {0};
static MedianFilter_t hum_filter = {0};

/*============================ FUNCTION PROTOTYPES =========================*/
void InitFilters(void);

/*============================ FUNCTIONS ===================================*/
// main() içinde, filtreleri sifirla
void InitFilters(void)
{
    temp_filter.index = 0;
    temp_filter.count = 0;
    hum_filter.index = 0;
    hum_filter.count = 0;

    // sort_buffer'i sifirla (opsiyonel)
    for (int i = 0; i < FILTER_SIZE; i++)
    {
        temp_filter.sort_buffer[i] = 0;
        hum_filter.sort_buffer[i] = 0;
    }
}

/*============================ MAIN ========================================*/
int main(void)
{
    setClock();
    SysTick_Init();

    UART_Init(57600);
    Modbus_Init();
    TimConfig();
    I2C1_Config();
//    ADC1_Init_PA3_PA4();
	  ADC1_DMA_Init();
    LCD_Init();
    LCD_Clear();
    DWT_Init();
    // 2. Reset Kaynagini Kontrol Et
if (RCC->CSR & RCC_CSR_IWDGRSTF)
    {
        // 12345678 (8 karakter siniri)
        LCD_Goto(0, 0);
        LCD_Puts("WDG RST!"); 
        LCD_Goto(1, 0);
        LCD_Puts("RECOVERY"); 
        
        RCC->CSR |= RCC_CSR_RMVF; 
        Delay_ms(3000); 
    }
    else
    {
        LCD_Goto(0, 0);
        LCD_Puts("SYSTEM  ");
        LCD_Goto(1, 0);
        LCD_Puts("READY   ");
        Delay_ms(1000);
    }
    I2C_Diag_ResetAll();
    Sensor_Task_Init();
    IWDG_Init_2s();

    while (1)
    {
if (i2c_diag.fatal_fault == 0U)
{
    IWDG_Kick();
}

if (i2c_diag.recover_request == 1U)
{
    I2C1_BusRecover();
    i2c_diag.recover_request = 0U;
    i2c_diag.recover_done = 1U;
    i2c_diag.bus_err_count = 0U;
    i2c_diag.timeout_count = 0U;
}
        start_c = DWT->CYCCNT;

        Modbus_Task();
        Sensor_Task();
        ADC_Task_Process();
        end_c = DWT->CYCCNT;
        diff_c = end_c - start_c;
        if (diff_c > max_cycles)
        {
            max_cycles = diff_c;
        }
    }
}