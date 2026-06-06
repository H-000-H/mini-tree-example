#include "board_nodes.h"
#include "board_devtable.h"
#include "device.h"

/* ===== probe 函数声明 ===== */
extern int __attribute__((weak)) board_driver_probe_stm32_gpio(device_t* dev);
extern int __attribute__((weak)) board_driver_probe_stm32_gpio(device_t* dev);
extern int __attribute__((weak)) board_driver_probe_stm32_gpio(device_t* dev);

/* ===== remove 函数声明 ===== */
extern int __attribute__((weak)) board_driver_remove_stm32_gpio(device_t* dev);
extern int __attribute__((weak)) board_driver_remove_stm32_gpio(device_t* dev);
extern int __attribute__((weak)) board_driver_remove_stm32_gpio(device_t* dev);

/* ===== probe 函数表 (按 DEV_ID 索引, .rodata) ===== */
static const probe_fn_t s_probe_fns[DEV_ID_COUNT] = {
    [DEV_ID_GPIO_40010800] = board_driver_probe_stm32_gpio,
    [DEV_ID_GPIO_1] = board_driver_probe_stm32_gpio,
    [DEV_ID_GPIO_40011000] = board_driver_probe_stm32_gpio,
};

/* ===== remove 函数表 (按 DEV_ID 索引, .rodata) ===== */
static const remove_fn_t s_remove_fns[DEV_ID_COUNT] = {
    [DEV_ID_GPIO_40010800] = board_driver_remove_stm32_gpio,
    [DEV_ID_GPIO_1] = board_driver_remove_stm32_gpio,
    [DEV_ID_GPIO_40011000] = board_driver_remove_stm32_gpio,
};

/* ===== probe 顺序 (按依赖拓扑排序) ===== */
static const device_id_t s_probe_order[DEV_ID_COUNT] = {
    DEV_ID_GPIO_40010800,
    DEV_ID_GPIO_1,
    DEV_ID_GPIO_40011000,
};

/* ===== API ===== */

probe_fn_t board_probe_get_fn(device_id_t id) {
    if ((int)id < 0 || (int)id >= DEV_ID_COUNT) return NULL;
    return s_probe_fns[id];
}

remove_fn_t board_remove_get_fn(device_id_t id) {
    if ((int)id < 0 || (int)id >= DEV_ID_COUNT) return NULL;
    return s_remove_fns[id];
}

const device_id_t* board_probe_order(void) {
    return s_probe_order;
}

int board_probe_order_count(void) {
    return DEV_ID_COUNT;
}

/* ===== 故障传播表 (编译期预计算, 替代运行时 BFS) ===== */
