/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Xtensa LX7 (ESP32-S3) context switching for mini-tree RT-Thread.
 *
 * Minimal cooperative context switch using windowed ABI.
 * Context switch by saving/restoring SP and relying on windowed
 * return (retw) to redirect execution to the next thread.
 *
 * Stack layout after entry sp, N (for call8):
 *   [SP + 0..15] = local frame
 *   [SP + 16]    = return PC (a0, saved by entry)
 *   [SP + 20]    = caller's SP (a1, saved by entry)
 *   [SP + 24]    = caller's a2
 *   [SP + 28]    = caller's a3
 *
 * Thread initial stack (set up by rt_hw_stack_init):
 *   [SP + 0]     = thread entry PC  → retw jumps here
 *   [SP + 4]     = thread SP value
 *   [SP + 8]     = thread param (a2)
 *   [SP + 12]    = thread exit func (a3)
 */
    .text
    .align  4

/*
 * rt_base_t rt_hw_interrupt_disable(void);
 * Set INTLEVEL to 15 (disable all maskable interrupts).
 * Returns previous PS.
 */
    .global rt_hw_interrupt_disable
    .type   rt_hw_interrupt_disable,@function
    .align  4
rt_hw_interrupt_disable:
    entry   sp, 16
    rsil    a2, 15
    retw.n

/*
 * void rt_hw_interrupt_enable(rt_base_t level);
 * Restore PS register.
 */
    .global rt_hw_interrupt_enable
    .type   rt_hw_interrupt_enable,@function
    .align  4
rt_hw_interrupt_enable:
    entry   sp, 16
    wsr     a2, PS
    rsync
    retw.n

/*
 * void rt_hw_context_switch_to(rt_ubase_t to);
 * a2 --> to (pointer to thread's SP save word)
 *
 * Start the first thread by loading its saved SP and
 * returning to the thread's entry point. The thread's
 * stack was set up by rt_hw_stack_init so that retw
 * "returns" to the thread entry function.
 */
    .global rt_hw_context_switch_to
    .type   rt_hw_context_switch_to,@function
    .align  4
rt_hw_context_switch_to:
    entry   sp, 16
    /* a2 = to (passed from C caller) */
    /* Load thread's saved SP */
    l32i    a1, a2, 0

    /* Set a10=param, a11=exit so that after window rotation
     * a2=param, a3=exit in the caller's window */
    l32i    a10, a1, 8       /* param at [SP + 8]  → a2 in thread */
    l32i    a11, a1, 12      /* texit at [SP + 12] → a3 in thread */

    /* retw: restores a0 from [SP+16] (thread entry) and
     * a1 from [SP+20] (thread SP). Window rotates back,
     * caller's a2 = our a10 = param, caller's a3 = our a11 = exit.
     * So entry function is called as: entry(param) */
    retw

/*
 * void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to);
 * a2 --> from (pointer to current thread's SP save word)
 * a3 --> to   (pointer to next thread's SP save word)
 *
 * Save current SP to *from, load SP from *to, and
 * return to the next thread's context.
 *
 * When retw executes with the new SP, it reads the
 * return address from the new thread's save area,
 * which "returns" execution to the new thread.
 */
    .global rt_hw_context_switch
    .type   rt_hw_context_switch,@function
    .align  4
rt_hw_context_switch:
    entry   sp, 16

    /* Save current SP to *from */
    s32i    a1, a2, 0

    /* Load SP from *to */
    l32i    a1, a3, 0

    /* retw: using new SP, reads [SP+16] as return PC
     * and [SP+20] as caller's SP. Window rotation makes
     * the caller's a2 = our a10, caller's a3 = our a11.
     * For subsequent switches, scheduler's return chain
     * is automatically preserved. */
    retw

/*
 * void rt_hw_context_switch_interrupt(rt_ubase_t from, rt_ubase_t to,
 *                                      rt_thread_t from_thread,
 *                                      rt_thread_t to_thread);
 *
 * Called from interrupt context to request a context switch.
 * Sets the switch flag and triggers context switch on interrupt exit.
 * For the initial cooperative port, simply does direct switch.
 */
    .global rt_hw_context_switch_interrupt
    .type   rt_hw_context_switch_interrupt,@function
    .align  4
rt_hw_context_switch_interrupt:
    entry   sp, 16
    /* Direct switch: just like rt_hw_context_switch */
    s32i    a1, a2, 0
    l32i    a1, a3, 0
    retw
