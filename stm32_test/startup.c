#include <stdint.h>

extern int main(void);

extern uint32_t _estack;
extern uint32_t _sdata, _edata, _sbss, _ebss, _sidata;

/* ── 默认中断处理 (weak, 可被 RTOS 覆盖) ── */
__attribute__((weak)) void Default_Handler(void) { while (1); }

/* ── RTOS 后端异常处理函数声明 ── */
#ifdef CONFIG_OSAL_FREERTOS
extern void vPortSVCHandler(void);
extern void xPortPendSVHandler(void);
extern void xPortSysTickHandler(void);
#define VEC_SVC     vPortSVCHandler
#define VEC_PENDSV  xPortPendSVHandler
#define VEC_SYSTICK xPortSysTickHandler
#elif defined(CONFIG_OSAL_RTTHREAD)
extern void PendSV_Handler(void);
void SysTick_Handler(void);
#define VEC_SVC     Default_Handler
#define VEC_PENDSV  PendSV_Handler
#define VEC_SYSTICK SysTick_Handler
#else
void SysTick_Handler(void);
#define VEC_SVC     Default_Handler
#define VEC_PENDSV  Default_Handler
#define VEC_SYSTICK SysTick_Handler
#endif

/* ── SysTick 中断处理 (NULL/RTT 后端提供; FreeRTOS 由 port 层提供) ── */
#ifndef CONFIG_OSAL_FREERTOS
void SysTick_Handler(void)
{
#ifdef CONFIG_OSAL_RTTHREAD
    extern void rt_interrupt_enter(void);
    extern void rt_interrupt_leave(void);
    extern void rt_tick_increase(void);
    rt_interrupt_enter();
    rt_tick_increase();
    rt_interrupt_leave();
#else
    extern void osal_null_systick_handler(void);
    osal_null_systick_handler();
#endif
}
#endif

/* ── 标准异常处理函数弱别名 — 全部指向 Default_Handler ── */
#define WEAK_ALIAS(name) \
    __attribute__((weak, alias("Default_Handler"))) void name(void)

WEAK_ALIAS(NMI_Handler);
WEAK_ALIAS(HardFault_Handler);
WEAK_ALIAS(MemManage_Handler);
WEAK_ALIAS(BusFault_Handler);
WEAK_ALIAS(UsageFault_Handler);
WEAK_ALIAS(DebugMon_Handler);

__attribute__((naked)) void Reset_Handler(void)
{
    __asm("ldr sp, =_estack");

    /* 使能 FPU — 必须在任何 C 代码前, 否则 hard-float ABI 下 VFP 指令会触发 NOCP HardFault */
    __asm("ldr r0, =0xE000ED88");     /* SCB->CPACR */
    __asm("ldr r1, [r0]");
    __asm("orr r1, r1, #0x00F00000"); /* CP10 + CP11 full access */
    __asm("str r1, [r0]");
    __asm("dsb");
    __asm("isb");

    /* 设置中断优先级分组: 全部 4bit 用于抢占 (FreeRTOS/RTT 均依赖此设定) */
    __asm("ldr r0, =0xE000ED0C");     /* SCB->AIRCR */
    __asm("ldr r1, [r0]");
    __asm("bic r1, r1, #0x0700");    /* 清 PRIGROUP[10:8] = 0 */
    __asm("ldr r2, =0x05FA0000");    /* VECTKEY — 必须从文字池加载, 无法编码为立即数 */
    __asm("orr r1, r1, r2");
    __asm("str r1, [r0]");
    __asm("dsb");

    /* PendSV + SysTick 设为最低优先级 (0xFF), 防止在 RTOS 初始化前抢占关键代码 */
    __asm("ldr r0, =0xE000ED20");     /* SCB->SHPR3 */
    __asm("ldr r1, =0xFFFF0000");     /* PRI_14(PendSV)=0xFF, PRI_15(SysTick)=0xFF */
    __asm("str r1, [r0]");
    __asm("dsb");

    /* 拷贝 .data 从 Flash → RAM */
    for (uint32_t* src = &_sidata, *dst = &_sdata; dst < &_edata;)
        *dst++ = *src++;

    /* 清零 .bss */
    for (uint32_t* p = &_sbss; p < &_ebss;)
        *p++ = 0;

    main();
    while (1) { __asm volatile("wfi"); }
}

/* ── 中断向量表 ── */
__attribute__((used, section(".isr_vector")))
void (* const g_pfnVectors[])(void) = {
    (void(*)(void))&_estack,
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    (void(*)(void))0,          /* Reserved 7 */
    (void(*)(void))0,          /* Reserved 8 */
    (void(*)(void))0,          /* Reserved 9 */
    (void(*)(void))0,          /* Reserved 10 */
    VEC_SVC,
    DebugMon_Handler,
    (void(*)(void))0,          /* Reserved 13 */
    VEC_PENDSV,
    VEC_SYSTICK,
};
