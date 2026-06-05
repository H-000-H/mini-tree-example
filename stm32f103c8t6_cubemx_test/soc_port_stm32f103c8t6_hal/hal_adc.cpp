#include "hal_adc.h"
#include "device.h"
#include "driver.h"

#include "adc.h"
#include "stm32f1xx_hal_adc.h"

#define ADC_MAX_COUNT  1

static ADC_HandleTypeDef* const s_adc_handles[ADC_MAX_COUNT] = {
    &hadc1,
};

static hal_adc_t s_adcs[ADC_MAX_COUNT];

static int adc_init(hal_adc_t* adc, const hal_adc_config_t* cfg)
{
    ADC_HandleTypeDef* hadc = (ADC_HandleTypeDef*)adc->_impl;
    if (!hadc) return -1;

    ADC_ChannelConfTypeDef ch = {0};
    ch.Channel      = (uint32_t)cfg->channel;
    ch.Rank         = ADC_REGULAR_RANK_1;
    ch.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;

    return (HAL_ADC_ConfigChannel(hadc, &ch) == HAL_OK) ? 0 : -1;
}

static int adc_read_raw(hal_adc_t* adc, uint32_t* val)
{
    ADC_HandleTypeDef* hadc = (ADC_HandleTypeDef*)adc->_impl;
    if (!hadc || !val) return -1;

    if (HAL_ADC_Start(hadc) != HAL_OK)
        return -1;
    if (HAL_ADC_PollForConversion(hadc, 100) != HAL_OK)
    {
        HAL_ADC_Stop(hadc);
        return -1;
    }
    *val = HAL_ADC_GetValue(hadc);
    HAL_ADC_Stop(hadc);
    return 0;
}

static int adc_read_mv(hal_adc_t* adc, uint32_t* mv)
{
    uint32_t raw;
    if (adc_read_raw(adc, &raw) != 0)
        return -1;
    /* STM32F103C8: 12-bit ADC, 默认参考电压 3.3V */
    *mv = raw * 3300U / 4095U;
    return 0;
}

static int adc_deinit(hal_adc_t* adc)
{
    ADC_HandleTypeDef* hadc = (ADC_HandleTypeDef*)adc->_impl;
    if (!hadc) return -1;
    HAL_ADC_DeInit(hadc);
    adc->_impl = NULL;
    return 0;
}

void hal_adc_init_struct(hal_adc_t* adc)
{
    adc->init      = adc_init;
    adc->read_raw  = adc_read_raw;
    adc->read_mv   = adc_read_mv;
    adc->deinit    = adc_deinit;
    adc->_impl     = NULL;
}

void hal_adc_force_stop(void)
{
    for (int i = 0; i < ADC_MAX_COUNT; i++)
    {
        if (s_adc_handles[i])
        {
            HAL_ADC_DeInit(s_adc_handles[i]);
        }
    }
}

/* ── probe / remove ── */
static int stm32_adc_probe(device_t* dev)
{
    int index = 0;
    if (device_get_prop_int(dev, "adc-index", &index) != 0)
        return -1;
    if (index < 0 || index >= ADC_MAX_COUNT || !s_adc_handles[index])
        return -1;

    MX_ADC1_Init();

    hal_adc_t* adc = &s_adcs[index];
    hal_adc_init_struct(adc);
    adc->_impl = s_adc_handles[index];
    device_set_priv(dev, adc);
    return 0;
}

static int stm32_adc_remove(device_t* dev)
{
    hal_adc_t* adc = (hal_adc_t*)device_get_priv(dev);
    if (adc && adc->_impl)
    {
        HAL_ADC_DeInit((ADC_HandleTypeDef*)adc->_impl);
        adc->_impl = NULL;
    }
    device_set_priv(dev, NULL);
    return 0;
}

DRIVER_REGISTER(stm32_adc, "st,stm32-adc", stm32_adc_probe, stm32_adc_remove);
