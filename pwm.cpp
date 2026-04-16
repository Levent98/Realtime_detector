#include "stm32f410rx.h"
#include "pwm.h"
#include <stdint.h>
/*
 * SYSCLK = 24 MHz
 * PWM = 10 kHz
 *
 * Timer clock = 24 MHz
 * PSC = 23  -> 1 MHz
 * ARR = 99  -> 10 kHz
 */
//TIM5 PA1 TEMP
//TIM11 PB9 HUM 
#define PWM_PSC   23
#define PWM_ARR   99

/*******************************************************
 * Temperature -> 4-20mA -> PWM (3.3V) -> 150 Ohm
 *******************************************************/



extern "C" {
/* ===================== CONSTANTS ===================== */

/* PWM */
#define PWM_MAX_DUTY          1000
#define PWM_REF_MV            3300

/* Hardware */
#define SHUNT_RESISTOR_OHM    150

/* 4–20 mA */
#define CURRENT_MIN_MA        4
#define CURRENT_MAX_MA        20

/* Temperature Range */
#define TEMP_MIN_C            0
#define TEMP_MAX_C            100

/* PWM duty limits (pre-calculated) */
#define DUTY_MIN              18   // 0.6V
#define DUTY_MAX              91   // 3.0V

#define HUM_MIN               0
#define HUM_MAX               100

#define TEMP_MIN_X100   0
#define TEMP_MAX_X100   100   // 100.00 °C

#define HUM_MIN_X100    0
#define HUM_MAX_X100    100   // 100.00 %


void pwm_init(void)
{
    /* ================= CLOCK ENABLE ================= */

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN |
                    RCC_AHB1ENR_GPIOBEN;

    RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;   // TIM5
    RCC->APB2ENR |= RCC_APB2ENR_TIM11EN;  // TIM11

    /* ================= GPIO CONFIG ================= */

    /* ---------- PA1 -> TIM5_CH2 (AF2) ---------- */
    GPIOA->MODER &= ~(3U << (1 * 2));
    GPIOA->MODER |=  (2U << (1 * 2));          // AF mode

    GPIOA->AFR[0] &= ~(0xFU << (1 * 4));
    GPIOA->AFR[0] |=  (2U << (1 * 4));          // AF2

    /* ---------- PB9 -> TIM11_CH1 (AF3) ---------- */
    GPIOB->MODER &= ~(3U << (9 * 2));
    GPIOB->MODER |=  (2U << (9 * 2));          // AF mode

    GPIOB->AFR[1] &= ~(0xFU << ((9 - 8) * 4));
    GPIOB->AFR[1] |=  (3U << ((9 - 8) * 4));    // AF3

    /* ================= TIM5 CONFIG ================= */

    TIM5->PSC = PWM_PSC;
    TIM5->ARR = PWM_ARR;

    TIM5->CCMR1 &= ~TIM_CCMR1_OC2M;
    TIM5->CCMR1 |=  (6U << TIM_CCMR1_OC2M_Pos); // PWM mode 1
    TIM5->CCMR1 |=  TIM_CCMR1_OC2PE;

    TIM5->CCER |= TIM_CCER_CC2E;

    TIM5->CCR2 = 0;

    TIM5->CR1 |= TIM_CR1_ARPE;
    TIM5->EGR  |= TIM_EGR_UG;
    TIM5->CR1 |= TIM_CR1_CEN;

    /* ================= TIM11 CONFIG ================= */

    TIM11->PSC = PWM_PSC;
    TIM11->ARR = PWM_ARR;

    TIM11->CCMR1 &= ~TIM_CCMR1_OC1M;
    TIM11->CCMR1 |=  (6U << TIM_CCMR1_OC1M_Pos); // PWM mode 1
    TIM11->CCMR1 |=  TIM_CCMR1_OC1PE;

    TIM11->CCER |= TIM_CCER_CC1E;

    TIM11->CCR1 = 0;

    TIM11->CR1 |= TIM_CR1_ARPE;
    TIM11->EGR  |= TIM_EGR_UG;
    TIM11->CR1 |= TIM_CR1_CEN;
}

/* ================= DUTY SET ================= */

void pwm_set_tim5(uint16_t duty)
{
    if (duty > 100) duty = 100;
    TIM5->CCR2 = (PWM_ARR + 1) * duty / 100;
}

void pwm_set_tim11(uint16_t duty)
{
    if (duty > 100) duty = 100;
    TIM11->CCR1 = (PWM_ARR + 1) * duty / 100;
}


/* ===================== CORE LOGIC ===================== */

/* Temperature -> Target Current (4–20 mA) */
uint32_t temperature_to_current_ma(int32_t temp_x100)
{
    if (temp_x100 <= TEMP_MIN_X100)
        return CURRENT_MIN_MA;

    if (temp_x100 >= TEMP_MAX_X100)
        return CURRENT_MAX_MA;

    return CURRENT_MIN_MA +
           ((uint32_t)(temp_x100 - TEMP_MIN_X100) *
           (CURRENT_MAX_MA - CURRENT_MIN_MA)) /
           (TEMP_MAX_X100 - TEMP_MIN_X100);
}

/* Humidity -> Target Current (4–20 mA) */
uint32_t humidity_to_current_ma(uint32_t hum_x100)
{
    if (hum_x100 <= HUM_MIN_X100)
        return CURRENT_MIN_MA;

    if (hum_x100 >= HUM_MAX_X100)
        return CURRENT_MAX_MA;

    return CURRENT_MIN_MA +
           ((hum_x100 - HUM_MIN_X100) *
           (CURRENT_MAX_MA - CURRENT_MIN_MA)) /
           (HUM_MAX_X100 - HUM_MIN_X100);
}

/* Target Current -> PWM Duty */
uint16_t current_to_pwm(uint32_t current_ma)
{
    if (current_ma <= CURRENT_MIN_MA)
        return DUTY_MIN;

    if (current_ma >= CURRENT_MAX_MA)
        return DUTY_MAX;

    return DUTY_MIN +
           ((current_ma - CURRENT_MIN_MA) *
           (DUTY_MAX - DUTY_MIN)) /
           (CURRENT_MAX_MA - CURRENT_MIN_MA);
}

/* ===================== ?? MAIN FUNCTION ===================== */
/*
 * Tek çagrilan fonksiyon
 * Temperature gir ? akimi sabitle ? PWM ayarla
 */
void set_temperature_output(uint32_t temp_x100)
{
    uint32_t target_current_ma;
    uint16_t pwm_duty;

    target_current_ma = temperature_to_current_ma(temp_x100);
    pwm_duty = current_to_pwm(target_current_ma);

    pwm_set_tim5(pwm_duty);
}

void set_humidity_output(uint32_t hum_x100)
{
    uint32_t target_current_ma;
    uint16_t pwm_duty;

    target_current_ma = humidity_to_current_ma(hum_x100);
    pwm_duty = current_to_pwm(target_current_ma);

    pwm_set_tim11(pwm_duty);
}
}
void DWT_Init(void) {
    // 1. Core Debug ünitesinde TRCENA bitini aktif et (Zorunludur)
    // Adres: 0xE000EDFC, Bit 24
    *((volatile uint32_t*)0xE000EDFC) |= (1 << 24);

    // 2. Bazi STM32F4 modellerinde DWT kilidini açmak gerekir (Software Lock)
    // LAR (Lock Access Register) adresi genelde 0xE0001FB0'dir.
    // 0xC5ACCE55 degeri kilidi açan sihirli anahtardir.
    *((volatile uint32_t*)0xE0001FB0) = 0xC5ACCE55;

    // 3. Cycle Counter (CYCCNT) register'ini sifirla
    DWT->CYCCNT = 0;

    // 4. CTRL register'indan sayaci baslat (0. bit: CYCCNTENA)
    DWT->CTRL |= 1; 
}