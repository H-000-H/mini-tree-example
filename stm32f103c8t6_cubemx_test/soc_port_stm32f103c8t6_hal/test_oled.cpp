/**
  test_oled.cpp — OLED 驱动集成测试
  架构: EventBus 订阅 EVENT_SYS_READY + 引脚从设备树读取
*/

#include "test_oled.hpp"
#include "event_bus.hpp"
#include "device.h"
#include "oled_drv.h"
#include "hal_gpio.h"

static hal_pin_t s_led_pin = HAL_MAKE_PIN(2, 13);

static void delay(uint32_t n)
{
    while (n--) { __asm volatile ("nop"); }
}

static void oled_test_body(const Event&, void*)
{
    oled_init();
    oled_clear();
    oled_print_8x8(0, 0, (const uint8_t*)"OLED 8x8: Hello!");
    oled_print_16x8(0, 3, (const uint8_t*)"16x8 MiniTree OK");

    for (int i = 0; i < 2; i++)
    {
        hal_gpio_set_level(s_led_pin, 0); delay(400000);
        hal_gpio_set_level(s_led_pin, 1); delay(400000);
    }
}

void run_oled_test(void)
{
    device_t* led = device_find_by_label("led0");
    if (led) { int p=2, n=13; device_get_prop_int(led,"port",&p); device_get_prop_int(led,"pin",&n); s_led_pin = HAL_MAKE_PIN(p,n); }

    EventBus::getInstance().subscribe(EVENT_SYS_READY, EVENT_SYS_READY,
        oled_test_body, nullptr);
}
