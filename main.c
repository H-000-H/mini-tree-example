/* 用户工程：STM32F407ZGT6 — OSAL 后端切换测试 */
#include "hal_cpu_delay.h"
#include "system_init.h"
#include "osal.h"
#include <stdint.h>

/* ── RCC 寄存器 ── */
#define RCC_CR             (*(volatile uint32_t*)0x40023800U)
#define RCC_PLLCFGR        (*(volatile uint32_t*)0x40023804U)
#define RCC_CFGR           (*(volatile uint32_t*)0x40023808U)
#define RCC_AHB1ENR        (*(volatile uint32_t*)0x40023830U)
#define RCC_AHB1ENR_GPIOC_EN  (1U << 2)
/* ── FLASH 控制 ── */
#define FLASH_ACR          (*(volatile uint32_t*)0x40023C00U)
/* ── GPIO ── */
#define GPIOC_MODER        (*(volatile uint32_t*)0x40020800U)
#define GPIOC_BSRR         (*(volatile uint32_t*)0x40020818U)
#define LED_PIN            13U
#define LED_BIT            (1U << LED_PIN)

static void system_clock_init(void)
{
    /* 1. 设置 FLASH 等待周期 (168 MHz 需 5 WS) */
    FLASH_ACR = (FLASH_ACR & ~7U) | 5U;

    /* 2. 切回 HSI 并关 PLL (RM0090: PLL 必须先关才能改参数) */
    RCC_CFGR &= ~(3U << 0);               /* SW=00 → HSI */
    while ((RCC_CFGR & (3U << 2)) != (0U << 2));

    RCC_CR &= ~(1U << 24);                /* PLLON=0 */
    while (RCC_CR & (1U << 25));          /* 等 PLL 完全关闭 */

    /* 3. 配置 PLL: HSI(16MHz) → /16 → *336 → /2 → SYSCLK=168MHz */
    RCC_PLLCFGR = (7U << 24) |            /* PLLQ=7  → 336/7=48MHz(USB) */
                  (0U << 22) |            /* PLLSRC=0 → HSI */
                  (336U << 6) |           /* PLLN=336 */
                  (0U << 16) |            /* PLLP=0  → /2 = 168MHz SYSCLK */
                  (16U << 0);             /* PLLM=16  → 16MHz/16=1MHz */

    /* 4. 使能 PLL */
    RCC_CR |= (1U << 24);
    while (!(RCC_CR & (1U << 25)));

    /* 5. 切换 SYSCLK=PLL */
    RCC_CFGR = (RCC_CFGR & ~(3U << 0)) | (2U << 0);  /* SW=10 → PLL */
    while ((RCC_CFGR & (3U << 2)) != (2U << 2));

    /* 6. 设置总线预分频 (SYSCLK 稳定后再切, 避免总线时钟超限) */
    RCC_CFGR |= (5U << 8)  |              /* PPRE1=101 → APB1 /4 */
                (4U << 11);               /* PPRE2=100 → APB2 /2 */
}

void platform_init(void)
{
    system_clock_init();
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOC_EN;
    GPIOC_MODER &= ~(3U << (LED_PIN * 2));
    GPIOC_MODER |=  (1U << (LED_PIN * 2));
}

void platform_register_drivers(void) { }

/* ── LED 翻转任务 (OSAL_NULL 时作为普通函数, RTOS 时作为任务入口) ── */
static void blink_task(void* param)
{
    (void)param;
    while (1) {
        GPIOC_BSRR = LED_BIT;
        osal_delay_ms(200);
        GPIOC_BSRR = (LED_BIT << 16U);
        osal_delay_ms(200);
    }
}

static void led_on(void)  { GPIOC_BSRR = (LED_BIT << 16U); } /* PC13 LOW */
static void led_off(void) { GPIOC_BSRR = LED_BIT; }          /* PC13 HIGH */

int main(void)
{
    platform_init();
    hal_delay_init();

#ifdef CONFIG_OSAL_RTTHREAD
    /* Step 1: 亮 1 秒 → 确认 platform_init 正常 */
    led_on();   hal_delay_ms(1000); led_off(); hal_delay_ms(500);

    /* Step 2: RT-Thread 内核基础初始化 */
    extern void rt_system_scheduler_init(void);
    extern void rt_system_timer_init(void);
    rt_system_scheduler_init();
    rt_system_timer_init();

    /* Step 3: 创建任务 */
    osal_task_create("led", 512, 1, blink_task, NULL, 0);
    led_on();   hal_delay_ms(200);  led_off(); hal_delay_ms(500);

    /* Step 4: SysTick 配置 (优先级已在 Reset_Handler 设为 0xFF, 不抢占 PendSV) */
    *(volatile uint32_t*)0xE000E014 = 168000U - 1;
    *(volatile uint32_t*)0xE000E018 = 0;
    *(volatile uint32_t*)0xE000E010 = 0x07;
    led_on();   hal_delay_ms(200);  led_off(); hal_delay_ms(500);

    /* Step 5: 创建空闲线程 + 启动调度器 */
    extern void rt_thread_idle_init(void);
    extern void rt_system_scheduler_start(void);
    rt_thread_idle_init();
    led_on();
    rt_system_scheduler_start();
    while (1) { __asm volatile("wfi"); }
#else
    mini_tree_pre_os_init();
    mini_tree_start_tasks();

#ifdef CONFIG_OSAL_NULL
    (void)blink_task;
    while (1) {
        led_on();
        hal_delay_ms(1000);
        led_off();
        hal_delay_ms(1000);
    }
#else
    osal_task_create("led", 512, 1, blink_task, NULL, 0);
    extern void vTaskStartScheduler(void);
    system_init_complete();
    vTaskStartScheduler();
    while (1) { __asm volatile("wfi"); }
#endif
#endif
}
