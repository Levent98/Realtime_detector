#ifndef LCD_H
#define LCD_H

#include <stdint.h>

void LCD_Init(void);
void LCD_Clear(void);
void LCD_Goto(uint8_t r, uint8_t c);
void LCD_Puts(const char *s);
void lcd_data(uint8_t d);
void lcd_cmd(uint8_t c);
void LCD_CreateChar(uint8_t location, const uint8_t *charmap);
void LCD_PrintFixed(int32_t val, uint8_t frac);
void LCD_Test(void);

#endif
