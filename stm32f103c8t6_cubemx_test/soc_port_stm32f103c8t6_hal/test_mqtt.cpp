/**
  test_mqtt.cpp — MQTT 集成测试 (ESP32 AT + Eclipse Paho)
  架构: EventBus 订阅 EVENT_SYS_READY + 引脚从设备树读取
*/

#include "test_mqtt.hpp"
#include "event_bus.hpp"
#include "device.h"
#include "main.h"
#include "wifi_esp32.h"
#include "hal_gpio.h"
#include "mqtt/mqtt_transport.h"
#include "MQTTClient.h"
#include <string.h>
#include <stdio.h>

/* ================================================================== */
/*  WiFi / MQTT 配置 – 从设备树读取 (wifi_config / mqtt_config 节点)    */
/* ================================================================== */

/* ================================================================== */
/*  内部变量                                                            */
/* ================================================================== */
static hal_pin_t       s_led_pin = HAL_MAKE_PIN(2, 13);
static Network         s_network;
static MQTTClient      s_client;
static unsigned char   s_sendbuf[512];
static unsigned char   s_readbuf[512];
static volatile int    s_sub_rcvd;

static void test_msg_handler(MessageData* md)
{
    (void)md;
    s_sub_rcvd = 1;
}

/* ------------------------------------------------------------------ */
/*  WiFi 连接辅助 (AT+CWJAP)                                           */
/* ------------------------------------------------------------------ */
static int wifi_connect(const char* ssid, const char* pass)
{
    char cmd[160];
    int  slen = snprintf(cmd, sizeof(cmd),
                         "AT+CWJAP=\"%s\",\"%s\"\r\n",
                         ssid ? ssid : "", pass ? pass : "");

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
/*  MQTT 测试主体 — EventBus 回调                                      */
/* ================================================================== */
static void mqtt_test_body(const Event&, void*)
{
    int ret = -1;
    const char* ssid       = NULL;
    const char* pass       = NULL;
    const char* broker_host = NULL;
    int         broker_port = 1883;
    const char* client_id  = NULL;
    const char* test_topic = NULL;

    /* 从设备树读取 WiFi 配置 */
    {
        device_t* cfg = device_find_by_label("wifi_config");
        if (cfg)
        {
            device_get_prop_str(cfg, "ssid",     &ssid);
            device_get_prop_str(cfg, "password", &pass);
        }
    }

    /* 从设备树读取 MQTT 配置 */
    {
        device_t* cfg = device_find_by_label("mqtt_config");
        if (cfg)
        {
            device_get_prop_str(cfg, "broker_host", &broker_host);
            device_get_prop_int(cfg, "broker_port", &broker_port);
            device_get_prop_str(cfg, "client_id",   &client_id);
            device_get_prop_str(cfg, "test_topic",  &test_topic);
        }
    }

    wifi_esp32_init();

    if (!ssid || !pass || !broker_host || !client_id || !test_topic)
        goto fail;

    if (wifi_connect(ssid, pass) != 0)
        goto fail;

    memset(&s_network, 0, sizeof(s_network));
    if (mqtt_transport_connect(&s_network, broker_host, broker_port) != 0)
        goto fail;

    MQTTClientInit(&s_client, &s_network, 5000,
                   s_sendbuf, sizeof(s_sendbuf),
                   s_readbuf, sizeof(s_readbuf));

    {
        MQTTPacket_connectData opts = MQTTPacket_connectData_initializer;
        opts.clientID.cstring    = const_cast<char*>(client_id);
        opts.keepAliveInterval   = 30;
        opts.cleansession        = 1;

        if (MQTTConnect(&s_client, &opts) != MQTT_SUCCESS)
            goto fail;
    }

    if (MQTTSubscribe(&s_client, test_topic, QOS0,
                      test_msg_handler) != MQTT_SUCCESS)
        goto fail;

    MQTTYield(&s_client, 500);

    {
        MQTTMessage msg;
        memset(&msg, 0, sizeof(msg));
        msg.qos        = QOS0;
        msg.payload    = (void*)"hello from mini-tree!";
        msg.payloadlen = 21;

        if (MQTTPublish(&s_client, test_topic, &msg) != MQTT_SUCCESS)
            goto fail;
    }

    MQTTYield(&s_client, 2000);
    ret = 0;

fail:
    MQTTDisconnect(&s_client);
    mqtt_transport_disconnect();

    if (ret == 0)
    {
        for (int i = 0; i < 2; i++)
        {
            hal_gpio_set_level(s_led_pin, 0); HAL_Delay(300);
            hal_gpio_set_level(s_led_pin, 1); HAL_Delay(300);
        }
    }
    else
    {
        for (int i = 0; i < 5; i++)
        {
            hal_gpio_set_level(s_led_pin, 0); HAL_Delay(150);
            hal_gpio_set_level(s_led_pin, 1); HAL_Delay(150);
        }
    }
}

/* ================================================================== */
/*  公有入口 — 由 main.c 调用                                          */
/* ================================================================== */
void run_mqtt_test(void)
{
    device_t* led = device_find_by_label("led0");
    if (led) { int p=2, n=13; device_get_prop_int(led,"port",&p); device_get_prop_int(led,"pin",&n); s_led_pin = HAL_MAKE_PIN(p,n); }

    EventBus::getInstance().subscribe(EVENT_SYS_READY, EVENT_SYS_READY,
        mqtt_test_body, nullptr);
}
