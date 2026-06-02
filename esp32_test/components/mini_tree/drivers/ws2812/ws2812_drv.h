/*
 * ws2812_drv.h — WS2812 LED 驱动 (mini_tree 设备框架)
 *
 * 通过 RMT 外设驱动 ESP32-S3 板载 WS2812 可寻址 RGB LED.
 * 支持 VFS 操作: open / write / ioctl.
 *
 * ioctl 命令:
 *   WS2812_IOCTL_SET_LED   — 设置单颗 LED 颜色
 *   WS2812_IOCTL_SET_ALL   — 设置全部 LED 同色
 *   WS2812_IOCTL_GET_COUNT — 获取 LED 数量
 */
#ifndef WS2812_DRV_H
#define WS2812_DRV_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── ioctl 命令 ── */
#define WS2812_IOCTL_SET_LED     0x01   /* arg: ws2812_led_t* (单颗) */
#define WS2812_IOCTL_SET_ALL     0x02   /* arg: ws2812_led_t* (全部同色) */
#define WS2812_IOCTL_GET_COUNT   0x03   /* arg: int* */

/* ── 单颗 LED 颜色结构 (GRB 顺序) ── */
typedef struct
{
    uint8_t index;   /* LED 序号 (0-based) */
    uint8_t g;       /* Green  */
    uint8_t r;       /* Red    */
    uint8_t b;       /* Blue   */
} ws2812_led_t;

#ifdef __cplusplus
}
#endif

#endif /* WS2812_DRV_H */