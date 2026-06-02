/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Xtensa LX7 (ESP32-S3) stack frame definition
 */
#ifndef XTENSA_STACKFRAME_H
#define XTENSA_STACKFRAME_H

#include "cpuport.h"

/*
 * Stack frame layout for Xtensa context switch.
 * This matches the Xtensa exception stack frame format used
 * by ESP-IDF's xtensa_context.S (_xt_context_save/_xt_context_restore).
 *
 * Offsets must match XT_STK_* defines in xtensa_context.h
 */
struct rt_hw_stack_frame
{
    rt_ubase_t exit;        /* 0x00: exit point for dispatch */
    rt_ubase_t pc;          /* 0x04: return PC */
    rt_ubase_t ps;          /* 0x08: return PS */
    rt_ubase_t a0;          /* 0x0C */
    rt_ubase_t a1;          /* 0x10: stack pointer before interrupt */
    rt_ubase_t a2;          /* 0x14 */
    rt_ubase_t a3;          /* 0x18 */
    rt_ubase_t a4;          /* 0x1C */
    rt_ubase_t a5;          /* 0x20 */
    rt_ubase_t a6;          /* 0x24 */
    rt_ubase_t a7;          /* 0x28 */
    rt_ubase_t a8;          /* 0x2C */
    rt_ubase_t a9;          /* 0x30 */
    rt_ubase_t a10;         /* 0x34 */
    rt_ubase_t a11;         /* 0x38 */
    rt_ubase_t a12;         /* 0x3C */
    rt_ubase_t a13;         /* 0x40 */
    rt_ubase_t a14;         /* 0x44 */
    rt_ubase_t a15;         /* 0x48 */
    rt_ubase_t sar;         /* 0x4C */
    rt_ubase_t exccause;    /* 0x50 */
    rt_ubase_t excvaddr;    /* 0x54 */
/* size = 0x58 (88 bytes) */
};

#endif /* XTENSA_STACKFRAME_H */
