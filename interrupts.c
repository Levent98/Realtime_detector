#include "stm32f410rx.h"
#include "uart.h"
// Global flag: ATEX safe durumu
volatile uint8_t atex_safe = 0;
volatile uint32_t g_tick = 0;
volatile uint32_t nmi_sayac = 0;
#define CLOCK_TIMEOUT      (100000UL)

/* -------------------------------
   ATEX Safe State Fonksiyonu
   PWM’leri durdurur, pinleri LOW yapar
---------------------------------*/
void ATEX_SafeState(void)
{
    // 1. PWM timerlarini durdur
    TIM5->CR1  &= ~TIM_CR1_CEN;   // PA1
    TIM11->CR1 &= ~TIM_CR1_CEN;   // PB9

    // 2. PA1 -> Output LOW
    GPIOA->MODER &= ~(3U << (1 * 2));  // AF temizle
    GPIOA->MODER |=  (1U << (1 * 2));  // Output mode
    GPIOA->ODR   &= ~(1U << 1);         // LOW

    // 3. PB9 -> Output LOW
    GPIOB->MODER &= ~(3U << (9 * 2));  // AF temizle
    GPIOB->MODER |=  (1U << (9 * 2));  // Output mode
    GPIOB->ODR   &= ~(1U << 9);         // LOW

    // 4. ATEX safe flag
    atex_safe = 1;
}

/* -------------------------------
   HSI’ye geçis fonksiyonu
---------------------------------*/
void SwitchToHSI(void)
{
    uint32_t timeout = CLOCK_TIMEOUT;
    //sysclk , apb1,apb2 hatlari 24mhzde calismaya devam eder
    // 1. HSI'yi aç
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY) && (timeout--));

    // 2. HSE veya PLL varsa kapat
    RCC->CR &= ~RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) != 0);

    // 3. PLL konfigürasyonu (HSI kaynak)
    RCC->PLLCFGR =
        (16U << RCC_PLLCFGR_PLLM_Pos) |  // M = 16 (HSI 16MHz)
        (192U << RCC_PLLCFGR_PLLN_Pos) | // N = 192
        (((8U/2 - 1) & 0x3) << RCC_PLLCFGR_PLLP_Pos) | // P = 8
        (4U << RCC_PLLCFGR_PLLQ_Pos);    // Q = 4 (CubeMX örnek)

    RCC->PLLCFGR |= RCC_PLLCFGR_PLLSRC_HSI; // PLL kaynagi HSI

    // 4. Prescaler ayarlari
    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV1 | RCC_CFGR_PPRE2_DIV1;

    // 5. PLL enable
    RCC->CR |= RCC_CR_PLLON;
    timeout = CLOCK_TIMEOUT;
    while (!(RCC->CR & RCC_CR_PLLRDY) && (timeout--));

    // 6. PLL’i SYSCLK olarak seç
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;

    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}

/* -------------------------------
   NMI Handler – CSS tetiklenirse
---------------------------------*/
void NMI_Handler(void){
	
	if (RCC->CIR & RCC_CIR_CSSF) {
        RCC->CIR |= RCC_CIR_CSSC; // CSS bayragini temizle
    } 
nmi_sayac++; // Her giriste bu sayi artacak
    while (1) {
        // Islemci burada takilacak
    }	
	
}
