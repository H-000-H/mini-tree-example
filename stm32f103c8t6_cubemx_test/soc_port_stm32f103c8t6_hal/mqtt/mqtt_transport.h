#ifndef MQTT_TRANSPORT_H
#define MQTT_TRANSPORT_H

#include "MQTTBareMetal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 连接 MQTT broker (WiFi 需提前就绪) */
int mqtt_transport_connect(Network* n, const char* host, int port);
void mqtt_transport_disconnect(void);

/* Network 回调 — 暴露给 MQTTClient 使用 */
int mqtt_read_callback(Network* n, unsigned char* buf, int len, int timeout_ms);
int mqtt_write_callback(Network* n, unsigned char* buf, int len, int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_TRANSPORT_H */
