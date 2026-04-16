#include "stm32f410rx.h"
#include "adc.h"
#include "modbus_rtu.h"

#include "stm32f410rx.h"
#include "adc.h"
#include "modbus_rtu.h"
volatile uint16_t adc_buffer[2];
void ADC1_Init_PA3_PA4(void)
{
    /* GPIOA clock enable */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    (void)RCC->AHB1ENR;

    /* ADC1 clock enable */
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    (void)RCC->APB2ENR;

    /* PA3, PA4 -> analog mode */
    GPIOA->MODER |= (3U << (3U * 2U)) | (3U << (4U * 2U));

    /* No pull-up / pull-down */
    GPIOA->PUPDR &= ~((3U << (3U * 2U)) | (3U << (4U * 2U)));

    /* ADC kapaliyken ayarla */
    ADC1->CR1 = 0U;
    ADC1->CR2 = 0U;

    /* 12-bit çözünürlük (default zaten 12-bit) */

    /* Tek conversion */
    ADC1->SQR1 &= ~ADC_SQR1_L;

    /*
     * Sampling time:
     * CH3 -> SMPR2 bits [11:9]
     * CH4 -> SMPR2 bits [14:12]
     * 111 = 480 cycles (daha stabil ölçüm)
     */
    ADC1->SMPR2 |= (7U << 9U) | (7U << 12U);

    /* ADC ON */
    ADC1->CR2 |= ADC_CR2_ADON;

    /* ADC stabilizasyon bekleme */
    for (volatile int i = 0; i < 1000; i++);
}

uint16_t ADC1_ReadChannel(uint8_t channel)
{
    if (channel > 15U)
        return 0U;

    /* Sequence length = 1 */
    ADC1->SQR1 &= ~ADC_SQR1_L;

    /* Kanal seç */
    ADC1->SQR3 = channel;

    /* Conversion baslat */
    ADC1->CR2 |= ADC_CR2_SWSTART;

    /* Bitmesini bekle */
    while (!(ADC1->SR & ADC_SR_EOC));

    /* DR okununca EOC otomatik temizlenir */
    return (uint16_t)ADC1->DR;
}
void ADC1_DMA_Init(void)
{
    /* --- CLOCK ENABLE --- */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    (void)RCC->AHB1ENR;
    (void)RCC->APB2ENR;

    /* --- GPIO ANALOG (PA3, PA4) --- */
    GPIOA->MODER |= (3U << (3U * 2U)) | (3U << (4U * 2U));
    GPIOA->PUPDR &= ~((3U << (3U * 2U)) | (3U << (4U * 2U)));

    /* --- DMA2 STREAM0 CHANNEL0 (ADC1) --- */
    DMA2_Stream0->CR = 0;
    while (DMA2_Stream0->CR & DMA_SxCR_EN);

    DMA2_Stream0->PAR  = (uint32_t)&ADC1->DR;
    DMA2_Stream0->M0AR = (uint32_t)adc_buffer;
    DMA2_Stream0->NDTR = 2;

    DMA2_Stream0->CR =
          (0U << 25) |     /* Channel 0 */
          DMA_SxCR_MINC |  /* Memory increment */
          DMA_SxCR_CIRC |  /* Circular mode */
          DMA_SxCR_PSIZE_0 | /* 16-bit peripheral */
          DMA_SxCR_MSIZE_0 | /* 16-bit memory */
          DMA_SxCR_PL_1;     /* High priority */

    DMA2_Stream0->CR |= DMA_SxCR_EN;

    /* --- ADC CONFIG --- */
    ADC1->CR1 = ADC_CR1_SCAN;   /* Scan mode */

    ADC1->CR2 =
          ADC_CR2_DMA |
          ADC_CR2_DDS |   /* DMA continuous request */
          ADC_CR2_CONT;   /* Continuous mode */

    /* Sampling time (yüksek = stabil) */
    ADC1->SMPR2 |= (7U << 9U) | (7U << 12U);

    /* Sequence: 2 conversions */
    ADC1->SQR1 = (1U << 20);  /* L = 1 => 2 kanal */

    /* Sira: */
    ADC1->SQR3 =
          (3U << 0) |   /* 1. sirada CH3 */
          (4U << 5);    /* 2. sirada CH4 */

    /* ADC ON */
    ADC1->CR2 |= ADC_CR2_ADON;

    /* kisa bekleme */
    for (volatile int i = 0; i < 1000; i++);

    /* Start */
    ADC1->CR2 |= ADC_CR2_SWSTART;
}


void ADC_Task_Process(void)
{
    uint16_t raw_ch3 = adc_buffer[0];
    uint16_t raw_ch4 = adc_buffer[1];

    /* PA4 = voltaj ölçüm */
    float v_supply = (float)raw_ch4 * 0.008864468f;

    uint16_t v_modbus = (uint16_t)(v_supply * 10.0f);

    Modbus_SetRegister(REG_IN_VOL_VALUE, v_modbus);
}