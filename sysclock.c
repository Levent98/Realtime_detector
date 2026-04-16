#include "stm32f410rx.h"
#include "system_stm32f4xx.h"
#include "sysclock.h"
#include <stdint.h>

/* ================= CLOCK CONFIG MACROS ================= */

#define CLOCK_TIMEOUT      (100000UL)
#define PLLP_DIV8    3  // Ikili karsiligi 11b


void setClock()
{
	  
    RCC->CIR = 0x00000000; // Tüm clock interruptlarini kapat ve temizle
    RCC->CR |= RCC_CR_HSEON; // 1. HSE'yi aç
    
    // HSE hazir olana kadar bekle (Güvenli bekleme)
    uint32_t timeout = 0xFFFF;
    while (!(RCC->CR & RCC_CR_HSERDY) && --timeout);
    
    if (timeout == 0) return; // Kristal kalkmadiysa zorlama, HSI ile devam et

    // 3. FLASH GECIKMESI (Bunu çok erkene aliyoruz)
    // 24-100 MHz arasi için 3WS her zaman güvenlidir.
    FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_LATENCY_1WS;
    
    // 4. Otobüs hizlarini ayarla
    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
    RCC->CFGR |= (RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV1 | RCC_CFGR_PPRE2_DIV1);

    // 5. PLL YAPILANDIRMASI (En güvenli yöntem: önce resetle)
    RCC->CR &= ~RCC_CR_PLLON; // Önce PLL kapali oldugundan emin ol
    while(RCC->CR & RCC_CR_PLLRDY); // Kapandigini teyit et

    // PLL_M=8, PLL_N=192, PLL_P=8, PLL_SRC=HSE
    // PLLP için 8 degeri "11" yani 3 binary degeridir.
    RCC->PLLCFGR = (8 << 0) | (192 << 6) | (3 << 16) | (RCC_PLLCFGR_PLLSRC_HSE) | (4 << 24);

    // 6. PLL'i aç ve KILITLENMESINI bekle
    RCC->CR |= RCC_CR_PLLON;
    timeout = 0xFFFF;
    while (!(RCC->CR & RCC_CR_PLLRDY) && --timeout);

    if (timeout > 0) {
        // 7. Sistemi PLL'e geçir
        RCC->CFGR &= ~RCC_CFGR_SW;
        RCC->CFGR |= RCC_CFGR_SW_PLL;
        // Geçisin tamamlandigini bekle
        while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
    }

    SystemCoreClockUpdate();
}
