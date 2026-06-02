#include "board_nodes.h"
#include "board_devtable.h"
#include "device.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

/* ===== 属性表 (静态 .rodata) ===== */

/* / */
static const device_prop_t DEV__props[] = {
    {"model", "ESP32-S3-DevKitC-1"},
};

/* /soc/ws2812@48 */
static const device_prop_t DEV_ws2812_48_props[] = {
    {"gpio", "0x30"},
    {"num-leds", "0x1"},
};

/* ===== 依赖表 ===== */

/* ===== 主节点表 (只读 .rodata) ===== */
static const device_node_t s_nodes[DEV_ID_COUNT] = {
    [DEV_ID_] = {
        .name       = "",
        .label      = "",
        .compatible = "esp32,s3-mini-tree",
        .path       = "/",
        .status     = DEVICE_STATUS_READY,
        .criticality = DEVICE_CRIT_WARNING,
        .flags      = 0,
        .prop_count = 1,
        .props      = DEV__props,
        .dep_count  = 0,
        .deps       = (const device_id_t*)NULL,
    },
    [DEV_ID_WS2812_48] = {
        .name       = "ws2812@48",
        .label      = "ws2812",
        .compatible = "esp32,ws2812",
        .path       = "/soc/ws2812@48",
        .status     = DEVICE_STATUS_READY,
        .criticality = DEVICE_CRIT_WARNING,
        .flags      = 0,
        .prop_count = 2,
        .props      = DEV_ws2812_48_props,
        .dep_count  = 0,
        .deps       = (const device_id_t*)NULL,
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

/* 级联失败: 返回依赖该设备的设备列表 (dtc-lite 未生成, 手动补充) */
const device_id_t* board_cascade_get(device_id_t id, int* count) {
    (void)id;
    if (count) *count = 0;
    return NULL;
}
