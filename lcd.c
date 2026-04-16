#include "lcd.h"
#include "stm32f410rx.h"
#include "delay.h"
#include <stdint.h>
#include <stdio.h> // snprintf 
static inline void lcd_write_db4to7(uint8_t);
char final_buf[20];

/* Pin Eslesmeleri:
 * RS  -> PC1, RW  -> PC0, E   -> PC2
 * DB4 -> PC14, DB5 -> PC15, DB6 -> PB8, DB7 -> PC13
 */

/* Kisa makrolar */
#define RS_H()    (GPIOC->BSRR = GPIO_BSRR_BS_1)
#define RS_L()    (GPIOC->BSRR = GPIO_BSRR_BR_1)
#define RW_L()    (GPIOC->BSRR = GPIO_BSRR_BR_0)
#define E_H()     (GPIOC->BSRR = GPIO_BSRR_BS_2)
#define E_L()     (GPIOC->BSRR = GPIO_BSRR_BR_2)
#define BL_H()    (GPIOC->BSRR = GPIO_BSRR_BS_3)
#define BL_L()    (GPIOC->BSRR = GPIO_BSRR_BR_3)

/* Helper: tek seferde DB set/reset yapmak için bit maskleri hazirla */
static inline void lcd_write_db4to7(uint8_t nibble)
{
    uint32_t bsrrc = 0;
    uint32_t bsrrb = 0;

    // DB4 -> PC14
    if (nibble & 0x01) bsrrc |= (1U << 14);       // Set PC14
    else               bsrrc |= (1U << (14+16));  // Reset PC14

    // DB5 -> PC15
    if (nibble & 0x02) bsrrc |= (1U << 15);       // Set PC15
    else               bsrrc |= (1U << (15+16));  // Reset PC15

    // DB6 -> PB8 (Tek basina B portunda)
    if (nibble & 0x04) bsrrb |= (1U << 8);        // Set PB8
    else               bsrrb |= (1U << (8+16));   // Reset PB8

    // DB7 -> PC13
    if (nibble & 0x08) bsrrc |= (1U << 13);       // Set PC13
    else               bsrrc |= (1U << (13+16));  // Reset PC13

    GPIOC->BSRR = bsrrc;
    GPIOB->BSRR = bsrrb;
}

void lcd_send4(uint8_t nibble)
{
    lcd_write_db4to7(nibble);
    Delay_us(1); // Kurulum süresi (tAS)
    E_H();
    Delay_us(5); // Enable pulse genisligi (PWEH)
    E_L();
    Delay_us(5); // Bekleme
}
void lcd_cmd(uint8_t cmd)
{
    RS_L();
    RW_L();
    lcd_send4(cmd >> 4);   // Üst nibble
    lcd_send4(cmd & 0x0F); // Alt nibble
    
    // Clear ve Home komutlari daha uzun sürer
    if (cmd <= 0x02) Delay_ms(2);
    else             Delay_us(50);
}

void lcd_data(uint8_t data)
{
    RS_H();
    RW_L();
    lcd_send4(data >> 4);
    lcd_send4(data & 0x0F);
    Delay_us(50);
}
/* ... mevcut LCD_Clear, LCD_CreateChar, LCD_PrintFixed, LCD_Puts, LCD_Goto fonksiyonlarini aynen kullanabilirsin ... */
void LCD_Clear(void)
{
    lcd_cmd(0x01);   // Clear display
    Delay_ms(2);     // Datasheet: >= 1.52 ms
}
void LCD_Goto(uint8_t row, uint8_t col)
{
    uint8_t addr;

    if (row == 0) {
        addr = 0x80 + col;
    } else {
        addr = 0xC0 + col;  // 0x80 + 0x40
    }

    lcd_cmd(addr);
}
void LCD_Puts(const char *s)
{
    while (*s) {
        lcd_data((uint8_t)*s++);
    }
}
void LCD_CreateChar(uint8_t loc, const uint8_t *charmap)
{
    loc &= 0x07;  // 0..7

    // CGRAM address = 0x40 + (loc * 8)
    lcd_cmd(0x40 | (loc << 3));

    for (uint8_t i = 0; i < 8; i++) {
        lcd_data(charmap[i]);
    }
}

void LCD_PrintFixed(int32_t val, uint8_t frac) {
    //char final_buf[20]; 
    uint8_t p = 0;
    uint32_t abs_val;

    // 1. Hesaplama (Kesmeler AÇIK)
    if (val < 0) {
        final_buf[p++] = '-';
        abs_val = (uint32_t)(-(val + 1)) + 1U;
    } else {
        abs_val = (uint32_t)val;
    }

    uint32_t div = 1;
    for (uint8_t i = 0; i < frac; i++) div *= 10;
    uint32_t ip = abs_val / div;
    uint32_t fp = abs_val % div;

    char temp_ip[12];
    int8_t t_idx = 0;
    if (ip == 0) temp_ip[t_idx++] = '0';
    else {
        while (ip > 0) { temp_ip[t_idx++] = (char)((ip % 10) + '0'); ip /= 10; }
    }
    while (t_idx > 0) final_buf[p++] = temp_ip[--t_idx];

    if (frac > 0) {
        final_buf[p++] = '.';
        uint32_t temp_div = div / 10;
        while (temp_div > 0) {
            final_buf[p++] = (char)((fp / temp_div) + '0');
            fp %= temp_div;
            temp_div /= 10;
        }
    }
    
    // KRITIK: Bosluklari buradan sildik! 
    final_buf[p] = '\0'; 

    // 2. Yazdirma (Sadece bu blokta kesme kapali)


    for(uint8_t i = 0; final_buf[i] != '\0'; i++) {
        lcd_data(final_buf[i]);
        // Bekleme döngüsü (Hizin 24MHz ise 50-100 arasi idealdir)
        //for(volatile int d = 0; d < 60; d++); 
    }


}
void LCD_Init(void)
{
    /* --- ADIM 1: GÜÇ BEKLEMESI (Wait for VCC to rise to 3.3V/5V) --- */
    /* Datasheet >40ms der, ancak stabilite için 150ms en güvenlisidir. */
    Delay_ms(60); 

    /* --- ADIM 2: DONANIM YAPILANDIRMASI --- */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIOBEN;
    (void)RCC->AHB1ENR; // Dummy read for bus synchronization

    // PC0,1,2,3, 13,14,15 ve PB8'i temizle ve Output yap (01)
    GPIOC->MODER &= ~((3U << 0)|(3U << 2)|(3U << 4)|(3U << 6)|(3U << 26)|(3U << 28)|(3U << 30));
    GPIOC->MODER |=  ((1U << 0)|(1U << 2)|(1U << 4)|(1U << 6)|(1U << 26)|(1U << 28)|(1U << 30));
    GPIOB->MODER &= ~(3U << 16);
    GPIOB->MODER |=  (1U << 16);

    // Hiz ayari: Low Speed (LCD yavas bir cihazdir, gürültüyü önler)
    GPIOC->OSPEEDR &= ~((3U << 0)|(3U << 2)|(3U << 4)|(3U << 6)|(3U << 26)|(3U << 28)|(3U << 30));
    GPIOB->OSPEEDR &= ~(3U << 16);

    // Baslangiç pin durumlari
    RW_L();  // Daima Write modunda
    RS_L();  // Komut modu
    E_L();   
    BL_H();  // Backlight ON

    /* --- ADIM 3: 4-BIT BASLATMA DIZISI (ST7066U Fig. 24) --- */
    /* Bu kisim LCD'yi 8-bit modundan 4-bit moduna senkronize eder. */
    
    // 1. Deneme: 0x03 gönderilir (LCD henüz hangi modda oldugunu bilmiyor)
    lcd_send4(0x03); 
    Delay_ms(5);   // Datasheet: >4.1ms bekleyin

    // 2. Deneme: 0x03 tekrar gönderilir
    lcd_send4(0x03); 
    Delay_us(200); // Datasheet: >100us bekleyin

    // 3. Deneme: 0x03 son kez (Senkronizasyon garantilenir)
    lcd_send4(0x03); 
    Delay_us(200);

    // 4. Deneme: 0x02 gönderilerek 4-BIT arayüzü seçilir
    lcd_send4(0x02); 
    Delay_ms(2);   // Mod degisimi için bekleme

    /* --- ADIM 4: YAZILIMSAL YAPILANDIRMA (Table of Commands) --- */
    
    // Function Set: N=1 (2 Satir), F=0 (5x8 dots) -> 0x28
    lcd_cmd(0x28); 
    
    // Display ON/OFF: D=0, C=0, B=0 (Display OFF iken ayar yapmak paraziti önler)
    lcd_cmd(0x08); 
    
    // Display Clear: Ekran hafizasini temizle
    lcd_cmd(0x01); 
    Delay_ms(2);   // Temizleme komutu uzundur (Min 1.52ms)
    
    // Entry Mode Set: I/D=1 (Saga kaydir), S=0 (Ekran kaydirma kapali)
    lcd_cmd(0x06); 
    
    // Display ON/OFF: D=1 (Display ON), C=0 (Cursor OFF), B=0 (Blink OFF)
    lcd_cmd(0x0C); 

    Delay_ms(2); // Son stabilizasyon
}
/* Basit test fonksiyonu: cursor'a "HELLO" yaz */
void LCD_Test(void)
{
    LCD_Goto(0,0);
    LCD_Puts("HELLO");
    LCD_Goto(1,0);
    LCD_Puts("STM32");
}
