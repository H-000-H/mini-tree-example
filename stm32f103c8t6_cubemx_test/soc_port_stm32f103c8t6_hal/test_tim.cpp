/**
  test_tim.cpp — TIM 驱动集成测试
  架构: EventBus 订阅 EVENT_SYS_READY + 引脚从设备树读取
*/

#include "test_tim.hpp"
#include "event_bus.hpp"
#include "device.h"
#include "driver.h"
#include "hal_timer.h"
#include "hal_gpio.h"
#include <cstdint>

static hal_pin_t s_led_pin = HAL_MAKE_PIN(2, 13);

static void delay(uint32_t n)
{
    while (n--) { __asm volatile ("nop"); }
}

static void tim_test_body(const Event&, void*)
{
    int pass = 0;
    device_t* dev = nullptr;
    hal_timer_t* timer = nullptr;
    hal_timer_config_t cfg;
    uint32_t cnt = 0;

    dev = device_find_by_label("tim1");
    if (!dev) goto fail;

    timer = (hal_timer_t*)device_get_priv(dev);
    if (!timer) goto fail;

    cfg.timer_id  = 0;
    cfg.freq_hz   = 72000000;
    cfg.mode      = HAL_TIMER_MODE_PERIODIC;
    cfg.period_us = 100000;
    cfg.dead_time_ns = 0;

    if (timer->init(timer, &cfg) != 0) goto fail;
    if (timer->start(timer) != 0) goto fail;

    delay(50000);
    timer->get_counter(timer, &cnt);
    pass = (cnt > 0) ? 1 : 0;
    timer->stop(timer);

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

void run_tim_test(void)
{
    device_t* led = device_find_by_label("led0");
    if (led) { int p=2, n=13; device_get_prop_int(led,"port",&p); device_get_prop_int(led,"pin",&n); s_led_pin = HAL_MAKE_PIN(p,n); }

    EventBus::getInstance().subscribe(EVENT_SYS_READY, EVENT_SYS_READY,
        tim_test_body, nullptr);
}
