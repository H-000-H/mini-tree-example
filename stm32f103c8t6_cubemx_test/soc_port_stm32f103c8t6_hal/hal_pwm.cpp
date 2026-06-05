/**
  hal_pwm.cpp — STM32F103 PWM 驱动 (TIM2 / TIM3 / TIM4)
  架构: 每个 timer 设备节点提供 4 个 PWM 通道
*/

#include "hal_pwm.h"
#include "device.h"
#include "driver.h"
#include "tim.h"
#include "stm32f1xx_hal_tim.h"
#include <string.h>

/* PWM 通道上下文: 每个 hal_pwm_channel_t._impl 指向此结构 */
typedef struct {
    int tim_idx;
    int ch;
} pwm_ctx_t;

/* ================================================================== */
/*  Timer 实例表                                                       */
/* ================================================================== */
#define PWM_TIM_COUNT  3   /* TIM2, TIM3, TIM4 */

static TIM_HandleTypeDef s_pwm_htim[PWM_TIM_COUNT];
static int               s_pwm_inited[PWM_TIM_COUNT];
static uint32_t          s_pwm_period[PWM_TIM_COUNT];

/* timer-index → 硬件 TIM 映射 */
static TIM_TypeDef* const s_tim_inst[PWM_TIM_COUNT] = {
    TIM2, TIM3, TIM4
};

/* timer-index → RCC 时钟使能宏 */
#define RCC_ENABLE(idx) do { \
    if ((idx) == 0) __HAL_RCC_TIM2_CLK_ENABLE(); \
    else if ((idx) == 1) __HAL_RCC_TIM3_CLK_ENABLE(); \
    else if ((idx) == 2) __HAL_RCC_TIM4_CLK_ENABLE(); \
} while(0)

/* timer-index → 通道默认 GPIO 配置 (第一组引脚) */
static void pwm_config_gpio(int tim_idx, int ch, int alt_mode)
{
    GPIO_InitTypeDef g = {0};
    g.Mode  = GPIO_MODE_AF_PP;
    g.Speed = GPIO_SPEED_FREQ_LOW;

    /* TIM2: PA0-3 */
    if (tim_idx == 0) {
        if (ch == 0) { g.Pin = GPIO_PIN_0; HAL_GPIO_Init(GPIOA, &g); }
        if (ch == 1) { g.Pin = GPIO_PIN_1; HAL_GPIO_Init(GPIOA, &g); }
        if (ch == 2) { g.Pin = GPIO_PIN_2; HAL_GPIO_Init(GPIOA, &g); }
        if (ch == 3) { g.Pin = GPIO_PIN_3; HAL_GPIO_Init(GPIOA, &g); }
    }
    /* TIM3: PA6-7, PB0-1 */
    else if (tim_idx == 1) {
        if (ch == 0) { g.Pin = GPIO_PIN_6; HAL_GPIO_Init(GPIOA, &g); }
        if (ch == 1) { g.Pin = GPIO_PIN_7; HAL_GPIO_Init(GPIOA, &g); }
        if (ch == 2) { g.Pin = GPIO_PIN_0; HAL_GPIO_Init(GPIOB, &g); }
        if (ch == 3) { g.Pin = GPIO_PIN_1; HAL_GPIO_Init(GPIOB, &g); }
    }
    /* TIM4: PB6-9 */
    else if (tim_idx == 2) {
        if (ch == 0) { g.Pin = GPIO_PIN_6; HAL_GPIO_Init(GPIOB, &g); }
        if (ch == 1) { g.Pin = GPIO_PIN_7; HAL_GPIO_Init(GPIOB, &g); }
        if (ch == 2) { g.Pin = GPIO_PIN_8; HAL_GPIO_Init(GPIOB, &g); }
        if (ch == 3) { g.Pin = GPIO_PIN_9; HAL_GPIO_Init(GPIOB, &g); }
    }
    (void)alt_mode;
}

static uint32_t ch_to_hal_ch(int ch)
{
    switch (ch) {
        case 0:  return TIM_CHANNEL_1;
        case 1:  return TIM_CHANNEL_2;
        case 2:  return TIM_CHANNEL_3;
        case 3:  return TIM_CHANNEL_4;
        default: return TIM_CHANNEL_1;
    }
}

/* ================================================================== */
/*  hal_pwm_channel_t 函数指针实现                                      */
/* ================================================================== */

static int pwm_ch_init(hal_pwm_channel_t* pwm, const hal_pwm_config_t* cfg)
{
    pwm_ctx_t* ctx = (pwm_ctx_t*)pwm->_impl;
    if (!ctx || !cfg) return -1;

    int idx = ctx->tim_idx;
    int ch  = ctx->ch;

    if (!s_pwm_inited[idx])
    {
        RCC_ENABLE(idx);

        memset(&s_pwm_htim[idx], 0, sizeof(s_pwm_htim[idx]));
        s_pwm_htim[idx].Instance = s_tim_inst[idx];
        s_pwm_htim[idx].Init.Prescaler         = 72 - 1;          /* 72MHz / 72 = 1MHz */
        s_pwm_htim[idx].Init.CounterMode       = TIM_COUNTERMODE_UP;
        s_pwm_htim[idx].Init.Period            = 10000;           /* 1MHz / 10000 = 100Hz default */
        s_pwm_htim[idx].Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
        s_pwm_htim[idx].Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

        if (HAL_TIM_PWM_Init(&s_pwm_htim[idx]) != HAL_OK)
            return -1;

        s_pwm_period[idx] = s_pwm_htim[idx].Init.Period;
        s_pwm_inited[idx] = 1;
    }

    /* 若用户指定了频率, 重新计算 Prescaler + Period */
    if (cfg->freq_hz > 0)
    {
        uint32_t prescaler = 72;
        uint32_t period = (72000000U / prescaler) / (uint32_t)cfg->freq_hz;
        if (period > 65535) { prescaler = 720; period = (72000000U / prescaler) / (uint32_t)cfg->freq_hz; }
        if (period > 65535) { prescaler = 7200; period = (72000000U / prescaler) / (uint32_t)cfg->freq_hz; }
        if (period < 1) period = 1;

        s_pwm_htim[idx].Init.Prescaler = prescaler - 1;
        s_pwm_htim[idx].Init.Period    = period;
        s_pwm_period[idx] = period;

        if (HAL_TIM_PWM_Init(&s_pwm_htim[idx]) != HAL_OK)
            return -1;
    }

    pwm_config_gpio(idx, ch, 0);

    TIM_OC_InitTypeDef oc = {0};
    oc.OCMode     = TIM_OCMODE_PWM1;
    oc.Pulse      = 0;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(&s_pwm_htim[idx], &oc, ch_to_hal_ch(ch)) != HAL_OK)
        return -1;

    HAL_TIM_PWM_Start(&s_pwm_htim[idx], ch_to_hal_ch(ch));
    return 0;
}

static int pwm_ch_set_duty(hal_pwm_channel_t* pwm, uint32_t duty)
{
    pwm_ctx_t* ctx = (pwm_ctx_t*)pwm->_impl;
    if (!ctx) return -1;

    uint32_t period = s_pwm_period[ctx->tim_idx];
    if (period == 0) return -1;
    if (duty > period) duty = period;

    __HAL_TIM_SET_COMPARE(&s_pwm_htim[ctx->tim_idx], ch_to_hal_ch(ctx->ch), duty);
    return 0;
}

static int pwm_ch_get_duty(hal_pwm_channel_t* pwm, uint32_t* duty)
{
    pwm_ctx_t* ctx = (pwm_ctx_t*)pwm->_impl;
    if (!ctx || !duty) return -1;

    *duty = __HAL_TIM_GET_COMPARE(&s_pwm_htim[ctx->tim_idx], ch_to_hal_ch(ctx->ch));
    return 0;
}

static int pwm_ch_deinit(hal_pwm_channel_t* pwm)
{
    pwm_ctx_t* ctx = (pwm_ctx_t*)pwm->_impl;
    if (!ctx) return -1;

    HAL_TIM_PWM_Stop(&s_pwm_htim[ctx->tim_idx], ch_to_hal_ch(ctx->ch));
    return 0;
}

/* ================================================================== */
/*  hal_pwm 公共接口                                                    */
/* ================================================================== */

void hal_pwm_init_struct(hal_pwm_channel_t* pwm)
{
    pwm->init     = pwm_ch_init;
    pwm->set_duty = pwm_ch_set_duty;
    pwm->get_duty = pwm_ch_get_duty;
    pwm->deinit   = pwm_ch_deinit;
    pwm->_impl    = NULL;
}

void hal_pwm_force_stop_all(void)
{
    for (int i = 0; i < PWM_TIM_COUNT; i++)
    {
        if (s_pwm_inited[i])
        {
            HAL_TIM_PWM_Stop(&s_pwm_htim[i], TIM_CHANNEL_1);
            HAL_TIM_PWM_Stop(&s_pwm_htim[i], TIM_CHANNEL_2);
            HAL_TIM_PWM_Stop(&s_pwm_htim[i], TIM_CHANNEL_3);
            HAL_TIM_PWM_Stop(&s_pwm_htim[i], TIM_CHANNEL_4);
            HAL_TIM_PWM_DeInit(&s_pwm_htim[i]);
            s_pwm_inited[i] = 0;
        }
    }
}

/* ================================================================== */
/*  probe / remove — 每个 timer 创建一个设备, 内建 4 通道              */
/* ================================================================== */

#define PWM_CH_PER_TIM  4

/* 运行时实例: 最多 PWM_TIM_COUNT 个 timer × 4 通道 */
static hal_pwm_channel_t s_pwm_channels[PWM_TIM_COUNT][PWM_CH_PER_TIM];
/* 每个通道的上下文: { timer_index, channel_index } */
static pwm_ctx_t s_pwm_ctx[PWM_TIM_COUNT][PWM_CH_PER_TIM];

static int stm32_pwm_probe(device_t* dev)
{
    int idx = 0;
    if (device_get_prop_int(dev, "timer-index", &idx) != 0)
        return -1;
    if (idx < 0 || idx >= PWM_TIM_COUNT)
        return -1;

    /* 初始化 4 个通道, 挂在同一个 timer 上 */
    for (int ch = 0; ch < 4; ch++)
    {
        s_pwm_ctx[idx][ch].tim_idx = idx;
        s_pwm_ctx[idx][ch].ch      = ch;

        hal_pwm_channel_t* pwm = &s_pwm_channels[idx][ch];
        hal_pwm_init_struct(pwm);
        pwm->_impl = &s_pwm_ctx[idx][ch];
    }

    device_set_priv(dev, &s_pwm_channels[idx][0]);  /* 暴露通道 0 作为设备私有数据 */
    return 0;
}

static int stm32_pwm_remove(device_t* dev)
{
    (void)dev;
    hal_pwm_force_stop_all();
    return 0;
}

DRIVER_REGISTER(stm32_pwm, "st,stm32-pwm", stm32_pwm_probe, stm32_pwm_remove);
