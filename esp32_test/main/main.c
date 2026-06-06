

#include <stdio.h>
#include <string.h>

#include "osal.h"
#include "system_init.h"
#include "device.h"
#include "driver.h"
#include "board_devtable.h"
#include "VFS.h"
#include "ws2812_drv.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

/* 强制链接 ws2812 驱动的 probe 函数
 * constructor + used 确保不被 --gc-sections 裁掉 */
extern int board_driver_probe_ws2812(device_t* dev);
static void __attribute__((constructor, used)) _force_link_ws2812(void)
{
    __attribute__((unused)) volatile void* p = (void*)board_driver_probe_ws2812;
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }


    printf("ESP32-S3 mini-tree WS2812 Test\n");

    printf("[1] device_tree_init()...\n");
    device_tree_init();
    printf("[1] OK\n\n");

    printf("[2] board_driver_probe_all()...\n");
    int probed = board_driver_probe_all();
    printf("[2] board_driver_probe_all() returned %d\n\n", probed);

    printf("[3] board_dev_find_by_compat(\"esp32,ws2812\")...\n");
    device_id_t dev_id = board_dev_find_by_compat("esp32,ws2812");
    printf("[3] dev_id = %d\n\n", dev_id);

    if (dev_id < 0) {
        printf("[ERROR] dev_id not found!\n");
        for (;;) { vTaskDelay(1000); }
    }

    printf("[4] board_dev_get(dev_id)...\n");
    device_t* dev = board_dev_get(dev_id);
    printf("[4] dev = %p\n\n", dev);

    if (!dev) 
    {
        printf("[ERROR] dev is NULL!\n");
        for (;;) { vTaskDelay(1000); }
    }

    printf("[5] device_get_status(dev)...\n");
    device_status_t status = device_get_status(dev);
    printf("[5] status = %d\n\n", status);

    /* board_driver_probe_all() 在 probe 成功后自动打开设备,
     * 因此到这里设备可能已经是 DEVICE_STATUS_RUNNING.
     * 仅在非 RUNNING 状态下才调用 device_open. */
    if (status != DEVICE_STATUS_RUNNING) 
    {
        printf("[6] device_open(dev)...\n");
        int open_ret = device_open(dev, NULL);
        printf("[6] device_open returned %d\n\n", open_ret);
        if (open_ret != VFS_OK) 
        {
            printf("[ERROR] device_open failed!\n");
            for (;;) 
            { 
                vTaskDelay(1000); 
            }
        }
    } 
    else 
    {
        printf("[6] device already RUNNING (opened during probe), skipping open\n\n");
    }

    printf("[7] Starting WS2812 loop...\n");
    ws2812_led_t led = {0};
    int i = 0;

    while (1)
    {
        printf("[loop] %d\n", i++);

        led.index = 0;
        led.g = (i % 2 == 0) ? 0x40 : 0x00;
        led.r = (i % 2 == 0) ? 0x00 : 0x40;
        led.b = 0x00;

        printf("[loop] Setting LED: g=%d, r=%d, b=%d\n", led.g, led.r, led.b);
        int ioctl_ret = device_ioctl(dev, WS2812_IOCTL_SET_ALL, &led, sizeof(led), 100);
        printf("[loop] device_ioctl returned %d\n", ioctl_ret);

        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
