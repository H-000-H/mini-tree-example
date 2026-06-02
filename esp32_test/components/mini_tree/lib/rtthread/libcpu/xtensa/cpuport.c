/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Xtensa LX7 (ESP32-S3) CPU port for RT-Thread
 */
#include <rthw.h>
#include <rtthread.h>
#include <rtatomic.h>

#include "cpuport.h"
#include "rt_hw_stack_frame.h"

#ifndef RT_USING_SMP
volatile rt_ubase_t  rt_interrupt_from_thread = 0;
volatile rt_ubase_t  rt_interrupt_to_thread   = 0;
volatile rt_uint32_t rt_thread_switch_interrupt_flag = 0;
#endif

/*
 * Initialize thread stack for cooperative context switching.
 *
 * Creates a stack region where rt_hw_context_switch_to's retw
 * will "return" to the thread entry function.
 *
 * Xtensa retw reads the return save area from [SP+0] and [SP+4],
 * then rotates the window back. For call8, register values in the
 * caller's window (a2/a3) come from the callee's a10/a11.
 *
 * Stack layout:
 *   [SP + 0]  = thread entry PC  -> retw jumps here
 *   [SP + 4]  = thread SP (stk)   -> retw restores this as a1
 *   [SP + 8]  = param             -> becomes a2 after window rotation
 *   [SP + 12] = texit             -> becomes a3 after window rotation
 */
rt_uint8_t *rt_hw_stack_init(void       *tentry,
                             void       *parameter,
                             rt_uint8_t *stack_addr,
                             void       *texit)
{
    rt_uint8_t *stk;
    rt_uint32_t *sa;

    stk = (rt_uint8_t *)RT_ALIGN_DOWN((rt_ubase_t)stack_addr, 16);
    stk -= 0x40;  /* 64 bytes: save area + margin */

    sa = (rt_uint32_t *)(stk);
    sa[0] = (rt_uint32_t)tentry;    /* [SP+0]: return PC = thread entry */
    sa[1] = (rt_uint32_t)stk;       /* [SP+4]: caller's SP */
    sa[2] = (rt_uint32_t)parameter; /* [SP+8]: param -> a2 in thread */
    sa[3] = (rt_uint32_t)texit;     /* [SP+12]: texit -> a3 in thread */

    return stk;
}

/* ── Atomic operations for Xtensa LX7 ──
 * Uses GCC __atomic builtins for portable atomic access.
 * Xtensa S32C1I provides hardware CAS for atomic add/exchange.
 */

rt_atomic_t rt_hw_atomic_load(volatile rt_atomic_t *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

void rt_hw_atomic_store(volatile rt_atomic_t *ptr, rt_atomic_t val)
{
    __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST);
}

rt_atomic_t rt_hw_atomic_add(volatile rt_atomic_t *ptr, rt_atomic_t val)
{
    return __atomic_fetch_add(ptr, val, __ATOMIC_SEQ_CST);
}

rt_atomic_t rt_hw_atomic_sub(volatile rt_atomic_t *ptr, rt_atomic_t val)
{
    return __atomic_fetch_sub(ptr, val, __ATOMIC_SEQ_CST);
}

rt_atomic_t rt_hw_atomic_and(volatile rt_atomic_t *ptr, rt_atomic_t val)
{
    return __atomic_fetch_and(ptr, val, __ATOMIC_SEQ_CST);
}

rt_atomic_t rt_hw_atomic_or(volatile rt_atomic_t *ptr, rt_atomic_t val)
{
    return __atomic_fetch_or(ptr, val, __ATOMIC_SEQ_CST);
}

rt_atomic_t rt_hw_atomic_xor(volatile rt_atomic_t *ptr, rt_atomic_t val)
{
    return __atomic_fetch_xor(ptr, val, __ATOMIC_SEQ_CST);
}

rt_atomic_t rt_hw_atomic_exchange(volatile rt_atomic_t *ptr, rt_atomic_t val)
{
    return __atomic_exchange_n(ptr, val, __ATOMIC_SEQ_CST);
}

rt_atomic_t rt_hw_atomic_compare_exchange_strong(volatile rt_atomic_t *ptr,
                                                  rt_atomic_t *expected,
                                                  rt_atomic_t desired)
{
    return __atomic_compare_exchange_n(ptr, expected, desired, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

void rt_hw_atomic_flag_clear(volatile rt_atomic_t *ptr)
{
    __atomic_clear(ptr, __ATOMIC_SEQ_CST);
}

rt_atomic_t rt_hw_atomic_flag_test_and_set(volatile rt_atomic_t *ptr)
{
    return __atomic_test_and_set(ptr, __ATOMIC_SEQ_CST);
}

/* 8-bit and 16-bit variants */
rt_atomic8_t rt_hw_atomic_load8(volatile rt_atomic8_t *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

void rt_hw_atomic_store8(volatile rt_atomic8_t *ptr, rt_atomic8_t val)
{
    __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST);
}

rt_atomic16_t rt_hw_atomic_load16(volatile rt_atomic16_t *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

void rt_hw_atomic_store16(volatile rt_atomic16_t *ptr, rt_atomic16_t val)
{
    __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST);
}

rt_atomic8_t rt_hw_atomic_and8(volatile rt_atomic8_t *ptr, rt_atomic8_t val)
{
    return __atomic_fetch_and(ptr, val, __ATOMIC_SEQ_CST);
}

rt_atomic8_t rt_hw_atomic_or8(volatile rt_atomic8_t *ptr, rt_atomic8_t val)
{
    return __atomic_fetch_or(ptr, val, __ATOMIC_SEQ_CST);
}

rt_atomic16_t rt_hw_atomic_and16(volatile rt_atomic16_t *ptr, rt_atomic16_t val)
{
    return __atomic_fetch_and(ptr, val, __ATOMIC_SEQ_CST);
}

rt_atomic16_t rt_hw_atomic_or16(volatile rt_atomic16_t *ptr, rt_atomic16_t val)
{
    return __atomic_fetch_or(ptr, val, __ATOMIC_SEQ_CST);
}

/* ── __rt_ffs / __rt_ffsl ──
 * Find first set bit (1-indexed, like ffs).
 * Returns 0 for input 0.
 */
int __rt_ffs(int value)
{
    return __builtin_ffs(value);
}

unsigned long __rt_ffsl(unsigned long value)
{
    return (unsigned long)__builtin_ffsl((long)value);
}

rt_weak void rt_hw_do_after_save_above(void)
{
    while (1);
}

#ifndef RT_USING_SMP
rt_weak void rt_hw_context_switch_interrupt(rt_ubase_t from, rt_ubase_t to,
                                            rt_thread_t from_thread,
                                            rt_thread_t to_thread)
{
    (void)from_thread;
    (void)to_thread;

    if (rt_thread_switch_interrupt_flag == 0)
        rt_interrupt_from_thread = from;

    rt_interrupt_to_thread = to;
    rt_thread_switch_interrupt_flag = 1;

    /* On Xtensa: fallback to direct switch via inline call */
    if (from && to) {
        rt_hw_context_switch(from, to);
    }
}
#endif /* end of RT_USING_SMP */
