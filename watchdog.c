#include "stm32f410rx.h"
#include "watchdog.h"

/*
 * IWDG yaklasik LSI (~32 kHz) ile çalisir.
 *
 * Timeout ˜ (RLR + 1) * Prescaler / LSI
 *
 * PR = 64
 * RLR = 999
 *
 * Timeout ˜ (1000 * 64) / 32000 = 2.0 s
 */

void IWDG_Init_2s(void)
{
    /* Start IWDG */
    IWDG->KR = 0xCCCCU;

    /* Enable write access */
    IWDG->KR = 0x5555U;

    /* Prescaler = 64 */
    IWDG->PR = 0x04U;

    /* Reload = 999 */
    IWDG->RLR = 999U;

    /* Wait until registers update */
    while ((IWDG->SR & (IWDG_SR_PVU | IWDG_SR_RVU)) != 0U)
    {
        /* wait */
    }

    /* Reload counter */
    IWDG->KR = 0xAAAAU;
}

void IWDG_Kick(void)
{
    IWDG->KR = 0xAAAAU;
}