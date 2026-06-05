#include "MQTTBareMetal.h"

void TimerInit(Timer* timer)
{
    timer->start_tick = HAL_GetTick();
    timer->timeout_ms = 0;
}

char TimerIsExpired(Timer* timer)
{
    return (HAL_GetTick() - timer->start_tick) >= timer->timeout_ms;
}

void TimerCountdownMS(Timer* timer, unsigned int timeout)
{
    timer->start_tick = HAL_GetTick();
    timer->timeout_ms = timeout;
}

void TimerCountdown(Timer* timer, unsigned int timeout)
{
    TimerCountdownMS(timer, timeout * 1000);
}

int TimerLeftMS(Timer* timer)
{
    uint32_t elapsed = HAL_GetTick() - timer->start_tick;
    if (elapsed >= timer->timeout_ms)
        return 0;
    return timer->timeout_ms - elapsed;
}