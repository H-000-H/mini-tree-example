/**
  test_wifi.cpp — ESP32 AT WiFi 测试
  架构: EventBus 订阅 EVENT_SYS_READY + 引脚从设备树读取
*/

#include "test_wifi.hpp"
#include "event_bus.hpp"
#include "device.h"
#include "wifi_esp32.h"
#include "hal_gpio.h"
#include <string.h>
#include <stdio.h>

static hal_pin_t s_led_pin = HAL_MAKE_PIN(2, 13);

static void delay(uint32_t n)
{
    while (n--) { __asm volatile ("nop"); }
}

static void wifi_test_body(const Event&, void*)
{
    int pass = 0;
    uint8_t resp[128];

    wifi_esp32_init();
    wifi_esp32_send_cmd((const uint8_t*)"AT\r\n", 4);
    uint16_t rlen = sizeof(resp) - 1;
    if (wifi_esp32_get_response(resp, &rlen) == 0)
    {
        resp[rlen] = '\0';
        if (strstr((const char*)resp, "OK")) pass = 1;
    }

    if (pass)
        for (int i = 0; i < 2; i++)
        {
            hal_gpio_set_level(s_led_pin, 0); delay(400000);
            hal_gpio_set_level(s_led_pin, 1); delay(400000);
        }
    else
        for (int i = 0; i < 5; i++)
        {
            hal_gpio_set_level(s_led_pin, 0); delay(200000);
            hal_gpio_set_level(s_led_pin, 1); delay(200000);
        }
}

void run_wifi_test(void)
{
    device_t* led = device_find_by_label("led0");
    if (led) { int p=2, n=13; device_get_prop_int(led,"port",&p); device_get_prop_int(led,"pin",&n); s_led_pin = HAL_MAKE_PIN(p,n); }

    EventBus::getInstance().subscribe(EVENT_SYS_READY, EVENT_SYS_READY,
        wifi_test_body, nullptr);
}
