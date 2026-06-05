/**
  hal_can.cpp — STM32F103 bxCAN 驱动
  架构: 设备树 + EventBus, 调用 CubeMX 生成的 MX_CAN_Init()
*/

#include "hal_can.h"
#include "device.h"
#include "driver.h"
#include "can.h"

#define CAN_MAX_COUNT  1

static CAN_HandleTypeDef* const s_can_handles[CAN_MAX_COUNT] = { &hcan };

static hal_can_t s_cans[CAN_MAX_COUNT];

static int can_init(hal_can_t* can, const hal_can_config_t* cfg)
{
    (void)cfg;
    CAN_HandleTypeDef* h = (CAN_HandleTypeDef*)can->_impl;
    if (!h) return -1;

    if (HAL_CAN_Start(h) != HAL_OK)
        return -1;
    return 0;
}

static int can_send(hal_can_t* can, const hal_can_msg_t* msg, uint32_t timeout_ms)
{
    CAN_HandleTypeDef* h = (CAN_HandleTypeDef*)can->_impl;
    if (!h || !msg) return -1;

    CAN_TxHeaderTypeDef tx = {0};
    tx.ExtId    = (msg->frame_type == HAL_CAN_FRAME_EXTENDED) ? msg->id : 0;
    tx.StdId    = (msg->frame_type == HAL_CAN_FRAME_STANDARD) ? msg->id : 0;
    tx.IDE      = (msg->frame_type == HAL_CAN_FRAME_EXTENDED) ? CAN_ID_EXT : CAN_ID_STD;
    tx.DLC      = msg->dlc;
    tx.RTR      = CAN_RTR_DATA;

    uint32_t mailbox;
    return (HAL_CAN_AddTxMessage(h, &tx, (uint8_t*)msg->data, &mailbox) == HAL_OK) ? 0 : -1;
}

static int can_recv(hal_can_t* can, hal_can_msg_t* msg, uint32_t timeout_ms)
{
    CAN_HandleTypeDef* h = (CAN_HandleTypeDef*)can->_impl;
    if (!h || !msg) return -1;

    CAN_RxHeaderTypeDef rx = {0};
    if (HAL_CAN_GetRxMessage(h, CAN_RX_FIFO0, &rx, msg->data) != HAL_OK)
        return -1;

    msg->id         = (rx.IDE == CAN_ID_EXT) ? rx.ExtId : rx.StdId;
    msg->frame_type = (rx.IDE == CAN_ID_EXT) ? HAL_CAN_FRAME_EXTENDED : HAL_CAN_FRAME_STANDARD;
    msg->dlc        = rx.DLC;
    return 0;
}

static int can_set_filter(hal_can_t* can, const hal_can_filter_t* filter, int count)
{
    CAN_HandleTypeDef* h = (CAN_HandleTypeDef*)can->_impl;
    if (!h || !filter || count < 1) return -1;

    CAN_FilterTypeDef f = {0};
    f.FilterBank         = 0;
    f.FilterMode         = CAN_FILTERMODE_IDMASK;
    f.FilterScale        = CAN_FILTERSCALE_32BIT;
    f.FilterIdHigh       = (uint16_t)(filter->id << 5);
    f.FilterIdLow        = 0;
    f.FilterMaskIdHigh   = (uint16_t)(filter->mask << 5);
    f.FilterMaskIdLow    = 0;
    f.FilterFIFOAssignment = CAN_RX_FIFO0;
    f.FilterActivation   = ENABLE;
    return (HAL_CAN_ConfigFilter(h, &f) == HAL_OK) ? 0 : -1;
}

static int can_set_rx_callback(hal_can_t* can, hal_can_rx_callback_t cb, void* user_data)
{
    (void)can; (void)cb; (void)user_data;
    return 0;
}

static int can_deinit(hal_can_t* can)
{
    CAN_HandleTypeDef* h = (CAN_HandleTypeDef*)can->_impl;
    if (!h) return -1;
    HAL_CAN_Stop(h);
    HAL_CAN_DeInit(h);
    can->_impl = NULL;
    return 0;
}

void hal_can_init_struct(hal_can_t* can)
{
    can->init            = can_init;
    can->send            = can_send;
    can->recv            = can_recv;
    can->set_filter      = can_set_filter;
    can->set_rx_callback = can_set_rx_callback;
    can->deinit          = can_deinit;
    can->_impl           = NULL;
}

void hal_can_force_stop(void)
{
    for (int i = 0; i < CAN_MAX_COUNT; i++)
    {
        if (s_can_handles[i])
        {
            HAL_CAN_Stop(s_can_handles[i]);
            HAL_CAN_DeInit(s_can_handles[i]);
        }
    }
}

static int stm32_can_probe(device_t* dev)
{
    int index = 0;
    if (device_get_prop_int(dev, "can-index", &index) != 0)
        return -1;
    if (index < 0 || index >= CAN_MAX_COUNT || !s_can_handles[index])
        return -1;

    MX_CAN_Init();

    hal_can_t* can = &s_cans[index];
    hal_can_init_struct(can);
    can->_impl = s_can_handles[index];
    device_set_priv(dev, can);
    return 0;
}

static int stm32_can_remove(device_t* dev)
{
    hal_can_t* can = (hal_can_t*)device_get_priv(dev);
    if (can && can->_impl)
    {
        HAL_CAN_DeInit((CAN_HandleTypeDef*)can->_impl);
        can->_impl = NULL;
    }
    device_set_priv(dev, NULL);
    return 0;
}

DRIVER_REGISTER(stm32_can, "st,stm32-can", stm32_can_probe, stm32_can_remove);
