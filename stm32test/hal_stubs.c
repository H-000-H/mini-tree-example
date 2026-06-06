/* 最小 HAL 弱桩 — 用户应在实际项目中提供真实实现 */
#include "hal_cpu_delay.h"
#include <stddef.h>

#include "device.h"

__attribute__((weak)) uint32_t hal_flash_get_app_addr(void) { return 0x08000000; }
__attribute__((weak)) uint32_t hal_flash_get_app_size(void) { return 0x00100000; }

/* ── hal_platform_safety (弱桩) ── */
__attribute__((weak)) void hal_platform_critical_hardware_lock(void) { }
__attribute__((weak)) void hal_platform_nmi_emergency_stamp(void) { }

/* ── hal_wdt (弱桩) ── */
__attribute__((weak)) _Bool hal_wdt_init_rtc(uint32_t timeout_ms) { (void)timeout_ms; return 0; }
__attribute__((weak)) void   hal_wdt_feed_rtc(void) { }
__attribute__((weak)) void   hal_wdt_rtc_set_long_timeout(void) { }
__attribute__((weak)) void   hal_wdt_rtc_restore_timeout(void) { }
__attribute__((weak)) _Bool  hal_wdt_init_twdt(uint32_t timeout_ms) { (void)timeout_ms; return 0; }
__attribute__((weak)) _Bool  hal_wdt_subscribe(void* task_handle) { (void)task_handle; return 0; }
__attribute__((weak)) _Bool  hal_wdt_unsubscribe(void* task_handle) { (void)task_handle; return 0; }
__attribute__((weak)) void   hal_wdt_feed_twdt(void) { }

/* ── board_cascade_get (裸机无 cascade 关系时的弱桩) ── */
__attribute__((weak)) const void* board_cascade_get(device_id_t id, int* count) { (void)id; *count = 0; return 0; }

/* ── FreeRTOS 钩子 (弱桩) ── */
__attribute__((weak)) void vApplicationStackOverflowHook(void* xTask, char* pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    /* 关全局中断后死锁 — 防止 ISR 继续尝试调度已损坏的任务 */
    __asm volatile("cpsid i" ::: "memory");
    for (;;);
}
