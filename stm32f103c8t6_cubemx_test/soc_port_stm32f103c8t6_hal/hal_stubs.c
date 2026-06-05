#include "hal_platform_safety.h"
#include "hal_flash.h"
#include "hal_wdt.h"
#include "board_devtable.h"
#include <stdint.h>
#include <stddef.h>

/* ═══════════════════════════════════════════════════════════════════
 *  安全硬件控制 (hal_platform_safety.h)
 *  由 safe_state.c 在致命故障时调用
 * ═══════════════════════════════════════════════════════════════════ */

void hal_platform_critical_hardware_lock(void)
{
    /*
     * TODO: 强制停止所有安全关键硬件。
     * STM32F103 示例:
     *   GPIOA->BRR = GPIO_PIN_All;  // 所有 GPIO 输出置低
     *   TIM1->BDTR &= ~TIM_BDTR_MOE; // 关闭 TIM1 PWM 输出
     */
}

void hal_platform_nmi_emergency_stamp(void)
{
    /*
     * TODO: 将崩溃标记写入后备寄存器。
     * STM32F103 示例:
     *   RTC->BKP0R = 0xBADF00D;
     */
}

/* ═══════════════════════════════════════════════════════════════════
 *  Flash 信息 (hal_flash.h)
 *  由 system_scrubber 调用，用于 CRC 校验固件
 * ═══════════════════════════════════════════════════════════════════ */

uint32_t hal_flash_get_app_addr(void)
{
    return 0x08000000;  /* STM32F103C8: Flash 起始地址 */
}

uint32_t hal_flash_get_app_size(void)
{
    return 64 * 1024;   /* STM32F103C8T6: 64 KB */
}

/* ═══════════════════════════════════════════════════════════════════
 *  看门狗 (hal_wdt.h)
 *  由 system_wdt 调用。当前未实现硬件初始化，返回 false 禁用
 * ═══════════════════════════════════════════════════════════════════ */

/* ── RTC / 独立看门狗 ── */

bool hal_wdt_init_rtc(uint32_t timeout_ms)
{
    (void)timeout_ms;
    return false;  /* 暂不启用独立看门狗 */
}

void hal_wdt_feed_rtc(void)
{
}

void hal_wdt_rtc_set_long_timeout(void)
{
}

void hal_wdt_rtc_restore_timeout(void)
{
}

/* ── 任务看门狗 TWDT ── */

bool hal_wdt_init_twdt(uint32_t timeout_ms)
{
    (void)timeout_ms;
    return false;  /* 暂不启用任务看门狗 */
}

bool hal_wdt_subscribe(void* task_handle)
{
    (void)task_handle;
    return false;
}

bool hal_wdt_unsubscribe(void* task_handle)
{
    (void)task_handle;
    return false;
}

void hal_wdt_feed_twdt(void)
{
}

/* ═══════════════════════════════════════════════════════════════════
 *  故障传播表 (board_devtable.h)
 *  当前 DTS 无 cascade 依赖，返回空
 * ═══════════════════════════════════════════════════════════════════ */

const device_id_t* board_cascade_get(device_id_t id, int* count)
{
    (void)id;
    if (count) *count = 0;
    return NULL;
}
