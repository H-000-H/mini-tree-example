#ifndef WIFI_ESP32_H
#define WIFI_ESP32_H

#include <stdint.h>

#define ESP32_RX_BUF_SIZE  512

void wifi_esp32_init(void);
void wifi_esp32_send_cmd(const uint8_t* cmd, uint16_t len);
int  wifi_esp32_get_response(uint8_t* buf, uint16_t* len);

void* wifi_esp32_get_uart(void);

#endif
