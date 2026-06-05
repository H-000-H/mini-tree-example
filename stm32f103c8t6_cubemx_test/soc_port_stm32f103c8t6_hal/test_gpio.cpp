/**
  test_gpio.cpp — mini-tree 框架 GPIO 完整集成测试

  架构:
    - 引脚配置从设备树读取 (led0 / btn0 节点)
    - EventBus 订阅 EVENT_SYS_READY, 由 dispatch_all() 或 RTOS 触发
*/

#include "test_gpio.hpp"

/* mini-tree 系统层 */
#include "system_init.hpp"
#include "event_bus.hpp"

/* mini-tree 设备 + 驱动框架 */
#include "device.h"
#include "driver.h"
#include "hal_gpio.h"
#include "VFS.h"

/* STM32 HAL */
#include "main.h"

#include <cstdint>
#include <cstring>

/* ============================================================================
 *  引脚 — 从设备树读取
 * ============================================================================ */
static hal_pin_t s_led_pin = HAL_MAKE_PIN(2, 13); /* PC13 兜底 */
static hal_pin_t s_btn_pin = HAL_MAKE_PIN(0, 0);  /* PA0  兜底 */

/* ============================================================================
 *  轻量测试断言
 * ============================================================================ */
static int s_pass = 0;
static int s_fail = 0;

#define TEST_CHECK(cond, msg)  do {                             \
    (void)(msg);                                                \
    if (cond) { s_pass++; } else {                              \
        s_fail++;                                               \
        for (int _i = 0; _i < 3; _i++) {                        \
            hal_gpio_set_level(s_led_pin, 0);                   \
            delay_busy(100000);                                 \
            hal_gpio_set_level(s_led_pin, 1);                   \
            delay_busy(100000);                                 \
        }                                                       \
    }                                                           \
} while(0)

static void delay_busy(uint32_t n)
{
    while (n--) { __NOP(); }
}

/* ============================================================================
 *  测试 1: GPIO 快速路径 API
 * ============================================================================ */
static void test_fast_path(void)
{
    /* 输出高低 */
    hal_gpio_set_level(s_led_pin, 1);  delay_busy(300000);
    hal_gpio_set_level(s_led_pin, 0);  delay_busy(300000);

    int val = hal_gpio_get_level(s_led_pin);
    TEST_CHECK(val == 0, "get_level after set LOW");

    hal_gpio_set_level(s_led_pin, 1);
    val = hal_gpio_get_level(s_led_pin);
    TEST_CHECK(val == 1, "get_level after set HIGH");

    /* 跨端口输出 */
    hal_gpio_set_level(HAL_MAKE_PIN(0, 1), 1);   /* PA1 */
    hal_gpio_set_level(HAL_MAKE_PIN(1, 5), 0);   /* PB5 */

    /* 边界检测 */
    TEST_CHECK(hal_gpio_set_level(HAL_MAKE_PIN(9, 0), 0) == -1, "invalid port -> -1");
    TEST_CHECK(hal_gpio_set_level(HAL_MAKE_PIN(0, 99), 0) == -1, "invalid pin -> -1");
    TEST_CHECK(hal_gpio_get_level(HAL_MAKE_PIN(9, 0)) == -1, "get invalid -> -1");
}

/* ============================================================================
 *  测试 2: GPIO 设备框架
 * ============================================================================ */
static void test_device_framework(void)
{
    static const char* const names[] = { "gpio0", "gpio1", "gpio2" };
    static const uint32_t    bases[] = { 0x40010800, 0x40010C00, 0x40011000 };

    for (int port = 0; port < 3; port++)
    {
        device_t* dev = device_find(names[port]);
        TEST_CHECK(dev != nullptr, names[port]);
        if (!dev) continue;

        /* ioctl: SET_LEVEL */
        gpio_level_arg_t arg = { HAL_MAKE_PIN(port, 13), 1 };
        int ret = dev->ops->ioctl(dev, GPIO_CMD_SET_LEVEL, &arg, sizeof(arg), 100);
        TEST_CHECK(ret == 0, "SET_LEVEL");

        /* ioctl: GET_LEVEL */
        arg.level = 0;
        ret = dev->ops->ioctl(dev, GPIO_CMD_GET_LEVEL, &arg, sizeof(arg), 100);
        TEST_CHECK(ret == 0, "GET_LEVEL");

        /* ioctl: TOGGLE */
        hal_pin_t pin = HAL_MAKE_PIN(port, 13);
        ret = dev->ops->ioctl(dev, GPIO_CMD_TOGGLE, &pin, sizeof(pin), 100);
        TEST_CHECK(ret == 0, "TOGGLE");

        /* 验证设备树 reg 地址 */
        const device_reg_t* reg = nullptr;
        if (device_get_reg(dev, 0, &reg) == 0 && reg && reg->addr)
        {
            TEST_CHECK(reg->addr[0] == bases[port], "reg match");
        }
    }

    /* 语义查找 */
    TEST_CHECK(device_find_by_compatible("st,stm32-gpio") != nullptr, "by_compatible");
    TEST_CHECK(device_find_by_path("/soc/gpio0@40010800") != nullptr, "by_path");
}

/* ============================================================================
 *  测试 3: GPIO 中断
 * ============================================================================ */
static volatile uint32_t s_isr_fired = 0;

static void test_isr_handler(void* arg)
{
    s_isr_fired++;
    (void)arg;
}

static void test_interrupt(void)
{
    device_t* dev = device_find("gpio0");
    if (!dev) return;

    hal_gpio_config_t cfg = {
        s_btn_pin,
        HAL_GPIO_MODE_INPUT,
        HAL_GPIO_PULL_UP,
        HAL_GPIO_INTR_FALLING,
    };
    dev->ops->ioctl(dev, GPIO_CMD_CONFIG, &cfg, sizeof(cfg), 100);

    gpio_isr_arg_t isr_arg = { s_btn_pin, test_isr_handler, nullptr };
    int ret = dev->ops->ioctl(dev, GPIO_CMD_INSTALL_ISR, &isr_arg, sizeof(isr_arg), 100);
    TEST_CHECK(ret == 0, "install ISR");
}

/* ============================================================================
 *  测试主体 — 由 EventBus 触发
 * ============================================================================ */
static void gpio_test_body(const Event&, void*)
{
    test_fast_path();
    test_device_framework();
    test_interrupt();

    /* 结果指示: 全 PASS → LED 快闪 3 下 */
    if (s_fail == 0)
    {
        for (int i = 0; i < 3; i++)
        {
            hal_gpio_set_level(s_led_pin, 0);
            delay_busy(50000);
            hal_gpio_set_level(s_led_pin, 1);
            delay_busy(50000);
        }
    }
}

/* ============================================================================
 *  公有入口 — 由 main.c 调用
 *  功能: 读设备树 → 注册 EventBus 订阅
 * ============================================================================ */
void run_gpio_test(void)
{
    /* 从设备树读取 LED 引脚 */
    device_t* led_node = device_find_by_label("led0");
    if (led_node)
    {
        int port = 2, pin = 13;
        device_get_prop_int(led_node, "port", &port);
        device_get_prop_int(led_node, "pin", &pin);
        s_led_pin = HAL_MAKE_PIN(port, pin);
    }

    /* 从设备树读取按键引脚 */
    device_t* btn_node = device_find_by_label("btn0");
    if (btn_node)
    {
        int port = 0, pin = 0;
        device_get_prop_int(btn_node, "port", &port);
        device_get_prop_int(btn_node, "pin", &pin);
        s_btn_pin = HAL_MAKE_PIN(port, pin);
    }

    /* 注册 EventBus 订阅, 由 EVENT_SYS_READY 触发 */
    EventBus::getInstance().subscribe(EVENT_SYS_READY, EVENT_SYS_READY,
        gpio_test_body, nullptr);
}
