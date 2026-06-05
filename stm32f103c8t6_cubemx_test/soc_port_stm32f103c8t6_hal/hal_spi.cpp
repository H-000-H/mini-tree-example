#include "hal_spi_bus.h"
#include "device.h"
#include "driver.h"

#include "spi.h"
#include "stm32f1xx_hal_spi.h"

#define SPI_MAX_COUNT  1

static SPI_HandleTypeDef* const s_spi_handles[SPI_MAX_COUNT] = {
    &hspi1,
};

static hal_spi_bus_t s_spi_buses[SPI_MAX_COUNT];

/* ── wrapper: 已由 CubeMX MX_SPI1_Init 初始化 ── */
static int spi_init(hal_spi_bus_t* bus, const hal_spi_bus_config_t* bus_cfg,
                    const hal_spi_device_config_t* dev_cfg)
{
    (void)bus; (void)bus_cfg; (void)dev_cfg;
    return 0;
}

static int spi_write(hal_spi_bus_t* bus, const uint8_t* data, size_t len)
{
    SPI_HandleTypeDef* hspi = (SPI_HandleTypeDef*)bus->_impl;
    if (!hspi) return -1;
    return (HAL_SPI_Transmit(hspi, (uint8_t*)data, (uint16_t)len, 1000)
            == HAL_OK) ? (int)len : -1;
}

static int spi_write_top_half(hal_spi_bus_t* bus, const uint8_t* data, size_t len)
{
    SPI_HandleTypeDef* hspi = (SPI_HandleTypeDef*)bus->_impl;
    if (!hspi) return -1;
    return (HAL_SPI_Transmit_IT(hspi, (uint8_t*)data, (uint16_t)len)
            == HAL_OK) ? (int)len : -1;
}

static int spi_read(hal_spi_bus_t* bus, uint8_t* data, size_t len)
{
    SPI_HandleTypeDef* hspi = (SPI_HandleTypeDef*)bus->_impl;
    if (!hspi) return -1;
    return (HAL_SPI_Receive(hspi, data, (uint16_t)len, 1000)
            == HAL_OK) ? (int)len : -1;
}

static int spi_deinit(hal_spi_bus_t* bus)
{
    SPI_HandleTypeDef* hspi = (SPI_HandleTypeDef*)bus->_impl;
    if (!hspi) return -1;
    HAL_SPI_DeInit(hspi);
    bus->_impl = NULL;
    return 0;
}

void hal_spi_bus_init_struct(hal_spi_bus_t* bus)
{
    bus->init           = spi_init;
    bus->write          = spi_write;
    bus->write_top_half = spi_write_top_half;
    bus->read           = spi_read;
    bus->deinit         = spi_deinit;
    bus->_impl          = NULL;
}

int hal_spi_lock_bus(int bus_id, uint32_t timeout_ms)
{
    (void)bus_id; (void)timeout_ms;
    return 0;
}

int hal_spi_unlock_bus(int bus_id)
{
    (void)bus_id;
    return 0;
}

int hal_spi_assert_cs(int bus_id, int cs_line)
{
    (void)bus_id; (void)cs_line;
    if (bus_id == 0 && cs_line >= 0)
        HAL_GPIO_WritePin(GPIOA, (uint16_t)(1U << cs_line), GPIO_PIN_RESET);
    return 0;
}

int hal_spi_deassert_cs(int bus_id, int cs_line)
{
    (void)bus_id; (void)cs_line;
    if (bus_id == 0 && cs_line >= 0)
        HAL_GPIO_WritePin(GPIOA, (uint16_t)(1U << cs_line), GPIO_PIN_SET);
    return 0;
}

void hal_spi_force_stop(void)
{
    for (int i = 0; i < SPI_MAX_COUNT; i++)
    {
        if (s_spi_handles[i])
        {
            HAL_SPI_DeInit(s_spi_handles[i]);
            __HAL_SPI_DISABLE(s_spi_handles[i]);
        }
    }
}

extern "C" hal_spi_bus_t* device_get_spi_bus(device_t* dev)
{
    return (hal_spi_bus_t*)device_get_priv(dev);
}

/* ── probe / remove ── */
static int stm32_spi_probe(device_t* dev)
{
    int index = 0;
    if (device_get_prop_int(dev, "spi-index", &index) != 0)
        return -1;
    if (index < 0 || index >= SPI_MAX_COUNT || !s_spi_handles[index])
        return -1;

    MX_SPI1_Init();

    hal_spi_bus_t* bus = &s_spi_buses[index];
    hal_spi_bus_init_struct(bus);
    bus->_impl = s_spi_handles[index];
    device_set_priv(dev, bus);
    return 0;
}

static int stm32_spi_remove(device_t* dev)
{
    hal_spi_bus_t* bus = (hal_spi_bus_t*)device_get_priv(dev);
    if (bus && bus->_impl)
    {
        HAL_SPI_DeInit((SPI_HandleTypeDef*)bus->_impl);
        bus->_impl = NULL;
    }
    device_set_priv(dev, NULL);
    return 0;
}

DRIVER_REGISTER(stm32_spi, "st,stm32-spi", stm32_spi_probe, stm32_spi_remove);
