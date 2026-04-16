#include "stm32f410rx.h"
#include "i2c.h"
#include <stdint.h>

/* ================= CONFIG ================= */
#define I2C_TIMEOUT        (100000UL)
#define PCLK1_FREQ_MHZ     (24U)
#define I2C_SPEED_HZ       (100000U)
#define NULL ((void *)0)


/* ================= INIT ================= */
void I2C1_Config(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    /* PB6=SCL, PB7=SDA */
    GPIOB->MODER &= ~((3U<<(6*2)) | (3U<<(7*2)));
    GPIOB->MODER |=  ((2U<<(6*2)) | (2U<<(7*2)));   // AF

    GPIOB->OTYPER |= (1U<<6) | (1U<<7);             // Open-drain
// GPIOB->OSPEEDR |= (3U<<(6*2)) | (3U<<(7*2)); // Bunu sil
    GPIOB->OSPEEDR &= ~((3U<<(6*2)) | (3U<<(7*2))); // Temizle
    GPIOB->OSPEEDR |=  ((1U<<(6*2)) | (1U<<(7*2))); // Medium Speed (100kHz için daha iyi)

    GPIOB->AFR[0] &= ~((0xFU<<24)|(0xFU<<28));
    GPIOB->AFR[0] |=  ((4U<<24)|(4U<<28));          // AF4 = I2C

    /* I2C reset */
    I2C1->CR1 &= ~I2C_CR1_PE;

    /* APB1 = 24 MHz */
    I2C1->CR2   = 24U;
    I2C1->CCR   = 120U;    // 100 kHz standard mode
    I2C1->TRISE = 25U;

    I2C1->CR1 |= I2C_CR1_PE;
}

void I2C1_BusRecover(void) {
    // 1. I2C birimini kapat
    I2C1->CR1 &= ~I2C_CR1_PE;

    // 2. Pinleri manuel kontrol etmek için GPIO moduna al (SCL çikis, SDA giris)
    GPIOB->MODER &= ~((3U << 12) | (3U << 14));
    GPIOB->MODER |= ((1U << 12) | (0U << 14)); // PB6 Output (SCL), PB7 Input (SDA)
    GPIOB->OSPEEDR &= ~((3U<<(6*2)) | (3U<<(7*2)));
    GPIOB->OSPEEDR |=  ((1U<<(6*2)) | (1U<<(7*2))); // Medium
    // 3. 9 adet clock darbesi gönder (SDA kilitliyse sensörü zorla bosa çikartir)
    for (int i = 0; i < 9; i++) {
        GPIOB->BSRR = (1U << 6);   // SCL HIGH
        for(volatile int d=0; d<100; d++);
        GPIOB->BSRR = (1U << 22);  // SCL LOW
        for(volatile int d=0; d<100; d++);
    }

    // 4. Pinleri tekrar I2C (Alternate Function) moduna al
    GPIOB->MODER &= ~((3U << 12) | (3U << 14));
    GPIOB->MODER |= ((2U << 12) | (2U << 14));

    // 5. I2C Birimini Resetle ve Yeniden Baslat
    I2C1->CR1 |= I2C_CR1_SWRST;
    for(volatile int d=0; d<100; d++);
    I2C1->CR1 &= ~I2C_CR1_SWRST;

    // Ayarlari (CCR, TRISE vb.) tekrar yükle
    I2C1->CR2 = 24;
    I2C1->CCR = 120;
    I2C1->TRISE = 25;
    I2C1->CR1 |= I2C_CR1_PE;
}
/*================ START =================*/
/*
 * START condition üretir
 * SB (Start Bit) set olana kadar bekler
 */
I2C_StatusType I2C_Start(void)
{
uint32_t timeout = I2C_TIMEOUT;

    // Eger BUSY ise ve hat gerçekten mesgulse kurtar
    if (I2C1->SR2 & I2C_SR2_BUSY) {
        I2C1_BusRecover();
    }
    
    // Hala BUSY ise donanimi kapat-aç
    if (I2C1->SR2 & I2C_SR2_BUSY) {
        I2C1->CR1 &= ~I2C_CR1_PE;
        for(volatile int d=0; d<100; d++);
        I2C1->CR1 |= I2C_CR1_PE;
    }
    I2C1->CR1 |= I2C_CR1_START;

    timeout = I2C_TIMEOUT;
    while ((I2C1->SR1 & I2C_SR1_SB) == 0U)
        if (timeout-- == 0U) return I2C_ERROR_TIMEOUT;

    (void)I2C1->SR1;
    return I2C_OK;
}


/*================ ADDRESS =================*/
/*
 * Slave adresini gönderir
 * ADDR flag set olunca adres kabul edilmistir
 */
I2C_StatusType I2C_SendAddress(uint8_t addr)
{
    uint32_t timeout = I2C_TIMEOUT;

    I2C1->DR = addr;

    while ((I2C1->SR1 & I2C_SR1_ADDR) == 0U)
    {
        if (I2C1->SR1 & I2C_SR1_AF)
        {
            I2C1->SR1 &= ~I2C_SR1_AF;
            I2C1->CR1 |= I2C_CR1_STOP;
            return I2C_ERROR_NACK;
        }

        if (timeout-- == 0U)
            return I2C_ERROR_TIMEOUT;
    }

    (void)I2C1->SR1;
    (void)I2C1->SR2;
    return I2C_OK;
}


I2C_StatusType I2C_WriteByte(uint8_t data)
{
    uint32_t timeout = I2C_TIMEOUT;

    while ((I2C1->SR1 & I2C_SR1_TXE) == 0U)
    {
        if (timeout-- == 0U)
            return I2C_ERROR_TIMEOUT;
    }

    I2C1->DR = data;

    timeout = I2C_TIMEOUT;
    while ((I2C1->SR1 & I2C_SR1_BTF) == 0U)
    {
        if (timeout-- == 0U)
            return I2C_ERROR_TIMEOUT;
    }

    return I2C_OK;
}


/*================ READ =================*/
/*
 * Slave’ten 1 byte veri okur
 * ack = 1  ACK gönder
 * ack = 0  NACK gönder (son byte için)
 */
I2C_StatusType I2C_ReadByte(uint8_t *data, uint8_t ack)
{
    uint32_t timeout = I2C_TIMEOUT;

    if (ack)
    {
        /* Devam edecek -> ACK */
        I2C1->CR1 |= I2C_CR1_ACK;
    }
    else
    {
        /* SON BYTE */
        I2C1->CR1 &= ~I2C_CR1_ACK;
        I2C1->CR1 |= I2C_CR1_STOP;   // <-- ISTE BURASI
    }

    /* Byte gelsin */
    while ((I2C1->SR1 & I2C_SR1_RXNE) == 0U)
    {
        if (timeout-- == 0U)
            return I2C_ERROR_TIMEOUT;
    }

    *data = (uint8_t)I2C1->DR;
    return I2C_OK;
}


/*================ STOP =================*/
/*
 * STOP condition üretir
 * BUSY flag temizlenene kadar bekler
 */
I2C_StatusType I2C_Stop(void)
{
    I2C1->CR1 |= I2C_CR1_STOP;
    return I2C_OK;
}

I2C_StatusType I2C_ReadData(uint8_t slaveAddr, uint8_t *buffer, uint16_t size)
{
    uint32_t timeout;

if ((buffer == NULL) || (size == 0U))
    return I2C_ERROR_BUS;

    /* --- BUSY kontrolü --- */
    timeout = I2C_TIMEOUT;
if (I2C1->SR2 & I2C_SR2_BUSY)
    return I2C_ERROR_BUS;

    /* --- START --- */
    I2C1->CR1 |= I2C_CR1_START;

    timeout = I2C_TIMEOUT;
    while (!(I2C1->SR1 & I2C_SR1_SB))
    {
        if (timeout-- == 0U)
            return I2C_ERROR_TIMEOUT;
    }

    /* --- ADDRESS (READ) --- */
    I2C1->DR = (slaveAddr << 1) | 1U;

    timeout = I2C_TIMEOUT;
    while (!(I2C1->SR1 & I2C_SR1_ADDR))
    {
        if (I2C1->SR1 & I2C_SR1_AF)
        {
            I2C1->SR1 &= ~I2C_SR1_AF;   // AF clear
            I2C1->CR1 |= I2C_CR1_STOP;  // STOP gönder
            return I2C_ERROR_NACK;
        }

        if (timeout-- == 0U)
        {
            I2C1->CR1 |= I2C_CR1_STOP;
            return I2C_ERROR_TIMEOUT;
        }
    }

    /* ================= 1 BYTE ================= */
    if (size == 1U)
    {
        I2C1->CR1 &= ~I2C_CR1_ACK;

        __disable_irq();
        (void)I2C1->SR1;
        (void)I2C1->SR2;
        I2C1->CR1 |= I2C_CR1_STOP;
        __enable_irq();

        timeout = I2C_TIMEOUT;
        while (!(I2C1->SR1 & I2C_SR1_RXNE))
        {
            if (timeout-- == 0U)
                return I2C_ERROR_TIMEOUT;
        }

        buffer[0] = (uint8_t)I2C1->DR;
    }

    /* ================= 2 BYTE ================= */
    else if (size == 2U)
    {
			  __disable_irq();
        I2C1->CR1 |= I2C_CR1_POS;
        I2C1->CR1 &= ~I2C_CR1_ACK;

        
        (void)I2C1->SR1;
        (void)I2C1->SR2;
        __enable_irq();

        timeout = I2C_TIMEOUT;
        while (!(I2C1->SR1 & I2C_SR1_BTF))
        {
            if (timeout-- == 0U)
                return I2C_ERROR_TIMEOUT;
        }

        __disable_irq();
        I2C1->CR1 |= I2C_CR1_STOP;
        buffer[0] = (uint8_t)I2C1->DR;
        __enable_irq();

        buffer[1] = (uint8_t)I2C1->DR;

        I2C1->CR1 &= ~I2C_CR1_POS;
    }

    /* ================= 3+ BYTE ================= */
    else
    {
        I2C1->CR1 |= I2C_CR1_ACK;

        (void)I2C1->SR1;
        (void)I2C1->SR2;

        uint16_t i = 0U;

        for (i = 0U; i < size - 3U; i++)
        {
            timeout = I2C_TIMEOUT;
            while (!(I2C1->SR1 & I2C_SR1_RXNE))
            {
                if (timeout-- == 0U)
                {
                    I2C1->CR1 |= I2C_CR1_STOP;
                    return I2C_ERROR_TIMEOUT;
                }
            }
            buffer[i] = (uint8_t)I2C1->DR;
        }

        timeout = I2C_TIMEOUT;
        while (!(I2C1->SR1 & I2C_SR1_BTF))
        {
            if (timeout-- == 0U)
            {
                I2C1->CR1 |= I2C_CR1_STOP;
                return I2C_ERROR_TIMEOUT;
            }
        }

        I2C1->CR1 &= ~I2C_CR1_ACK;

        __disable_irq();
        buffer[i++] = (uint8_t)I2C1->DR;   // N-2
        I2C1->CR1 |= I2C_CR1_STOP;
        __enable_irq();

        timeout = I2C_TIMEOUT;
        while (!(I2C1->SR1 & I2C_SR1_RXNE))
        {
            if (timeout-- == 0U)
                return I2C_ERROR_TIMEOUT;
        }
        buffer[i++] = (uint8_t)I2C1->DR;   // N-1

        timeout = I2C_TIMEOUT;
        while (!(I2C1->SR1 & I2C_SR1_RXNE))
        {
            if (timeout-- == 0U)
                return I2C_ERROR_TIMEOUT;
        }
        buffer[i++] = (uint8_t)I2C1->DR;   // N
    }

    I2C1->CR1 |= I2C_CR1_ACK;
    I2C1->CR1 &= ~I2C_CR1_POS;
    return I2C_OK; //calisan keil
}
I2C_StatusType I2C_WriteData(uint8_t slaveAddr, uint8_t *data, uint16_t size)
{
    uint32_t timeout;
    uint16_t i;

    if ((data == NULL) || (size == 0U)) {
        return I2C_ERROR_BUS;
    }

if (I2C1->SR2 & I2C_SR2_BUSY)
    return I2C_ERROR_BUS;

    timeout = I2C_TIMEOUT;
    while (I2C1->SR2 & I2C_SR2_BUSY) {
        if (timeout-- == 0U) return I2C_ERROR_BUS;
    }

    I2C1->CR1 |= I2C_CR1_START;
    timeout = I2C_TIMEOUT;
    while (!(I2C1->SR1 & I2C_SR1_SB)) {
        if (timeout-- == 0U) {
            I2C1->CR1 |= I2C_CR1_STOP;
            return I2C_ERROR_TIMEOUT;
        }
    }

    I2C1->DR = (uint8_t)(slaveAddr << 1);

    timeout = I2C_TIMEOUT;
    while (!(I2C1->SR1 & I2C_SR1_ADDR)) {
        if (I2C1->SR1 & I2C_SR1_AF) {
            I2C1->SR1 &= ~I2C_SR1_AF;
            I2C1->CR1 |= I2C_CR1_STOP;
            return I2C_ERROR_NACK;
        }
        if (timeout-- == 0U) {
            I2C1->CR1 |= I2C_CR1_STOP;
            return I2C_ERROR_TIMEOUT;
        }
    }

    (void)I2C1->SR1;
    (void)I2C1->SR2;

    for (i = 0U; i < size; i++) {
        timeout = I2C_TIMEOUT;
        while (!(I2C1->SR1 & I2C_SR1_TXE)) {
            if (I2C1->SR1 & I2C_SR1_AF) {
                I2C1->SR1 &= ~I2C_SR1_AF;
                I2C1->CR1 |= I2C_CR1_STOP;
                return I2C_ERROR_NACK;
            }
            if (timeout-- == 0U) {
                I2C1->CR1 |= I2C_CR1_STOP;
                return I2C_ERROR_TIMEOUT;
            }
        }
        I2C1->DR = data[i];
    }

    timeout = I2C_TIMEOUT;
    while (!(I2C1->SR1 & I2C_SR1_BTF)) {
        if (I2C1->SR1 & I2C_SR1_AF) {
            I2C1->SR1 &= ~I2C_SR1_AF;
            I2C1->CR1 |= I2C_CR1_STOP;
            return I2C_ERROR_NACK;
        }
        if (timeout-- == 0U) {
            I2C1->CR1 |= I2C_CR1_STOP;
            return I2C_ERROR_TIMEOUT;
        }
    }

    I2C1->CR1 |= I2C_CR1_STOP;

    timeout = I2C_TIMEOUT;
    while (I2C1->SR2 & I2C_SR2_BUSY) {
        if (timeout-- == 0U) {
            return I2C_ERROR_TIMEOUT;
        }
    }

    return I2C_OK;
}
I2C_StatusType I2C_WriteData_Wrapper(uint8_t slaveAddr, uint8_t *data, uint16_t size)
{
    I2C_StatusType status;

    status = I2C_WriteData(slaveAddr, data, size);

    if ((status == I2C_ERROR_TIMEOUT) || (status == I2C_ERROR_BUS))
    {
        I2C1_BusRecover();
        status = I2C_WriteData(slaveAddr, data, size);
    }

    return status;
}

I2C_StatusType I2C_ReadData_Wrapper(uint8_t slaveAddr, uint8_t *buffer, uint16_t size)
{
    I2C_StatusType status;

    status = I2C_ReadData(slaveAddr, buffer, size);

    if ((status == I2C_ERROR_TIMEOUT) || (status == I2C_ERROR_BUS))
    {
        I2C1_BusRecover();
        status = I2C_ReadData(slaveAddr, buffer, size);
    }

    return status;
}
