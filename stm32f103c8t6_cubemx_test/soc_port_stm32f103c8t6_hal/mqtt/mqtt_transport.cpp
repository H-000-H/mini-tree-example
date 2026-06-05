#include "mqtt_transport.h"
#include "wifi_esp32.h"
#include <string.h>
#include <stdio.h>

/* usart.h 由 CubeMX 生成, 定义 UART_HandleTypeDef */
#include "usart.h"

/* ------------------------------------------------------------------ */
/* 内部读缓冲区 — 缓存 +IPD 解析后的净荷                                */
/* ------------------------------------------------------------------ */
#define MQTT_RX_BUF  512
static uint8_t  s_pool[MQTT_RX_BUF];
static uint16_t s_pool_len;

static UART_HandleTypeDef* uart(void)
{
    return (UART_HandleTypeDef*)wifi_esp32_get_uart();
}

/* 从 UART 读取原始数据, 解析 +IPD 并提取 payload 存入 s_pool */
static void poll_data(int timeout_ms)
{
    uint8_t raw[MQTT_RX_BUF];
    uint16_t raw_len = 0;

    memset(raw, 0, sizeof(raw));
    HAL_UARTEx_ReceiveToIdle(uart(), raw, sizeof(raw) - 1, &raw_len, timeout_ms);
    if (raw_len == 0)
        return;

    raw[raw_len] = '\0';

    /* ── 尝试解析 +IPD,<len>:<data> 或 +IPD,<id>,<len>:<data> ── */
    const char *ipd = strstr((const char*)raw, "+IPD,");
    if (ipd)
    {
        const char *colon = strchr(ipd, ':');
        if (!colon) return;

        uint16_t header = (uint16_t)(colon + 1 - (char*)raw);
        uint16_t data   = raw_len - header;

        if (data > MQTT_RX_BUF)
            data = MQTT_RX_BUF;
        memcpy(s_pool, raw + header, data);
        s_pool_len = data;
        return;
    }

    /* ── 裸 MQTT 数据 (透传 / 读剩余部分) ── */
    {
        uint16_t copy = raw_len;
        if (copy > MQTT_RX_BUF)
            copy = MQTT_RX_BUF;
        memcpy(s_pool, raw, copy);
        s_pool_len = copy;
    }
}

/* ------------------------------------------------------------------ */
/* Network 回调                                                        */
/* ------------------------------------------------------------------ */
int mqtt_read_callback(Network* n, unsigned char* buf, int len, int timeout_ms)
{
    (void)n;

    /* 缓冲区为空时先填充 */
    if (s_pool_len == 0)
        poll_data(timeout_ms);
    if (s_pool_len == 0)
        return -1;

    int copy = len;
    if (copy > (int)s_pool_len)
        copy = (int)s_pool_len;

    memcpy(buf, s_pool, copy);
    /* 将剩余数据前移 */
    if ((int)s_pool_len > copy)
        memmove(s_pool, s_pool + copy, s_pool_len - copy);
    s_pool_len -= (uint16_t)copy;

    return copy;
}

int mqtt_write_callback(Network* n, unsigned char* buf, int len, int timeout_ms)
{
    (void)n;

    /* AT+CIPSEND=0,<len> */
    char cmd[64];
    int slen = snprintf(cmd, sizeof(cmd), "AT+CIPSEND=0,%d\r\n", len);
    HAL_UART_Transmit(uart(), (uint8_t*)cmd, slen, 1000);

    /* 等待 ">" 提示符 */
    uint32_t start = HAL_GetTick();
    while (HAL_GetTick() - start < 5000)
    {
        uint8_t c;
        uint16_t got = 0;
        if (HAL_UART_Receive(uart(), &c, 1, 100) == HAL_OK)
        {
            if (c == '>')
                goto send_raw;
        }
    }
    return -1;  /* 超时未收到 ">" */

send_raw:
    HAL_UART_Transmit(uart(), buf, len, timeout_ms);

    /* 等待 SEND OK */
    {
        uint8_t ack[64];
        uint16_t ack_len = 0;
        memset(ack, 0, sizeof(ack));
        HAL_UARTEx_ReceiveToIdle(uart(), ack, sizeof(ack) - 1, &ack_len, 3000);
        if (ack_len > 0)
            ack[ack_len] = '\0';

        if (strstr((const char*)ack, "SEND OK"))
            return len;
        if (strstr((const char*)ack, "ERROR") || strstr((const char*)ack, "FAIL"))
            return -1;
    }

    return len;  /* 超时但乐观返回已发送 */
}

/* ------------------------------------------------------------------ */
/* 公开接口                                                            */
/* ------------------------------------------------------------------ */
int mqtt_transport_connect(Network* n, const char* host, int port)
{
    char cmd[128];
#ifdef MQTT_USE_TLS
    snprintf(cmd, sizeof(cmd), "AT+CIPSTART=0,\"SSL\",\"%s\",%d\r\n", host, port);
#else
    snprintf(cmd, sizeof(cmd), "AT+CIPSTART=0,\"TCP\",\"%s\",%d\r\n", host, port);
#endif
    HAL_UART_Transmit(uart(), (uint8_t*)cmd, strlen(cmd), 1000);

    /* 等待 CONNECT / ALREADY CONNECTED */
    uint32_t start = HAL_GetTick();
    while (HAL_GetTick() - start < 15000)
    {
        uint8_t buf[128];
        uint16_t len = 0;
        memset(buf, 0, sizeof(buf));
        HAL_UARTEx_ReceiveToIdle(uart(), buf, sizeof(buf) - 1, &len, 2000);
        if (len > 0)
        {
            buf[len] = '\0';
            if (strstr((const char*)buf, "CONNECT"))
            {
                n->mqttread  = mqtt_read_callback;
                n->mqttwrite = mqtt_write_callback;
                return 0;
            }
            if (strstr((const char*)buf, "ERROR") || strstr((const char*)buf, "FAIL"))
                return -1;
        }
    }
    return -1;
}

void mqtt_transport_disconnect(void)
{
    static const char cmd[] = "AT+CIPCLOSE=0\r\n";
    HAL_UART_Transmit(uart(), (const uint8_t*)cmd, strlen(cmd), 1000);
    s_pool_len = 0;
}
