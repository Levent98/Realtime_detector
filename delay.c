#include "delay.h"
#include "stm32f410rx.h"
#include "uart.h"
volatile uint32_t g_ms_ticks = 0; // SysTick tarafindan artirilir
volatile uint16_t modbus_counter=0;

void TimConfig(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
    (void)RCC->APB1ENR;

    TIM6->CR1 = 0U;
    TIM6->PSC = 24U - 1U;      // 1 tick = 1 us
    TIM6->ARR = 0xFFFF;        // KRITIK: ARR maksimum olmali! 
                               // 999 yaparsan Delay_us(1500) çalismaz ve matematik sapitir.
    TIM6->CNT = 0U;
    TIM6->CR1 |= TIM_CR1_CEN;
}

void Delay_us(uint16_t us)
{
    uint16_t start = (uint16_t)TIM6->CNT;
    // uint16_t cast ve 0xFFFF ARR sayesinde tasma (overflow) otomatik çözülür
    while ((uint16_t)(TIM6->CNT - start) < us);
}

void Delay_ms(uint16_t ms)
{
    // SysTick tabanli milisaniye gecikmesi (LCD init için ideal)
    uint32_t start = g_ms_ticks;
    while ((g_ms_ticks - start) < ms);
}

void SysTick_Init(void)
{
    // Modbus T3.5 (1.75ms) yakalamak için 50us periyot (Saniyede 20bin kesme)
    // Eger sadece 1ms istersen 24000 - 1 yapabilirsin.
    SysTick->LOAD = 2400 - 1; // 50 us @ 24MHz
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk |
                    SysTick_CTRL_ENABLE_Msk;
}

void SysTick_Handler(void) {
    // 1. MODBUS ZAMANLAYICI (50us hassasiyetle çalisir)
    if (modbus_timer_running) {
        modbus_counter++;
        if (modbus_counter >= modbus_t35_steps) {
            modbus_timer_running = 0;
            modbus_counter = 0;
            rtu_frame_ready = 1; // Paket TAMAMLANDI bayragi
        }
    }

    // 2. MILISANIYE ZAMANLAYICI (GetTick/Delay_ms için)
    // 50us * 20 = 1000us = 1ms
    static uint8_t ms_divider = 0;
    ms_divider++;
    if (ms_divider >= 10) {
        g_ms_ticks++; // Iste simdi GetTick() düzgün çalisacak
        ms_divider = 0;
    }
}

uint32_t GetTick(void) {
    return g_ms_ticks;
}