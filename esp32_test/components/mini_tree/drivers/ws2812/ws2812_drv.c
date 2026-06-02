/*
 * ws2812_drv.c — WS2812 LED 驱动 (ESP32-S3 RMT)
 *
 * 使用 ESP32-S3 RMT 外设驱动 WS2812 可寻址 RGB LED.
 * 通过 mini_tree 设备框架注册, 支持 VFS 操作.
 *
 * GPIOD: GPIO48 (ESP32-S3-DevKitC-1 板载)
 * RMT:   80MHz 时钟, 字节编码器
 * 协议:  GRB 字节序, MSB first
 */
#include "ws2812_drv.h"

#include "device.h"
#include "driver.h"
#include "VFS.h"

#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "driver/gpio.h"
#include "esp_check.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ── RMT 时钟配置 ── */
#define RMT_RESOLUTION_HZ      (10 * 1000 * 1000)  /* 10MHz → 100ns/tick */
#define WS2812_T0H_TICKS       4    /* 0.4µs */
#define WS2812_T0L_TICKS       9    /* 0.85µs */
#define WS2812_T1H_TICKS       8    /* 0.8µs */
#define WS2812_T1L_TICKS       5    /* 0.45µs */
#define WS2812_RESET_TICKS     500  /* 50µs */

/* ── 驱动私有数据 ── */
typedef struct
{
    rmt_channel_handle_t   tx_chan;
    rmt_encoder_handle_t   bytes_encoder;
    rmt_encoder_handle_t   copy_encoder;
    rmt_symbol_word_t      reset_code;
    gpio_num_t             gpio;
    int                    num_leds;
    uint8_t*               led_buf;
} ws2812_priv_t;

/* 常量: 单颗 LED 需要 3 字节 (GRB) */
#define BYTES_PER_LED  3

/* ── 内部辅助 ── */
static const char* TAG = "ws2812_drv";

/* ── VFS 操作实现 ── */

static int ws2812_open(device_t* dev, void* arg)
{
    printf("!!! [ws2812_open] CALLED !!! dev=%p, arg=%p\n", dev, arg);
    (void)arg;
    ws2812_priv_t* priv = (ws2812_priv_t*)device_get_priv(dev);
    printf("!!! [ws2812_open] priv=%p\n", priv);
    if (!priv) return VFS_ERR_INVAL;

    /* 全黑初始化 */
    memset(priv->led_buf, 0, priv->num_leds * BYTES_PER_LED);

    rmt_transmit_config_t tx_cfg = {
        .loop_count = 0,
    };

    /* 发送全黑数据 */
    printf("!!! [ws2812_open] transmitting black data...\n");
    esp_err_t ret = rmt_transmit(priv->tx_chan, priv->bytes_encoder,
                                  priv->led_buf,
                                  priv->num_leds * BYTES_PER_LED,
                                  &tx_cfg);
    printf("!!! [ws2812_open] rmt_transmit 1 ret=%d\n", ret);
    ESP_ERROR_CHECK(ret);
    ret = rmt_tx_wait_all_done(priv->tx_chan, 100);
    printf("!!! [ws2812_open] rmt_tx_wait_all_done 1 ret=%d\n", ret);
    ESP_ERROR_CHECK(ret);

    /* 发送 RESET 码 */
    printf("!!! [ws2812_open] transmitting reset...\n");
    ret = rmt_transmit(priv->tx_chan, priv->copy_encoder,
                       &priv->reset_code,
                       sizeof(priv->reset_code), &tx_cfg);
    printf("!!! [ws2812_open] rmt_transmit 2 ret=%d\n", ret);
    ESP_ERROR_CHECK(ret);
    ret = rmt_tx_wait_all_done(priv->tx_chan, 100);
    printf("!!! [ws2812_open] rmt_tx_wait_all_done 2 ret=%d\n", ret);
    ESP_ERROR_CHECK(ret);

    printf("!!! [ws2812_open] OK\n");
    return VFS_OK;
}

static int ws2812_close(device_t* dev)
{
    printf("!!! [ws2812_close] CALLED !!! dev=%p\n", dev);
    ws2812_priv_t* priv = (ws2812_priv_t*)device_get_priv(dev);
    if (!priv) return VFS_ERR_INVAL;

    /* 关闭前灭灯 */
    memset(priv->led_buf, 0, priv->num_leds * BYTES_PER_LED);
    rmt_transmit_config_t tx_cfg = { .loop_count = 0 };
    rmt_transmit(priv->tx_chan, priv->bytes_encoder,
                 priv->led_buf, priv->num_leds * BYTES_PER_LED, &tx_cfg);
    rmt_tx_wait_all_done(priv->tx_chan, 100);
    rmt_transmit(priv->tx_chan, priv->copy_encoder,
                 &priv->reset_code, sizeof(priv->reset_code), &tx_cfg);
    rmt_tx_wait_all_done(priv->tx_chan, 100);

    printf("!!! [ws2812_close] OK\n");
    return VFS_OK;
}

static int ws2812_write(device_t* dev, const void* buf, size_t len, uint32_t timeout_ms)
{
    printf("!!! [ws2812_write] CALLED !!! dev=%p, buf=%p, len=%d\n", dev, buf, (int)len);
    ws2812_priv_t* priv = (ws2812_priv_t*)device_get_priv(dev);
    if (!priv || !buf) return VFS_ERR_INVAL;

    size_t max_bytes = priv->num_leds * BYTES_PER_LED;
    if (len > max_bytes) len = max_bytes;

    memcpy(priv->led_buf, buf, len);

    rmt_transmit_config_t tx_cfg = { .loop_count = 0 };
    esp_err_t ret = rmt_transmit(priv->tx_chan, priv->bytes_encoder,
                                  priv->led_buf, max_bytes, &tx_cfg);
    if (ret != ESP_OK) return VFS_ERR_IO;

    ret = rmt_tx_wait_all_done(priv->tx_chan, (int)timeout_ms);
    if (ret != ESP_OK) return VFS_ERR_TIMEOUT;

    ret = rmt_transmit(priv->tx_chan, priv->copy_encoder,
                       &priv->reset_code, sizeof(priv->reset_code), &tx_cfg);
    if (ret != ESP_OK) return VFS_ERR_IO;

    ret = rmt_tx_wait_all_done(priv->tx_chan, (int)timeout_ms);
    if (ret != ESP_OK) return VFS_ERR_TIMEOUT;

    printf("!!! [ws2812_write] OK, wrote %d bytes\n", (int)len);
    return (int)len;
}

static int ws2812_ioctl(device_t* dev, int cmd, void* arg, size_t arg_len, uint32_t timeout_ms)
{
    printf("!!! [ws2812_ioctl] CALLED !!! cmd=%d, dev=%p, arg=%p, arg_len=%d, timeout=%d\n", cmd, dev, arg, (int)arg_len, (int)timeout_ms);
    ws2812_priv_t* priv = (ws2812_priv_t*)device_get_priv(dev);
    printf("!!! [ws2812_ioctl] priv=%p\n", priv);
    if (!priv) return VFS_ERR_INVAL;

    switch (cmd) {
    case WS2812_IOCTL_GET_COUNT: {
        printf("!!! [ws2812_ioctl] cmd=WS2812_IOCTL_GET_COUNT\n");
        if (!arg || arg_len < sizeof(int)) return VFS_ERR_INVAL;
        *(int*)arg = priv->num_leds;
        printf("!!! [ws2812_ioctl] num_leds=%d\n", priv->num_leds);
        return VFS_OK;
    }

    case WS2812_IOCTL_SET_LED: {
        printf("!!! [ws2812_ioctl] cmd=WS2812_IOCTL_SET_LED\n");
        if (!arg || arg_len < sizeof(ws2812_led_t)) return VFS_ERR_INVAL;
        ws2812_led_t* led = (ws2812_led_t*)arg;
        printf("!!! [ws2812_ioctl] led index=%d, g=%d, r=%d, b=%d\n", led->index, led->g, led->r, led->b);
        if (led->index >= (uint8_t)priv->num_leds) return VFS_ERR_INVAL;

        uint8_t* p = &priv->led_buf[led->index * BYTES_PER_LED];
        p[0] = led->g;
        p[1] = led->r;
        p[2] = led->b;
        printf("!!! [ws2812_ioctl] led_buf set: [0]=%d, [1]=%d, [2]=%d\n", p[0], p[1], p[2]);

        /* 立即刷新 */
        printf("!!! [ws2812_ioctl] transmitting LED data...\n");
        rmt_transmit_config_t tx_cfg = { .loop_count = 0 };
        esp_err_t ret = rmt_transmit(priv->tx_chan, priv->bytes_encoder,
                                      priv->led_buf,
                                      priv->num_leds * BYTES_PER_LED, &tx_cfg);
        printf("!!! [ws2812_ioctl] rmt_transmit 1 ret=%d\n", ret);
        if (ret != ESP_OK) return VFS_ERR_IO;
        ret = rmt_tx_wait_all_done(priv->tx_chan, (int)timeout_ms);
        printf("!!! [ws2812_ioctl] rmt_tx_wait_all_done 1 ret=%d\n", ret);
        if (ret != ESP_OK) return VFS_ERR_TIMEOUT;
        ret = rmt_transmit(priv->tx_chan, priv->copy_encoder,
                           &priv->reset_code, sizeof(priv->reset_code), &tx_cfg);
        printf("!!! [ws2812_ioctl] rmt_transmit 2 ret=%d\n", ret);
        if (ret != ESP_OK) return VFS_ERR_IO;
        ret = rmt_tx_wait_all_done(priv->tx_chan, (int)timeout_ms);
        printf("!!! [ws2812_ioctl] rmt_tx_wait_all_done 2 ret=%d\n", ret);
        return VFS_OK;
    }

    case WS2812_IOCTL_SET_ALL: {
        printf("!!! [ws2812_ioctl] cmd=WS2812_IOCTL_SET_ALL\n");
        if (!arg || arg_len < sizeof(ws2812_led_t)) {
            printf("!!! [ws2812_ioctl] ERROR: invalid arg/arg_len\n");
            return VFS_ERR_INVAL;
        }
        ws2812_led_t* led = (ws2812_led_t*)arg;
        printf("!!! [ws2812_ioctl] led index=%d, g=%d, r=%d, b=%d\n", led->index, led->g, led->r, led->b);
        for (int i = 0; i < priv->num_leds; i++) {
            uint8_t* p = &priv->led_buf[i * BYTES_PER_LED];
            p[0] = led->g;
            p[1] = led->r;
            p[2] = led->b;
            printf("!!! [ws2812_ioctl] led_buf[%d] set: [0]=%d, [1]=%d, [2]=%d\n", i, p[0], p[1], p[2]);
        }

        printf("!!! [ws2812_ioctl] transmitting LED data...\n");
        rmt_transmit_config_t tx_cfg = { .loop_count = 0 };
        esp_err_t ret = rmt_transmit(priv->tx_chan, priv->bytes_encoder,
                                      priv->led_buf,
                                      priv->num_leds * BYTES_PER_LED, &tx_cfg);
        printf("!!! [ws2812_ioctl] rmt_transmit 1 ret=%d\n", ret);
        if (ret != ESP_OK) return VFS_ERR_IO;
        ret = rmt_tx_wait_all_done(priv->tx_chan, (int)timeout_ms);
        printf("!!! [ws2812_ioctl] rmt_tx_wait_all_done 1 ret=%d\n", ret);
        if (ret != ESP_OK) return VFS_ERR_TIMEOUT;
        ret = rmt_transmit(priv->tx_chan, priv->copy_encoder,
                           &priv->reset_code, sizeof(priv->reset_code), &tx_cfg);
        printf("!!! [ws2812_ioctl] rmt_transmit 2 ret=%d\n", ret);
        if (ret != ESP_OK) return VFS_ERR_IO;
        ret = rmt_tx_wait_all_done(priv->tx_chan, (int)timeout_ms);
        printf("!!! [ws2812_ioctl] rmt_tx_wait_all_done 2 ret=%d\n", ret);
        return VFS_OK;
    }

    default:
        printf("!!! [ws2812_ioctl] ERROR: invalid cmd=%d\n", cmd);
        return VFS_ERR_INVAL;
    }
}

/* ── VFS 操作表 ── */
static const file_operation_t ws2812_ops = {
    .init    = NULL,
    .open    = ws2812_open,
    .close   = ws2812_close,
    .write   = ws2812_write,
    .read    = NULL,
    .ioctl   = ws2812_ioctl,
    .suspend = NULL,
    .resume  = NULL,
};

/* ── probe / remove ── */

static int ws2812_probe(device_t* dev)
{
    printf("!!! [ws2812_probe] CALLED !!! dev=%p\n", dev);

    ws2812_priv_t* priv = calloc(1, sizeof(ws2812_priv_t));
    if (!priv) {
        printf("[%s] ERROR: calloc failed for priv\n", TAG);
        return VFS_ERR_NOMEM;
    }
    printf("[%s] priv allocated at %p\n", TAG, priv);

    /* 解析 DTS 属性 */
    int gpio_val = 48;  /* 默认 GPIO48 */
    int ret_gpio = device_get_prop_int(dev, "gpio", &gpio_val);
    printf("[%s] device_get_prop_int(gpio) ret=%d, val=%d\n", TAG, ret_gpio, gpio_val);
    priv->gpio = (gpio_num_t)gpio_val;

    int num_leds = 1;
    int ret_num = device_get_prop_int(dev, "num-leds", &num_leds);
    printf("[%s] device_get_prop_int(num-leds) ret=%d, val=%d\n", TAG, ret_num, num_leds);
    priv->num_leds = (num_leds > 0) ? num_leds : 1;

    printf("[%s] probe: gpio=%d, num_leds=%d\n", TAG, gpio_val, priv->num_leds);

    /* 分配 LED 缓冲区 */
    priv->led_buf = calloc(priv->num_leds, BYTES_PER_LED);
    if (!priv->led_buf) {
        printf("[%s] ERROR: calloc failed for led_buf\n", TAG);
        free(priv);
        return VFS_ERR_NOMEM;
    }
    printf("[%s] led_buf allocated at %p, size=%d bytes\n", TAG, priv->led_buf, (int)(priv->num_leds*BYTES_PER_LED));

    /* 配置 RMT TX 通道 */
    rmt_tx_channel_config_t tx_chan_cfg = {
        .clk_src          = RMT_CLK_SRC_DEFAULT,
        .gpio_num         = priv->gpio,
        .mem_block_symbols = 64,
        .resolution_hz    = RMT_RESOLUTION_HZ,
        .trans_queue_depth = 4,
        .flags.invert_out  = false,
        .flags.with_dma    = false,
    };
    printf("[%s] calling rmt_new_tx_channel (gpio=%d)...\n", TAG, priv->gpio);
    esp_err_t ret = rmt_new_tx_channel(&tx_chan_cfg, &priv->tx_chan);
    printf("[%s] rmt_new_tx_channel ret=%d, tx_chan=%p\n", TAG, ret, priv->tx_chan);
    if (ret != ESP_OK) {
        printf("[%s] rmt_new_tx_channel failed: %d\n", TAG, ret);
        free(priv->led_buf);
        free(priv);
        return VFS_ERR_IO;
    }

    /* 创建字节编码器 (WS2812 协议) */
    rmt_bytes_encoder_config_t byte_enc_cfg = {
        .bit0 = {
            .duration0 = WS2812_T0H_TICKS,
            .level0    = 1,
            .duration1 = WS2812_T0L_TICKS,
            .level1    = 0,
        },
        .bit1 = {
            .duration0 = WS2812_T1H_TICKS,
            .level0    = 1,
            .duration1 = WS2812_T1L_TICKS,
            .level1    = 0,
        },
        .flags.msb_first = 1,
    };
    printf("[%s] calling rmt_new_bytes_encoder...\n", TAG);
    ret = rmt_new_bytes_encoder(&byte_enc_cfg, &priv->bytes_encoder);
    printf("[%s] rmt_new_bytes_encoder ret=%d, bytes_encoder=%p\n", TAG, ret, priv->bytes_encoder);
    if (ret != ESP_OK) {
        printf("[%s] rmt_new_bytes_encoder failed: %d\n", TAG, ret);
        rmt_del_channel(priv->tx_chan);
        free(priv->led_buf);
        free(priv);
        return VFS_ERR_IO;
    }

    /* 创建 RESET 码编码器 */
    priv->reset_code = (rmt_symbol_word_t){
        .duration0 = WS2812_RESET_TICKS,
        .level0    = 0,
        .duration1 = 0,
        .level1    = 0,
    };
    rmt_copy_encoder_config_t copy_enc_cfg = {};
    printf("[%s] calling rmt_new_copy_encoder...\n", TAG);
    ret = rmt_new_copy_encoder(&copy_enc_cfg, &priv->copy_encoder);
    printf("[%s] rmt_new_copy_encoder ret=%d, copy_encoder=%p\n", TAG, ret, priv->copy_encoder);
    if (ret != ESP_OK) {
        printf("[%s] rmt_new_copy_encoder failed: %d\n", TAG, ret);
        rmt_del_encoder(priv->bytes_encoder);
        rmt_del_channel(priv->tx_chan);
        free(priv->led_buf);
        free(priv);
        return VFS_ERR_IO;
    }

    /* 启用 RMT 通道 */
    printf("[%s] calling rmt_enable...\n", TAG);
    ret = rmt_enable(priv->tx_chan);
    printf("[%s] rmt_enable ret=%d\n", TAG, ret);
    if (ret != ESP_OK) {
        printf("[%s] rmt_enable failed: %d\n", TAG, ret);
        rmt_del_encoder(priv->copy_encoder);
        rmt_del_encoder(priv->bytes_encoder);
        rmt_del_channel(priv->tx_chan);
        free(priv->led_buf);
        free(priv);
        return VFS_ERR_IO;
    }

    /* 注册 VFS 操作表 */
    device_set_priv(dev, priv);
    dev->ops = &ws2812_ops;

    printf("[%s] probe OK\n", TAG);
    return VFS_OK;
}

static int ws2812_remove(device_t* dev)
{
    printf("!!! [ws2812_remove] CALLED !!! dev=%p\n", dev);
    ws2812_priv_t* priv = (ws2812_priv_t*)device_get_priv(dev);
    if (!priv) return VFS_ERR_INVAL;

    /* 灭灯 */
    memset(priv->led_buf, 0, priv->num_leds * BYTES_PER_LED);
    rmt_transmit_config_t tx_cfg = { .loop_count = 0 };
    rmt_transmit(priv->tx_chan, priv->bytes_encoder,
                 priv->led_buf, priv->num_leds * BYTES_PER_LED, &tx_cfg);
    rmt_tx_wait_all_done(priv->tx_chan, 100);
    rmt_transmit(priv->tx_chan, priv->copy_encoder,
                 &priv->reset_code, sizeof(priv->reset_code), &tx_cfg);
    rmt_tx_wait_all_done(priv->tx_chan, 100);

    rmt_disable(priv->tx_chan);
    rmt_del_encoder(priv->copy_encoder);
    rmt_del_encoder(priv->bytes_encoder);
    rmt_del_channel(priv->tx_chan);

    free(priv->led_buf);
    free(priv);

    device_ops_unregister(dev);
    printf("[%s] remove OK\n", TAG);
    return VFS_OK;
}

/* ── 驱动注册 ──
 * dtc-lite.py 扫描此宏, 生成 board_probe 调度表.
 */
DRIVER_REGISTER(ws2812, "esp32,ws2812", ws2812_probe, ws2812_remove)
