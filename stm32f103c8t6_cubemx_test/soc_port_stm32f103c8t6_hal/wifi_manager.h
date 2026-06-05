#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WIFI_SCAN_MAX      20
#define WIFI_SSID_MAX_LEN  33
#define WIFI_BSSID_LEN     6

typedef struct {
    uint8_t  ecn;                            /* 0=OPEN, 1=WEP, 2=WPA_PSK, 3=WPA2_PSK ... */
    int8_t   rssi;
    uint8_t  bssid[WIFI_BSSID_LEN];
    char     ssid[WIFI_SSID_MAX_LEN];
    int      channel;
} wifi_ap_record_t;

int  wifi_init_sta(void);
int  wifi_scan(wifi_ap_record_t *aps, int *count);
int  wifi_connect(const char *ssid, const char *password);
int  wifi_disconnect(void);
int  wifi_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif
