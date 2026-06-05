#include "wifi_manager.h"
#include "wifi_esp32.h"
#include <string.h>
#include <stdio.h>
#include "main.h"

#define UART ((UART_HandleTypeDef*)wifi_esp32_get_uart())

/* 发送 AT 指令并读响应, 返回是否含 OK */
static int at_cmd(const char *cmd, uint8_t *resp, uint16_t *rlen, uint16_t rmax)
{
    HAL_UART_Transmit(UART, (uint8_t*)cmd, strlen(cmd), 1000);
    memset(resp, 0, rmax);
    *rlen = 0;
    HAL_UARTEx_ReceiveToIdle(UART, resp, rmax - 1, rlen, 3000);
    if (*rlen < rmax) resp[*rlen] = '\0';
    else resp[rmax - 1] = '\0';
    return strstr((const char*)resp, "OK") ? 0 : -1;
}

int wifi_init_sta(void)
{
    wifi_esp32_init();
    uint8_t resp[128];
    uint16_t rlen = 0;
    return at_cmd("AT+CWMODE=1\r\n", resp, &rlen, sizeof(resp));
}

int wifi_scan(wifi_ap_record_t *aps, int *count)
{
    if (!aps || !count || *count <= 0) return -1;
    int max = (*count < WIFI_SCAN_MAX) ? *count : WIFI_SCAN_MAX;
    *count = 0;

    HAL_UART_Transmit(UART, (uint8_t*)"AT+CWLAP\r\n", 10, 1000);
    uint8_t buf[2048];
    uint16_t len = 0;
    memset(buf, 0, sizeof(buf));
    HAL_UARTEx_ReceiveToIdle(UART, buf, sizeof(buf) - 1, &len, 10000);
    if (len > 0) buf[len] = '\0';

    /* 逐行解析 +CWLAP:(ecn,rssi,"ssid",ch) */
    char *line = (char*)buf;
    int n = 0;
    while ((line = strstr(line, "+CWLAP:")) && n < max)
    {
        int ecn, rssi, ch;
        char ssid[64];
        if (sscanf(line, "+CWLAP:(%d,%d,\"%[^\"]\",%d)", &ecn, &rssi, ssid, &ch) >= 3)
        {
            aps[n].ecn     = (uint8_t)ecn;
            aps[n].rssi    = (int8_t)rssi;
            aps[n].channel = ch;
            strncpy(aps[n].ssid, ssid, WIFI_SSID_MAX_LEN - 1);
            aps[n].ssid[WIFI_SSID_MAX_LEN - 1] = '\0';
            n++;
        }
        line++;
    }

    *count = n;
    return (n > 0) ? 0 : -1;
}

int wifi_connect(const char *ssid, const char *password)
{
    char cmd[196];
    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);
    HAL_UART_Transmit(UART, (uint8_t*)cmd, strlen(cmd), 1000);

    uint32_t start = HAL_GetTick();
    while (HAL_GetTick() - start < 20000)
    {
        uint8_t buf[128];
        uint16_t len = 0;
        memset(buf, 0, sizeof(buf));
        HAL_UARTEx_ReceiveToIdle(UART, buf, sizeof(buf) - 1, &len, 2000);
        if (len == 0) continue;
        buf[len] = '\0';
        if (strstr((const char*)buf, "WIFI GOT IP") ||
            strstr((const char*)buf, "WIFI CONNECTED"))
            return 0;
        if (strstr((const char*)buf, "FAIL") || strstr((const char*)buf, "ERROR"))
            return -1;
    }
    return -1;
}

int wifi_disconnect(void)
{
    uint8_t resp[64];
    uint16_t rlen = 0;
    return at_cmd("AT+CWQAP\r\n", resp, &rlen, sizeof(resp));
}

int wifi_is_connected(void)
{
    uint8_t resp[128];
    uint16_t rlen = 0;
    at_cmd("AT+CWJAP?\r\n", resp, &rlen, sizeof(resp));
    /* CWJAP 返回 "+CWJAP:" 表示已连接 */
    return strstr((const char*)resp, "+CWJAP:") ? 1 : 0;
}
