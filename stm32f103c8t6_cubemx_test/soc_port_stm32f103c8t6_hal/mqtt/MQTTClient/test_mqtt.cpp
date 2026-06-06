#include "test_mqtt.hpp"
#include "main.h"
#include "wifi_esp32.h"
#include "hal_gpio.h"
#include "mqtt/mqtt_transport.h"

/* MQTTClient-C 库 */
#include "MQTTClient.h"

#include <string.h>
#include <stdio.h>

/* ================================================================== */
/*  用户配置 – 编译时通过 -D 注入或在此修改                              */
/* ================================================================== */
#ifndef MQTT_WIFI_SSID
#define MQTT_WIFI_SSID     "your_ssid"
#endif
#ifndef MQTT_WIFI_PASS
#define MQTT_WIFI_PASS     "your_password"
#endif
#ifndef MQTT_BROKER_HOST
#define MQTT_BROKER_HOST   "test.mosquitto.org"
#endif
#ifndef MQTT_BROKER_PORT
#define MQTT_BROKER_PORT   1883
#endif
#ifndef MQTT_CLIENT_ID
#define MQTT_CLIENT_ID     "mini-tree-test"
#endif
#ifndef MQTT_TEST_TOPIC
#define MQTT_TEST_TOPIC    "mini-tree/test"
#endif

/* ================================================================== */
/*  内部变量                                                            */
/* ================================================================== */
static Network        s_network;
static MQTTClient     s_client;
static unsigned char  s_sendbuf[512];
static unsigned char  s_readbuf[512];
static volatile int   s_sub_rcvd;

static void test_msg_handler(MessageData* md)
{
    (void)md;
    s_sub_rcvd = 1;
}

/* ------------------------------------------------------------------ */
/*  WiFi 连接辅助 (AT+CWJAP)                                           */
/* ------------------------------------------------------------------ */
static int wifi_connect(void)
{
    char cmd[160];
    int  slen = snprintf(cmd, sizeof(cmd),
                         "AT+CWJAP=\"%s\",\"%s\"\r\n",
                         MQTT_WIFI_SSID, MQTT_WIFI_PASS);

    /* 先切 STA 模式 */
    wifi_esp32_send_cmd((const uint8_t*)"AT+CWMODE=1\r\n", 14);

    HAL_UART_Transmit((UART_HandleTypeDef*)wifi_esp32_get_uart(),
                      (uint8_t*)cmd, slen, 1000);

    uint32_t start = HAL_GetTick();
    while (HAL_GetTick() - start < 20000)
    {
        uint8_t buf[128];
        uint16_t len = 0;
        memset(buf, 0, sizeof(buf));
        HAL_UARTEx_ReceiveToIdle((UART_HandleTypeDef*)wifi_esp32_get_uart(),
                                  buf, sizeof(buf) - 1, &len, 2000);
        if (len == 0) continue;
        buf[len] = '\0';

        if (strstr((const char*)buf, "WIFI GOT IP") ||
            strstr((const char*)buf, "WIFI CONNECTED"))
            return 0;
        if (strstr((const char*)buf, "FAIL") ||
            strstr((const char*)buf, "ERROR"))
            return -1;
    }
    return -1;
}

/* ================================================================== */
/*  测试入口                                                            */
/* ================================================================== */
void run_mqtt_test(void)
{
    int ret = -1;

    /* ---- 1. 初始化 ESP32 ---- */
    wifi_esp32_init();

    /* ---- 2. 连 WiFi ---- */
    if (wifi_connect() != 0)
        goto fail;

    /* ---- 3. TCP -> MQTT broker ---- */
    memset(&s_network, 0, sizeof(s_network));
    if (mqtt_transport_connect(&s_network, MQTT_BROKER_HOST,
                               MQTT_BROKER_PORT) != 0)
        goto fail;

    /* ---- 4. 初始化 MQTT 客户端 ---- */
    MQTTClientInit(&s_client, &s_network, 5000,
                   s_sendbuf, sizeof(s_sendbuf),
                   s_readbuf, sizeof(s_readbuf));

    /* ---- 5. MQTT Connect ---- */
    {
        MQTTPacket_connectData opts = MQTTPacket_connectData_initializer;
        opts.clientID.cstring    = MQTT_CLIENT_ID;
        opts.keepAliveInterval   = 30;
        opts.cleansession        = 1;

        if (MQTTConnect(&s_client, &opts) != MQTT_SUCCESS)
            goto fail;
    }

    /* ---- 6. Subscribe ---- */
    if (MQTTSubscribe(&s_client, MQTT_TEST_TOPIC, QOS0,
                      test_msg_handler) != MQTT_SUCCESS)
        goto fail;

    /* 给 broker 一点时间处理 subscribe */
    MQTTYield(&s_client, 500);

    /* ---- 7. Publish ---- */
    {
        MQTTMessage msg;
        memset(&msg, 0, sizeof(msg));
        msg.qos        = QOS0;
        msg.payload    = (void*)"hello from mini-tree!";
        msg.payloadlen = 21;

        if (MQTTPublish(&s_client, MQTT_TEST_TOPIC, &msg) != MQTT_SUCCESS)
            goto fail;
    }

    /* ---- 8. Yield 等一会, 看看能不能收到自己 publish 的消息 ---- */
    MQTTYield(&s_client, 2000);

    /* ---- 9. 结果判断 ---- */
    /* publish 成功返回 MQTT_SUCCESS == 0, 失败返回 FAILURE == -1 */
    ret = 0;  /* 走到这里说明 publish 成功了 */

fail:
    MQTTDisconnect(&s_client);
    mqtt_transport_disconnect();

    /* ---- LED 反馈 (PC13, 与其它 test 一致) ---- */
    {
        #define TEST_LED_PIN  HAL_MAKE_PIN(2, 13)

        if (ret == 0)
        {
            for (int i = 0; i < 2; i++)
            {
                hal_gpio_set_level(TEST_LED_PIN, 0);
                HAL_Delay(300);
                hal_gpio_set_level(TEST_LED_PIN, 1);
                HAL_Delay(300);
            }
        }
        else
        {
            for (int i = 0; i < 5; i++)
            {
                hal_gpio_set_level(TEST_LED_PIN, 0);
                HAL_Delay(150);
                hal_gpio_set_level(TEST_LED_PIN, 1);
                HAL_Delay(150);
            }
        }
    }
}
