#include "hal_i2c.h"
#include "device.h"
#include "driver.h"

#include "i2c.h"
#include "stm32f1xx_hal_i2c.h"

#define I2C_MAX_COUNT  1

static I2C_HandleTypeDef* const s_i2c_handles[I2C_MAX_COUNT] = {
    &hi2c1,
};

static hal_i2c_bus_t s_i2c_buses[I2C_MAX_COUNT];

static int i2c_init(hal_i2c_bus_t* bus, const hal_i2c_config_t* cfg)
{
    (void)bus; (void)cfg;
    return 0;
}

static int i2c_write(hal_i2c_bus_t* bus, uint8_t addr,
                     const uint8_t* data, size_t len, uint32_t time_out)
{
    I2C_HandleTypeDef* hi2c = (I2C_HandleTypeDef*)bus->_impl;
    if (!hi2c) return -1;
    return (HAL_I2C_Master_Transmit(hi2c, (uint16_t)(addr << 1),
                                    (uint8_t*)data, (uint16_t)len, time_out)
            == HAL_OK) ? (int)len : -1;
}

static int i2c_read(hal_i2c_bus_t* bus, uint8_t addr,
                    uint8_t* data, size_t len, uint32_t time_out)
{
    I2C_HandleTypeDef* hi2c = (I2C_HandleTypeDef*)bus->_impl;
    if (!hi2c) return -1;
    return (HAL_I2C_Master_Receive(hi2c, (uint16_t)(addr << 1),
                                   data, (uint16_t)len, time_out)
            == HAL_OK) ? (int)len : -1;
}

static int i2c_write_read(hal_i2c_bus_t* bus, uint8_t addr,
                          const uint8_t* wdata, size_t wlen,
                          uint8_t* rdata, size_t rlen, uint32_t time_out)
{
    I2C_HandleTypeDef* hi2c = (I2C_HandleTypeDef*)bus->_impl;
    if (!hi2c) return -1;

    if (wlen > 0)
    {
        if (HAL_I2C_Master_Transmit(hi2c, (uint16_t)(addr << 1),
                                    (uint8_t*)wdata, (uint16_t)wlen,
                                    time_out) != HAL_OK)
            return -1;
    }

    if (rlen > 0)
    {
        if (HAL_I2C_Master_Receive(hi2c, (uint16_t)(addr << 1),
                                   rdata, (uint16_t)rlen,
                                   time_out) != HAL_OK)
            return -1;
    }

    return (int)(wlen + rlen);
}

static int i2c_bus_recover(hal_i2c_bus_t* bus)
{
    (void)bus;
    return 0;
}

static int i2c_deinit(hal_i2c_bus_t* bus)
{
    I2C_HandleTypeDef* hi2c = (I2C_HandleTypeDef*)bus->_impl;
    if (!hi2c) return -1;
    HAL_I2C_DeInit(hi2c);
    bus->_impl = NULL;
    return 0;
}

void hal_i2c_init_struct(hal_i2c_bus_t* bus)
{
    bus->init        = i2c_init;
    bus->write       = i2c_write;
    bus->read        = i2c_read;
    bus->write_read  = i2c_write_read;
    bus->bus_recover = i2c_bus_recover;
    bus->deinit      = i2c_deinit;
    bus->_impl       = NULL;
}

int hal_i2c_lock_bus(int port, uint32_t timeout_ms)
{
    (void)port; (void)timeout_ms;
    return 0;
}

int hal_i2c_unlock_bus(int port)
{
    (void)port;
    return 0;
}

void hal_i2c_force_stop(void)
{
    for (int i = 0; i < I2C_MAX_COUNT; i++)
    {
        if (s_i2c_handles[i])
        {
            HAL_I2C_DeInit(s_i2c_handles[i]);
        }
    }
}

extern "C" hal_i2c_bus_t* device_get_i2c_bus(device_t* dev)
{
    return (hal_i2c_bus_t*)device_get_priv(dev);
}

/* ── probe / remove ── */
static int stm32_i2c_probe(device_t* dev)
{
    int index = 0;
    if (device_get_prop_int(dev, "i2c-index", &index) != 0)
        return -1;
    if (index < 0 || index >= I2C_MAX_COUNT || !s_i2c_handles[index])
        return -1;

    MX_I2C1_Init();

    hal_i2c_bus_t* bus = &s_i2c_buses[index];
    hal_i2c_init_struct(bus);
    bus->_impl = s_i2c_handles[index];
    device_set_priv(dev, bus);
    return 0;
}

static int stm32_i2c_remove(device_t* dev)
{
    hal_i2c_bus_t* bus = (hal_i2c_bus_t*)device_get_priv(dev);
    if (bus && bus->_impl)
    {
        HAL_I2C_DeInit((I2C_HandleTypeDef*)bus->_impl);
        bus->_impl = NULL;
    }
    device_set_priv(dev, NULL);
    return 0;
}

DRIVER_REGISTER(stm32_i2c, "st,stm32-i2c", stm32_i2c_probe, stm32_i2c_remove);
