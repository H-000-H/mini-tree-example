/**
  w25q64.c — W25Q64 SPI Flash 驱动
  架构: 通过 SPI1 总线访问, 设备树节点触发 probe
*/

#include "device.h"
#include "driver.h"
#include "hal_spi_bus.h"
#include <string.h>

/* ================================================================== */
/*  W25Q64 命令                                                        */
/* ================================================================== */
#define W25X_WREN       0x06
#define W25X_WRDI       0x04
#define W25X_RDSR1      0x05
#define W25X_RDSR2      0x35
#define W25X_READ       0x03
#define W25X_FAST_READ  0x0B
#define W25X_PP         0x02          /* Page Program (256B) */
#define W25X_SE         0x20          /* Sector Erase (4KB) */
#define W25X_CE         0xC7          /* Chip Erase */
#define W25X_RDID       0x9F          /* Read JEDEC ID */

#define W25Q64_JEDEC_ID  0xEF4017

/* ================================================================== */
/*  内部变量                                                            */
/* ================================================================== */
static hal_spi_bus_t* s_spi_bus = NULL;
static uint8_t        s_cs_pin  = 0xFF;   /* 由 DTS 指定, 默认无 */

/* ================================================================== */
/*  SPI 底层操作                                                        */
/* ================================================================== */

static void cs_low(void)
{
    if (s_cs_pin != 0xFF)
        hal_spi_assert_cs(0, s_cs_pin);
}

static void cs_high(void)
{
    if (s_cs_pin != 0xFF)
        hal_spi_deassert_cs(0, s_cs_pin);
}

static int spi_write(const uint8_t* data, size_t len)
{
    if (!s_spi_bus || !s_spi_bus->write) return -1;
    return s_spi_bus->write(s_spi_bus, data, len);
}

static int spi_read(uint8_t* data, size_t len)
{
    if (!s_spi_bus || !s_spi_bus->read) return -1;
    return s_spi_bus->read(s_spi_bus, data, len);
}

static void w25q_cmd(uint8_t cmd)
{
    cs_low();
    spi_write(&cmd, 1);
    cs_high();
}

static void w25q_read_id(uint8_t* buf)
{
    uint8_t cmd = W25X_RDID;
    cs_low();
    spi_write(&cmd, 1);
    spi_read(buf, 3);
    cs_high();
}

static uint8_t w25q_read_sr1(void)
{
    uint8_t cmd = W25X_RDSR1;
    uint8_t val = 0;
    cs_low();
    spi_write(&cmd, 1);
    spi_read(&val, 1);
    cs_high();
    return val;
}

static void w25q_wait_busy(void)
{
    uint32_t timeout = 100000;
    while ((w25q_read_sr1() & 0x01) && timeout--)
        ;
}

static void w25q_write_enable(void)
{
    w25q_cmd(W25X_WREN);
}

/* ================================================================== */
/*  公有接口 (通过 ioctl 暴露)                                          */
/* ================================================================== */

#define W25Q_CMD_READ     0x50
#define W25Q_CMD_WRITE    0x51
#define W25Q_CMD_ERASE_SEC 0x52
#define W25Q_CMD_ERASE_CHIP 0x53
#define W25Q_CMD_RDID     0x54

typedef struct {
    uint32_t addr;
    uint8_t* data;
    size_t   len;
} w25q_rw_arg_t;

static int w25q_read(uint32_t addr, uint8_t* buf, size_t len)
{
    if (!buf || len == 0) return -1;
    w25q_wait_busy();

    uint8_t cmd[4] = { W25X_READ,
                       (uint8_t)(addr >> 16),
                       (uint8_t)(addr >> 8),
                       (uint8_t)(addr) };
    cs_low();
    spi_write(cmd, 4);
    spi_read(buf, len);
    cs_high();
    return 0;
}

static int w25q_write_page(uint32_t addr, const uint8_t* data, size_t len)
{
    if (!data || len == 0 || len > 256) return -1;
    w25q_wait_busy();
    w25q_write_enable();

    uint8_t cmd[4] = { W25X_PP,
                       (uint8_t)(addr >> 16),
                       (uint8_t)(addr >> 8),
                       (uint8_t)(addr) };
    cs_low();
    spi_write(cmd, 4);
    spi_write(data, len);
    cs_high();
    w25q_wait_busy();
    return 0;
}

static int w25q_sector_erase(uint32_t addr)
{
    w25q_wait_busy();
    w25q_write_enable();

    uint8_t cmd[4] = { W25X_SE,
                       (uint8_t)(addr >> 16),
                       (uint8_t)(addr >> 8),
                       (uint8_t)(addr) };
    cs_low();
    spi_write(cmd, 4);
    cs_high();
    w25q_wait_busy();
    return 0;
}

static int w25q_chip_erase(void)
{
    w25q_wait_busy();
    w25q_write_enable();
    w25q_cmd(W25X_CE);
    w25q_wait_busy();
    return 0;
}

/* ================================================================== */
/*  VFS ioctl 入口                                                      */
/* ================================================================== */
static int w25q_ioctl(device_t* dev, int cmd, void* arg, size_t arg_len, uint32_t timeout_ms)
{
    (void)dev; (void)arg_len; (void)timeout_ms;

    switch (cmd)
    {
    case W25Q_CMD_READ: {
        w25q_rw_arg_t* a = (w25q_rw_arg_t*)arg;
        if (!a) return -1;
        return w25q_read(a->addr, a->data, a->len);
    }
    case W25Q_CMD_WRITE: {
        w25q_rw_arg_t* a = (w25q_rw_arg_t*)arg;
        if (!a) return -1;
        size_t remaining = a->len;
        size_t offset = 0;
        while (remaining > 0) {
            size_t chunk = (remaining > 256) ? 256 : remaining;
            if (w25q_write_page(a->addr + offset, a->data + offset, chunk) != 0)
                return -1;
            offset += chunk;
            remaining -= chunk;
        }
        return 0;
    }
    case W25Q_CMD_ERASE_SEC: {
        uint32_t addr = *(uint32_t*)arg;
        return w25q_sector_erase(addr);
    }
    case W25Q_CMD_ERASE_CHIP:
        return w25q_chip_erase();
    case W25Q_CMD_RDID: {
        uint8_t id[3];
        w25q_read_id(id);
        if (arg && arg_len >= 3) memcpy(arg, id, 3);
        return (int)((uint32_t)id[0] << 16 | (uint32_t)id[1] << 8 | id[2]);
    }
    default:
        return -1;
    }
}

static const file_operation_t w25q_ops = {
    .ioctl = w25q_ioctl,
};

/* ================================================================== */
/*  probe / remove                                                     */
/* ================================================================== */
static int w25q_probe(device_t* dev)
{
    /* 获取 SPI1 总线 */
    device_t* spi_dev = device_find("spi1");
    if (!spi_dev) return -1;

    s_spi_bus = device_get_spi_bus(spi_dev);
    if (!s_spi_bus) return -1;

    /* 读取 DTS CS 引脚配置 (可选) */
    int cs = 0xFF;
    device_get_prop_int(dev, "cs-pin", &cs);
    s_cs_pin = (uint8_t)cs;

    /* 验证 JEDEC ID */
    uint8_t id[3];
    w25q_read_id(id);
    uint32_t jedec = ((uint32_t)id[0] << 16) | ((uint32_t)id[1] << 8) | id[2];
    if (jedec != W25Q64_JEDEC_ID)
        return -1;

    dev->ops = &w25q_ops;
    return 0;
}

static int w25q_remove(device_t* dev)
{
    s_spi_bus = NULL;
    dev->ops = NULL;
    return 0;
}

DRIVER_REGISTER(w25q64, "winbond,w25q64", w25q_probe, w25q_remove);
