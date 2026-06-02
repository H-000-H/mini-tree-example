/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Xtensa LX7 (ESP32-S3) CPU port for mini-tree RT-Thread
 */
#ifndef CPUPORT_H__
#define CPUPORT_H__

#include <rtconfig.h>

#ifndef __ASSEMBLY__
#ifdef RT_USING_SMP
typedef union {
    unsigned long slock;
    struct __arch_tickets {
        unsigned short owner;
        unsigned short next;
    } tickets;
} rt_hw_spinlock_t;
#endif
#endif

/* bytes of register width */
#define STORE    s32i
#define LOAD     l32i
#define REGBYTES 4

#endif /* CPUPORT_H__ */
