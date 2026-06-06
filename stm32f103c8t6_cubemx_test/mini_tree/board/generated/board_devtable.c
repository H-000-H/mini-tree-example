#include "board_nodes.h"
#include "board_devtable.h"
#include "device.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

/* ===== 属性表 (静态 .rodata) ===== */

/* /soc/gpio@40010800 */
static const device_prop_t DEV_gpio_40010800_props[] = {
    {"reg", "0x40010800 0x400"},
    {"port", "0x0"},
};

/* /soc/gpio@1 */
static const device_prop_t DEV_gpio_1_props[] = {
    {"reg", "0x40010c00 0x400"},
    {"port", "0x1"},
};

/* /soc/gpio@40011000 */
static const device_prop_t DEV_gpio_40011000_props[] = {
    {"reg", "0x40011000 0x400"},
    {"port", "0x2"},
};

/* ===== 依赖表 ===== */

/* ===== reg 分组表 (预分组, 按 #address-cells / #size-cells) ===== */

static const uint32_t DEV_gpio_40010800_REG_DATA[] = {
    0x40010800, 0x400,
};
static const uint32_t DEV_gpio_1_REG_DATA[] = {
    0x40010c00, 0x400,
};
static const uint32_t DEV_gpio_40011000_REG_DATA[] = {
    0x40011000, 0x400,
};
static const device_reg_t DEV_gpio_40010800_REGS[] = {
    { .addr = &DEV_gpio_40010800_REG_DATA[0], .addr_cells = 1, .size = &DEV_gpio_40010800_REG_DATA[1], .size_cells = 1 },
};

static const device_reg_t DEV_gpio_1_REGS[] = {
    { .addr = &DEV_gpio_1_REG_DATA[0], .addr_cells = 1, .size = &DEV_gpio_1_REG_DATA[1], .size_cells = 1 },
};

static const device_reg_t DEV_gpio_40011000_REGS[] = {
    { .addr = &DEV_gpio_40011000_REG_DATA[0], .addr_cells = 1, .size = &DEV_gpio_40011000_REG_DATA[1], .size_cells = 1 },
};

/* ===== irq 表 (预分组, 按 #interrupt-cells) ===== */

/* ===== 主节点表 (只读 .rodata) ===== */
static const device_node_t s_nodes[DEV_ID_COUNT] = {
    [DEV_ID_GPIO_40010800] = {
        .name       = "gpio@40010800",
        .label      = "gpio0",
        .compatible = "st,stm32-gpio",
        .path       = "/soc/gpio@40010800",
        .status     = DEVICE_STATUS_READY,
        .criticality = DEVICE_CRIT_WARNING,
        .flags      = 0,
        .prop_count = 2,
        .props      = DEV_gpio_40010800_props,
        .dep_count  = 0,
        .deps       = (const device_id_t*)NULL,
        .reg_count  = 1,
        .regs       = (const device_reg_t*)DEV_gpio_40010800_REGS,
        .irq_count  = 0,
        .irqs       = (const device_irq_t*)NULL,
    },
    [DEV_ID_GPIO_1] = {
        .name       = "gpio@1",
        .label      = "gpio1",
        .compatible = "st,stm32-gpio",
        .path       = "/soc/gpio@1",
        .status     = DEVICE_STATUS_READY,
        .criticality = DEVICE_CRIT_WARNING,
        .flags      = 0,
        .prop_count = 2,
        .props      = DEV_gpio_1_props,
        .dep_count  = 0,
        .deps       = (const device_id_t*)NULL,
        .reg_count  = 1,
        .regs       = (const device_reg_t*)DEV_gpio_1_REGS,
        .irq_count  = 0,
        .irqs       = (const device_irq_t*)NULL,
    },
    [DEV_ID_GPIO_40011000] = {
        .name       = "gpio@40011000",
        .label      = "gpio2",
        .compatible = "st,stm32-gpio",
        .path       = "/soc/gpio@40011000",
        .status     = DEVICE_STATUS_READY,
        .criticality = DEVICE_CRIT_WARNING,
        .flags      = 0,
        .prop_count = 2,
        .props      = DEV_gpio_40011000_props,
        .dep_count  = 0,
        .deps       = (const device_id_t*)NULL,
        .reg_count  = 1,
        .regs       = (const device_reg_t*)DEV_gpio_40011000_REGS,
        .irq_count  = 0,
        .irqs       = (const device_irq_t*)NULL,
    },
};

/* ===== API 实现 ===== */

const device_node_t* board_node_get(device_id_t id) {
    if ((int)id < 0 || (int)id >= DEV_ID_COUNT) return NULL;
    return &s_nodes[id];
}

int board_dev_count(void) { return DEV_ID_COUNT; }

device_id_t board_dev_find(const char* name) {
    if (!name) return -1;
    for (int i = 0; i < DEV_ID_COUNT; i++) {
        if (strcmp(s_nodes[i].name, name) == 0)
            return (device_id_t)i;
    }
    return -1;
}

device_id_t board_dev_find_by_compat(const char* compatible) {
    if (!compatible) return -1;
    for (int i = 0; i < DEV_ID_COUNT; i++) {
        if (s_nodes[i].compatible[0] &&
            strcmp(s_nodes[i].compatible, compatible) == 0)
            return (device_id_t)i;
    }
    return -1;
}

device_id_t board_dev_find_by_label(const char* label) {
    if (!label || !label[0]) return -1;
    for (int i = 0; i < DEV_ID_COUNT; i++) {
        if (s_nodes[i].label[0] &&
            strcmp(s_nodes[i].label, label) == 0)
            return (device_id_t)i;
    }
    return -1;
}
