#ifndef ADC_H
#define ADC_H

#include <stdint.h>

#define ADC_CH_TEMP   3U   /* PA3 = ADC1_IN3 */
#define ADC_CH_HUM    4U   /* PA4 = ADC1_IN4 */

void ADC1_Init_PA3_PA4(void);
uint16_t ADC1_ReadChannel(uint8_t channel);
void ADC_Task_Process(void);
void ADC1_DMA_Init(void);
#endif