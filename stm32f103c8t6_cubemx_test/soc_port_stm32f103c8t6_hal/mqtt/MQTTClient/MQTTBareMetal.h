#if !defined(MQTTBareMetal_H)
#define MQTTBareMetal_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t HAL_GetTick(void);

typedef struct Timer
{
    uint32_t start_tick;
    uint32_t timeout_ms;
} Timer;

typedef struct Network Network;

struct Network
{
    int (*mqttread)(Network*, unsigned char* read_buffer, int, int);
    int (*mqttwrite)(Network*, unsigned char* send_buffer, int, int);
};

void TimerInit(Timer*);
char TimerIsExpired(Timer*);
void TimerCountdownMS(Timer*, unsigned int);
void TimerCountdown(Timer*, unsigned int);
int TimerLeftMS(Timer*);

#ifdef __cplusplus
}
#endif

#endif