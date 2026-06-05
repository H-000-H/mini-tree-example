#include "wifi_esp32.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>

#define ESP32_TIMEOUT  1000

static UART_HandleTypeDef huart2;

/* USART2 GPIO: TX=PA2, RX=PA3 */
static void mx_usart2_uart_init(void)
{
    if (huart2.Instance) return;  /* already initialized */

    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    /* TX: PA2 push-pull alternate */
    gpio.Pin       = GPIO_PIN_2;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);
    /* RX: PA3 floating input */
    gpio.Pin       = GPIO_PIN_3;
    gpio.Mode      = GPIO_MODE_INPUT;
    gpio.Pull      = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    huart2.Instance          = USART2;
    huart2.Init.BaudRate     = 115200;
    huart2.Init.WordLength   = UART_WORDLENGTH_8B;
    huart2.Init.StopBits     = UART_STOPBITS_1;
    huart2.Init.Parity       = UART_PARITY_NONE;
    huart2.Init.Mode         = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

static uint8_t s_resp_buf[ESP32_RX_BUF_SIZE];
static uint16_t s_resp_len;

void wifi_esp32_init(void)
{
    mx_usart2_uart_init();

    /* 软件复位 ESP32 */
    wifi_esp32_send_cmd((const uint8_t*)"AT+RST\r\n", 8);
    HAL_Delay(3000);

    /* 关闭回显 */
    wifi_esp32_send_cmd((const uint8_t*)"ATE0\r\n", 6);
    HAL_Delay(500);
}

void wifi_esp32_send_cmd(const uint8_t* cmd, uint16_t len)
{
    memset(s_resp_buf, 0, ESP32_RX_BUF_SIZE);
    s_resp_len = 0;

    HAL_UART_Transmit(&huart2, (uint8_t*)cmd, len, ESP32_TIMEOUT);

    /* 读响应 (阻塞, 最大等 2s) */
    HAL_UARTEx_ReceiveToIdle(&huart2, s_resp_buf, ESP32_RX_BUF_SIZE - 1,
                             &s_resp_len, 2000);
    if (s_resp_len < ESP32_RX_BUF_SIZE)
        s_resp_buf[s_resp_len] = '\0';
    else
        s_resp_buf[ESP32_RX_BUF_SIZE - 1] = '\0';
}

int wifi_esp32_get_response(uint8_t* buf, uint16_t* len)
{
    if (!buf || !len) return -1;
    if (s_resp_len == 0) return -1;

    if (*len > s_resp_len)
        *len = s_resp_len;
    memcpy(buf, s_resp_buf, *len);
    return 0;
}

void* wifi_esp32_get_uart(void)
{
    return &huart2;
}
