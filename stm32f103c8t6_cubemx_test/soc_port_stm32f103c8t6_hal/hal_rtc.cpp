/**
  hal_rtc.cpp — STM32F103 RTC 驱动
  架构: 设备树 + EventBus, 调用 CubeMX 生成的 MX_RTC_Init()
*/

#include "hal_rtc.h"
#include "device.h"
#include "driver.h"
#include "rtc.h"

#define RTC_MAX_COUNT  1

static RTC_HandleTypeDef* const s_rtc_handles[RTC_MAX_COUNT] = { &hrtc };
static hal_rtc_t s_rtcs[RTC_MAX_COUNT];

static int rtc_init(hal_rtc_t* rtc, const hal_rtc_config_t* cfg)
{
    (void)cfg;
    (void)rtc;
    return 0;
}

static int rtc_set_time(hal_rtc_t* rtc, const hal_rtc_time_t* t)
{
    RTC_HandleTypeDef* h = (RTC_HandleTypeDef*)rtc->_impl;
    if (!h || !t) return -1;

    RTC_TimeTypeDef s = {0};
    s.Hours   = (uint8_t)((t->hour / 10) << 4) | (t->hour % 10);
    s.Minutes = (uint8_t)((t->minute / 10) << 4) | (t->minute % 10);
    s.Seconds = (uint8_t)((t->second / 10) << 4) | (t->second % 10);
    if (HAL_RTC_SetTime(h, &s, RTC_FORMAT_BCD) != HAL_OK)
        return -1;

    RTC_DateTypeDef d = {0};
    d.Year    = (uint8_t)(((t->year % 100) / 10) << 4) | (t->year % 10);
    d.Month   = (uint8_t)((t->month / 10) << 4) | (t->month % 10);
    d.Date    = (uint8_t)((t->day / 10) << 4) | (t->day % 10);
    d.WeekDay = t->weekday;
    return (HAL_RTC_SetDate(h, &d, RTC_FORMAT_BCD) == HAL_OK) ? 0 : -1;
}

static int rtc_get_time(hal_rtc_t* rtc, hal_rtc_time_t* t)
{
    RTC_HandleTypeDef* h = (RTC_HandleTypeDef*)rtc->_impl;
    if (!h || !t) return -1;

    RTC_TimeTypeDef s = {0};
    HAL_RTC_GetTime(h, &s, RTC_FORMAT_BCD);
    t->hour   = ((s.Hours >> 4) & 0x0F) * 10 + (s.Hours & 0x0F);
    t->minute = ((s.Minutes >> 4) & 0x0F) * 10 + (s.Minutes & 0x0F);
    t->second = ((s.Seconds >> 4) & 0x0F) * 10 + (s.Seconds & 0x0F);

    RTC_DateTypeDef d = {0};
    HAL_RTC_GetDate(h, &d, RTC_FORMAT_BCD);
    t->year    = 2000 + ((d.Year >> 4) & 0x0F) * 10 + (d.Year & 0x0F);
    t->month   = ((d.Month >> 4) & 0x0F) * 10 + (d.Month & 0x0F);
    t->day     = ((d.Date >> 4) & 0x0F) * 10 + (d.Date & 0x0F);
    t->weekday = d.WeekDay;
    return 0;
}

static int rtc_set_alarm(hal_rtc_t* rtc, const hal_rtc_time_t* alarm,
                          hal_rtc_alarm_callback_t cb, void* user_data)
{
    (void)rtc; (void)alarm; (void)cb; (void)user_data;
    return -1;  /* STM32F103 RTC 闹钟需要额外中断配置, 暂不实现 */
}

static int rtc_cancel_alarm(hal_rtc_t* rtc)
{
    (void)rtc;
    return 0;
}

static int rtc_set_wakeup(hal_rtc_t* rtc, uint32_t seconds)
{
    (void)rtc; (void)seconds;
    return -1;  /* F103 RTC 无硬件唤醒定时器 */
}

static int rtc_deinit(hal_rtc_t* rtc)
{
    RTC_HandleTypeDef* h = (RTC_HandleTypeDef*)rtc->_impl;
    if (!h) return -1;
    HAL_RTC_DeInit(h);
    rtc->_impl = NULL;
    return 0;
}

void hal_rtc_init_struct(hal_rtc_t* rtc)
{
    rtc->init           = rtc_init;
    rtc->set_time       = rtc_set_time;
    rtc->get_time       = rtc_get_time;
    rtc->set_alarm      = rtc_set_alarm;
    rtc->cancel_alarm   = rtc_cancel_alarm;
    rtc->set_wakeup_timer = rtc_set_wakeup;
    rtc->deinit         = rtc_deinit;
    rtc->_impl          = NULL;
}

void hal_rtc_force_stop(void)
{
    for (int i = 0; i < RTC_MAX_COUNT; i++)
    {
        if (s_rtc_handles[i])
            HAL_RTC_DeInit(s_rtc_handles[i]);
    }
}

static int stm32_rtc_probe(device_t* dev)
{
    int index = 0;
    if (device_get_prop_int(dev, "rtc-index", &index) != 0)
        return -1;
    if (index < 0 || index >= RTC_MAX_COUNT || !s_rtc_handles[index])
        return -1;

    MX_RTC_Init();

    hal_rtc_t* rtc = &s_rtcs[index];
    hal_rtc_init_struct(rtc);
    rtc->_impl = s_rtc_handles[index];
    device_set_priv(dev, rtc);
    return 0;
}

static int stm32_rtc_remove(device_t* dev)
{
    hal_rtc_t* rtc = (hal_rtc_t*)device_get_priv(dev);
    if (rtc && rtc->_impl)
    {
        HAL_RTC_DeInit((RTC_HandleTypeDef*)rtc->_impl);
        rtc->_impl = NULL;
    }
    device_set_priv(dev, NULL);
    return 0;
}

DRIVER_REGISTER(stm32_rtc, "st,stm32-rtc", stm32_rtc_probe, stm32_rtc_remove);
