#include "oled_drv.h"
#include "font.h"
#include "i2c.h"
#include <string.h>

extern I2C_HandleTypeDef hi2c1;
#define OLED_HANDLE &hi2c1

#define OLED_TIMEOUT 1000

static HAL_StatusTypeDef oled_send_cmd(uint8_t cmd)
{
    uint8_t buf[2] = { OLED_CMD, cmd };
    return HAL_I2C_Master_Transmit(OLED_HANDLE, OLED_ADDR, buf, 2, OLED_TIMEOUT);
}

static HAL_StatusTypeDef oled_send_data(uint8_t data)
{
    uint8_t buf[2] = { OLED_DATA, data };
    return HAL_I2C_Master_Transmit(OLED_HANDLE, OLED_ADDR, buf, 2, OLED_TIMEOUT);
}

static HAL_StatusTypeDef oled_send_data_batch(const uint8_t* data, uint16_t len)
{
    if (!data || len == 0) return HAL_ERROR;
    return HAL_I2C_Mem_Write(OLED_HANDLE, OLED_ADDR, OLED_DATA,
                             I2C_MEMADD_SIZE_8BIT, (uint8_t*)data, len, OLED_TIMEOUT);
}

static void oled_set_pos(uint8_t page, uint8_t col)
{
    if (page > 7 || col > 127) return;
    oled_send_cmd(0xB0 + page);
    oled_send_cmd(0x00 + (col & 0x0F));
    oled_send_cmd(0x10 + ((col >> 4) & 0x0F));
}

void oled_init(void)
{
    HAL_Delay(100);
    oled_send_cmd(0xAE);
    oled_send_cmd(0xD5); oled_send_cmd(0x80);
    oled_send_cmd(0xA8); oled_send_cmd(0x3F);
    oled_send_cmd(0xD3); oled_send_cmd(0x00);
    oled_send_cmd(0x40);
    oled_send_cmd(0x8D); oled_send_cmd(0x14);
    oled_send_cmd(0x20); oled_send_cmd(0x02);
    oled_send_cmd(0xA1);
    oled_send_cmd(0xC8);
    oled_send_cmd(0xDA); oled_send_cmd(0x12);
    oled_send_cmd(0x81); oled_send_cmd(0xCF);
    oled_send_cmd(0xD9); oled_send_cmd(0xF1);
    oled_send_cmd(0xDB); oled_send_cmd(0x40);
    oled_send_cmd(0xA4);
    oled_send_cmd(0xA6);
    oled_send_cmd(0xAF);
}

void oled_clear(void)
{
    uint8_t zero[128] = {0};
    for (uint8_t page = 0; page < 8; page++)
    {
        oled_send_cmd(0xB0 + page);
        oled_send_cmd(0x00);
        oled_send_cmd(0x10);
        oled_send_data_batch(zero, 128);
    }
}

void oled_print_8x8(uint8_t x, uint8_t y, const uint8_t* str)
{
    if (x > 127 || y > 7 || !str || *str == '\0') return;

    uint8_t page = y;
    uint8_t col  = x;

    while (*str)
    {
        uint8_t idx = *str - 0x20;
        if (idx > 95) idx = 0;

        if (col + 8 > 127) { page++; col = 0; if (page > 7) return; }

        oled_set_pos(page, col);
        oled_send_data_batch(asc2_8x8[idx], 8);
        col += 8;
        str++;
    }
}

void oled_print_16x8(uint8_t x, uint8_t y, const uint8_t* str)
{
    if (x > 127 || y > 7 || !str || *str == '\0') return;

    uint8_t page = y;
    uint8_t col  = x;

    while (*str)
    {
        uint8_t idx = *str;
        if (idx > 127) idx = 0;

        if (col + 16 > 127) { page += 2; col = 0; if (page + 1 > 7) return; }

        oled_set_pos(page, col);
        oled_send_data_batch(ascii_font[idx], 8);
        oled_set_pos(page + 1, col);
        oled_send_data_batch(&ascii_font[idx][8], 8);

        col += 8;
        str++;
    }
}
