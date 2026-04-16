#ifndef PWM_H
#define PWM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void pwm_init(void);
void pwm_set_tim11(uint16_t duty);   // 0–100
void pwm_set_tim5(uint16_t duty); 
uint32_t temperature_to_current_ma(int32_t );
uint32_t humidity_to_current_ma(uint32_t );  // 0–100
uint16_t current_to_pwm(uint32_t );
void set_temperature_output(uint32_t );
void set_humidity_output(uint32_t );
void DWT_Init(void);
#ifdef __cplusplus
}
#endif

#endif
