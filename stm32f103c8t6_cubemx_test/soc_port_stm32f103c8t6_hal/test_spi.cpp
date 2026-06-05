/**
  test_spi.cpp — SPI 驱动集成测试
  架构: EventBus 订阅 EVENT_SYS_READY + 引脚从设备树读取
*/

#include "test_spi.hpp"
#include "event_bus.hpp"
#include "device.h"
#include "driver.h"
#include "hal_spi_bus.h"
#include "hal_gpio.h"
#include <cstdint>

/* LED 引脚 — 从设备树 led0 节点读取 */
static hal_pin_t s_led_pin = HAL_MAKE_PIN(2, 13);  /* PC13 兜底 */

static void delay(uint32_t n)
{
    while (n--) { __asm volatile ("nop"); }
}

static void spi_test_body(const Event&, void*)
{
    int pass = 0;
    device_t* dev = nullptr;
    hal_spi_bus_t* bus = nullptr;
    uint8_t tx = 0xA5;

    dev = device_find_by_label("spi1");
    if (!dev) goto fail;

    bus = device_get_spi_bus(dev);
    if (!bus) goto fail;

    if (bus->write(bus, &tx, 1) == 1) pass = 1;

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

void run_spi_test(void)
{
    device_t* led = device_find_by_label("led0");
    if (led) { int p=2, n=13; device_get_prop_int(led,"port",&p); device_get_prop_int(led,"pin",&n); s_led_pin = HAL_MAKE_PIN(p,n); }

    EventBus::getInstance().subscribe(EVENT_SYS_READY, EVENT_SYS_READY,
        spi_test_body, nullptr);
}
