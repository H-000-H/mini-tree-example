/**
  test_adc.cpp — ADC 驱动集成测试
  架构: EventBus 订阅 EVENT_SYS_READY + 引脚从设备树读取
*/

#include "test_adc.hpp"
#include "event_bus.hpp"
#include "device.h"
#include "driver.h"
#include "hal_adc.h"
#include "hal_gpio.h"
#include <cstdint>

static hal_pin_t s_led_pin = HAL_MAKE_PIN(2, 13);

static void delay(uint32_t n)
{
    while (n--) { __asm volatile ("nop"); }
}

static void adc_test_body(const Event&, void*)
{
    int pass = 0;
    device_t* dev = nullptr;
    hal_adc_t* adc = nullptr;
    hal_adc_config_t cfg;
    uint32_t raw = 0;

    dev = device_find_by_label("adc1");
    if (!dev) goto fail;

    adc = (hal_adc_t*)device_get_priv(dev);
    if (!adc) goto fail;

    cfg.channel = 0;
    cfg.width   = HAL_ADC_WIDTH_12BIT;
    cfg.vref_mv = 3300;

    if (adc->init(adc, &cfg) != 0) goto fail;

    if (adc->read_raw(adc, &raw) != 0) goto fail;
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

void run_adc_test(void)
{
    device_t* led = device_find_by_label("led0");
    if (led) { int p=2, n=13; device_get_prop_int(led,"port",&p); device_get_prop_int(led,"pin",&n); s_led_pin = HAL_MAKE_PIN(p,n); }

    EventBus::getInstance().subscribe(EVENT_SYS_READY, EVENT_SYS_READY,
        adc_test_body, nullptr);
}
