#include "hal_timer.h"
#include "device.h"
#include "driver.h"

#include "tim.h"
#include "stm32f1xx_hal_tim.h"

#define TIM_MAX_COUNT  1

static TIM_HandleTypeDef* const s_tim_handles[TIM_MAX_COUNT] = {
    &htim1,
};

static hal_timer_t s_timers[TIM_MAX_COUNT];

static int timer_init(hal_timer_t* timer, const hal_timer_config_t* cfg)
{
    TIM_HandleTypeDef* htim = (TIM_HandleTypeDef*)timer->_impl;
    if (!htim) return -1;

    uint32_t tim_clk = cfg->freq_hz;

    /* 计算预分频使计数值以 1µs 递增 */
    uint32_t prescaler = (tim_clk / 1000000U) - 1U;
    uint32_t period    = cfg->period_us - 1U;

    htim->Init.Prescaler           = (uint16_t)prescaler;
    htim->Init.Period              = period;
    htim->Init.CounterMode         = TIM_COUNTERMODE_UP;
    htim->Init.ClockDivision       = TIM_CLOCKDIVISION_DIV1;
    htim->Init.RepetitionCounter   = 0;
    htim->Init.AutoReloadPreload   = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_Base_Init(htim) != HAL_OK)
        return -1;

    /* 单次模式：不自动重载 */
    if (cfg->mode == HAL_TIMER_MODE_ONESHOT)
        htim->Instance->CR1 &= ~TIM_CR1_ARPE;

    return 0;
}

static int timer_config_channel(hal_timer_t* timer,
                                const hal_timer_channel_config_t* ch_cfg)
{
    TIM_HandleTypeDef* htim = (TIM_HandleTypeDef*)timer->_impl;
    if (!htim) return -1;

    TIM_OC_InitTypeDef oc = {0};
    oc.OCMode     = TIM_OCMODE_PWM1;
    oc.Pulse      = ch_cfg->pulse;
    oc.OCPolarity = (ch_cfg->polarity) ? TIM_OCPOLARITY_LOW : TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(htim, &oc, (uint32_t)ch_cfg->channel) != HAL_OK)
        return -1;

    return 0;
}

static int timer_start(hal_timer_t* timer)
{
    TIM_HandleTypeDef* htim = (TIM_HandleTypeDef*)timer->_impl;
    if (!htim) return -1;
    return (HAL_TIM_Base_Start_IT(htim) == HAL_OK) ? 0 : -1;
}

static int timer_stop(hal_timer_t* timer)
{
    TIM_HandleTypeDef* htim = (TIM_HandleTypeDef*)timer->_impl;
    if (!htim) return -1;
    HAL_TIM_Base_Stop_IT(htim);
    return 0;
}

static int timer_reset(hal_timer_t* timer)
{
    TIM_HandleTypeDef* htim = (TIM_HandleTypeDef*)timer->_impl;
    if (!htim) return -1;
    __HAL_TIM_SET_COUNTER(htim, 0);
    return 0;
}

static int timer_set_callback(hal_timer_t* timer, hal_timer_callback_t cb,
                              void* user_data)
{
    (void)timer; (void)cb; (void)user_data;
    /* TIM 中断回调通过 HAL_TIM_PeriodElapsedCallback 分发，
     * 上层可在此注册自己的回调链。当前保留扩展。 */
    return 0;
}

static int timer_get_counter(hal_timer_t* timer, uint32_t* val)
{
    TIM_HandleTypeDef* htim = (TIM_HandleTypeDef*)timer->_impl;
    if (!htim || !val) return -1;
    *val = __HAL_TIM_GET_COUNTER(htim);
    return 0;
}

static int timer_deinit(hal_timer_t* timer)
{
    TIM_HandleTypeDef* htim = (TIM_HandleTypeDef*)timer->_impl;
    if (!htim) return -1;
    HAL_TIM_Base_DeInit(htim);
    timer->_impl = NULL;
    return 0;
}

void hal_timer_init_struct(hal_timer_t* timer)
{
    timer->init           = timer_init;
    timer->config_channel = timer_config_channel;
    timer->start          = timer_start;
    timer->stop           = timer_stop;
    timer->reset          = timer_reset;
    timer->set_callback   = timer_set_callback;
    timer->get_counter    = timer_get_counter;
    timer->deinit         = timer_deinit;
    timer->_impl          = NULL;
}

void hal_timer_force_stop(void)
{
    for (int i = 0; i < TIM_MAX_COUNT; i++)
    {
        if (s_tim_handles[i])
        {
            HAL_TIM_Base_DeInit(s_tim_handles[i]);
            HAL_TIM_PWM_DeInit(s_tim_handles[i]);
        }
    }
}

/* ── probe / remove ── */
static int stm32_tim_probe(device_t* dev)
{
    int index = 0;
    if (device_get_prop_int(dev, "tim-index", &index) != 0)
        return -1;
    if (index < 0 || index >= TIM_MAX_COUNT || !s_tim_handles[index])
        return -1;

    MX_TIM1_Init();

    hal_timer_t* timer = &s_timers[index];
    hal_timer_init_struct(timer);
    timer->_impl = s_tim_handles[index];
    device_set_priv(dev, timer);
    return 0;
}

static int stm32_tim_remove(device_t* dev)
{
    hal_timer_t* timer = (hal_timer_t*)device_get_priv(dev);
    if (timer && timer->_impl)
    {
        HAL_TIM_Base_DeInit((TIM_HandleTypeDef*)timer->_impl);
        timer->_impl = NULL;
    }
    device_set_priv(dev, NULL);
    return 0;
}

DRIVER_REGISTER(stm32_tim, "st,stm32-tim", stm32_tim_probe, stm32_tim_remove);
