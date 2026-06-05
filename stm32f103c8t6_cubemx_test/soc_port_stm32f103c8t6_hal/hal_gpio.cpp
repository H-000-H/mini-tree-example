#include "hal_gpio.h"
#include "gpio.h"
#include "device.h"
#include "driver.h"
#include "VFS.h"
#include "stm32f1xx_hal_gpio.h"

#define GPIO_PORT_COUNT 4
#define GPIO_PORT_MAX_IDX (GPIO_PORT_COUNT - 1)

static GPIO_TypeDef* const s_port_map[GPIO_PORT_COUNT] =
{
    GPIOA, GPIOB, GPIOC, GPIOD
};

int hal_gpio_set_level(hal_pin_t pin, int level)
{
    int port = HAL_PIN_PORT(pin);
    int num  = HAL_PIN_NUM(pin);

    if (port < 0 || port > GPIO_PORT_MAX_IDX)
        return -1;
    if (num < 0 || num > 15)
        return -1;

    HAL_GPIO_WritePin(s_port_map[port], (uint16_t)(1U << num),
                      (level) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    return 0;
}

int hal_gpio_get_level(hal_pin_t pin)
{
    int port = HAL_PIN_PORT(pin);
    int num  = HAL_PIN_NUM(pin);

    if (port < 0 || port > GPIO_PORT_MAX_IDX)
        return -1;
    if (num < 0 || num > 15)
        return -1;

    return (HAL_GPIO_ReadPin(s_port_map[port], (uint16_t)(1U << num))
            == GPIO_PIN_SET) ? 1 : 0;
}

static int hal_gpio_setup(const hal_gpio_config_t* cfg)
{
    int port = HAL_PIN_PORT(cfg->pin);
    int num  = HAL_PIN_NUM(cfg->pin);

    if (port < 0 || port > GPIO_PORT_MAX_IDX)
        return -1;
    if (num < 0 || num > 15)
        return -1;

    GPIO_InitTypeDef init = {0};
    init.Pin   = (uint16_t)(1U << num);
    init.Speed = GPIO_SPEED_FREQ_HIGH;

    switch (cfg->mode)
    {
    case HAL_GPIO_MODE_INPUT:
        init.Mode = GPIO_MODE_INPUT;
        break;
    case HAL_GPIO_MODE_OUTPUT:
        init.Mode = GPIO_MODE_OUTPUT_PP;
        break;
    case HAL_GPIO_MODE_INPUT_OUTPUT:
        init.Mode = GPIO_MODE_OUTPUT_OD;
        break;
    default:
        return -1;
    }

    switch (cfg->pull)
    {
    case HAL_GPIO_PULL_UP:
        init.Pull = GPIO_PULLUP;
        break;
    case HAL_GPIO_PULL_DOWN:
        init.Pull = GPIO_PULLDOWN;
        break;
    case HAL_GPIO_PULL_NONE:
    default:
        init.Pull = GPIO_NOPULL;
        break;
    }

    /* 中断模式优先覆盖 Mode */
    switch (cfg->intr_type)
    {
    case HAL_GPIO_INTR_RISING:
        init.Mode = GPIO_MODE_IT_RISING;
        break;
    case HAL_GPIO_INTR_FALLING:
        init.Mode = GPIO_MODE_IT_FALLING;
        break;
    case HAL_GPIO_INTR_ANY_EDGE:
        init.Mode = GPIO_MODE_IT_RISING_FALLING;
        break;
    case HAL_GPIO_INTR_DISABLE:
    default:
        break;
    }

    HAL_GPIO_Init(s_port_map[port], &init);
    return 0;
}

#define MAX_ISR_ENTRIES 16

typedef struct
{
    hal_pin_t       pin;
    hal_gpio_isr_t  handler;
    void*           arg;
} gpio_isr_entry_t;

static gpio_isr_entry_t s_isr_table[MAX_ISR_ENTRIES];

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    for (int i = 0; i < MAX_ISR_ENTRIES; i++)
    {
        if (s_isr_table[i].handler &&
            HAL_PIN_NUM(s_isr_table[i].pin) == (int)GPIO_Pin)
        {
            s_isr_table[i].handler(s_isr_table[i].arg);
        }
    }
}

static int isr_install(hal_pin_t pin, hal_gpio_isr_t handler, void* arg)
{
    for (int i = 0; i < MAX_ISR_ENTRIES; i++)
    {
        if (s_isr_table[i].handler == NULL)
        {
            s_isr_table[i].pin     = pin;
            s_isr_table[i].handler = handler;
            s_isr_table[i].arg     = arg;
            return 0;
        }
    }
    return -1;
}

static int isr_remove(hal_pin_t pin)
{
    for (int i = 0; i < MAX_ISR_ENTRIES; i++)
    {
        if (s_isr_table[i].pin == pin)
        {
            s_isr_table[i].handler = NULL;
            s_isr_table[i].arg     = NULL;
            return 0;
        }
    }
    return -1;
}

static int gpio_ioctl(device_t* dev, int cmd, void* arg,
                      size_t arg_len, uint32_t timeout_ms)
{
    (void)dev; (void)arg_len; (void)timeout_ms;

    switch (cmd)
    {
    case GPIO_CMD_SET_LEVEL:
    {
        gpio_level_arg_t* level = (gpio_level_arg_t*)arg;
        return hal_gpio_set_level(level->pin, level->level);
    }
    case GPIO_CMD_GET_LEVEL:
    {
        gpio_level_arg_t* level = (gpio_level_arg_t*)arg;
        level->level = hal_gpio_get_level(level->pin);
        return 0;
    }
    case GPIO_CMD_CONFIG:
    {
        hal_gpio_config_t* cfg = (hal_gpio_config_t*)arg;
        return hal_gpio_setup(cfg);
    }
    case GPIO_CMD_TOGGLE:
    {
        hal_pin_t pin = *(hal_pin_t*)arg;
        int cur = hal_gpio_get_level(pin);
        return hal_gpio_set_level(pin, !cur);
    }
    case GPIO_CMD_INSTALL_ISR:
    {
        gpio_isr_arg_t* isr_arg = (gpio_isr_arg_t*)arg;
        return isr_install(isr_arg->pin, isr_arg->handler, isr_arg->arg);
    }
    case GPIO_CMD_REMOVE_ISR:
    {
        hal_pin_t pin = *(hal_pin_t*)arg;
        return isr_remove(pin);
    }
    default:
        return VFS_ERR_INVAL;
    }
}

static const file_operation_t stm32_gpio_fops =
{
    .ioctl = gpio_ioctl,
};

static int stm32_gpio_probe(device_t* dev)
{
    int port = -1;
    int ret = device_get_prop_int(dev, "port", &port);
    if (ret != 0 || port < 0 || port > GPIO_PORT_MAX_IDX)
        return -1;

    /* 使能 GPIO 时钟 (驱动自初始化, 不依赖 main 中 MX_GPIO_Init) */
    MX_GPIO_Init();

    device_set_subsys_priv(dev, s_port_map[port]);
    dev->ops = &stm32_gpio_fops;
    return 0;
}

static int stm32_gpio_remove(device_t* dev)
{
    GPIO_TypeDef* gpio = (GPIO_TypeDef*)device_get_subsys_priv(dev);
    if (gpio)
        HAL_GPIO_DeInit(gpio, GPIO_PIN_All);
    device_ops_unregister(dev);
    return 0;
}

DRIVER_REGISTER(stm32_gpio, "st,stm32-gpio", stm32_gpio_probe, stm32_gpio_remove);