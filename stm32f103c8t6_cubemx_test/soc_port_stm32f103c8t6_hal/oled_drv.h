#ifndef OLED_DRV_H
#define OLED_DRV_H

#include <stdint.h>

/* I2C 地址 (7 位地址左移 1 位) */
#define OLED_ADDR   0x78
#define OLED_CMD    0x00
#define OLED_DATA   0x40

void oled_init(void);
void oled_clear(void);
void oled_print_8x8(uint8_t x, uint8_t y, const uint8_t* str);
void oled_print_16x8(uint8_t x, uint8_t y, const uint8_t* str);

#endif
