#ifndef DELAY_H
#define DELAY_H

#include <stdint.h>

void TimConfig(void);
void Modbus_Timer_Start(uint32_t baud);
uint32_t GetTick(void); // SysTick yerine geçecek milisaniye sayaci
void Delay_us(uint16_t us);
void Delay_ms(uint16_t ms);
void SysTick_Init(void);
// State Machine için zaman kontrol makrosu
#define IS_TIMEOUT(start, interval) ((GetTick() - (start)) >= (interval))
//uint32_t GetTick(void);
//uint32_t GetTime_us(void);

#endif