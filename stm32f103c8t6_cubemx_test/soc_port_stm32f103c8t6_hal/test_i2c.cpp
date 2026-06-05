/**
  test_i2c.cpp — I2C 驱动集成测试
  架构: EventBus 订阅 EVENT_SYS_READY + 引脚从设备树读取
*/

#include "test_i2c.hpp"
#include "event_bus.hpp"
#include "device.h"
#include "driver.h"
#include "hal_i2c.h"
#include "hal_gpio.h"
#include <cstdint>

static hal_pin_t s_led_pin = HAL_MAKE_PIN(2, 13);

static void delay(uint32_t n)
{
    while (n--) { __asm volatile ("nop"); }
}

static void i2c_test_body(const Event&, void*)
{
    int pass = 0;
    device_t* dev = nullptr;
    hal_i2c_bus_t* bus = nullptr;
    uint8_t dummy = 0;
    uint8_t reg = 0x00;

    dev = device_find_by_label("i2c1");
    if (!dev) goto fail;

    bus = device_get_i2c_bus(dev);
    if (!bus) goto fail;

    bus->read(bus, 0x50, &dummy, 1, 10);
    bus->write_read(bus, 0x50, &reg, 1, &dummy, 1, 10);
    pass = 1;

fail:
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

void run_i2c_test(void)
{
    device_t* led = device_find_by_label("led0");
    if (led) { int p=2, n=13; device_get_prop_int(led,"port",&p); device_get_prop_int(led,"pin",&n); s_led_pin = HAL_MAKE_PIN(p,n); }

    EventBus::getInstance().subscribe(EVENT_SYS_READY, EVENT_SYS_READY,
        i2c_test_body, nullptr);
}
