/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Greg White
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef SYSTEM_TARGET_H
#define SYSTEM_TARGET_H

#include "system-arm.h"
#include "mmu-arm.h"

#ifndef HAVE_ADJUSTABLE_CPU_FREQ
/* TODO: implement CPU frequency scaling */
#define CPUFREQ_DEFAULT CPU_FREQ
#define CPUFREQ_NORMAL CPU_FREQ
#define CPUFREQ_MAX CPU_FREQ
#endif

#if 0
static inline void udelay(unsigned int usecs)
{
    volatile signed int stop = EPITCNT1 - usecs;
    while ((signed int)EPITCNT1 > stop);
}
#endif

void watchdog_init(unsigned int half_seconds);
void watchdog_service(void);

/* Prepare for transition to firmware */
void system_prepare_fw_start(void);
void tick_stop(void);
void kernel_device_init(void);

void imx31_regmod32(volatile uint32_t *reg_p, uint32_t value,
                    uint32_t mask);
void imx31_regset32(volatile uint32_t *reg_p, uint32_t mask);
void imx31_regclr32(volatile uint32_t *reg_p, uint32_t mask);

#define KDEV_INIT

#define HAVE_INVALIDATE_ICACHE
static inline void invalidate_icache(void)
{
    asm volatile(
        /* Clean and invalidate entire data cache */
        "mcr p15, 0, %0, c7, c14, 0 \n"
        /* Invalidate entire instruction cache
         * Also flushes the branch target cache */
        "mcr p15, 0, %0, c7, c5, 0  \n"
        /* Data synchronization barrier */
        "mcr p15, 0, %0, c7, c10, 4 \n"
        /* Flush prefetch buffer */
        "mcr p15, 0, %0, c7, c5, 4  \n"
        : : "r"(0)
    );
}

#define HAVE_FLUSH_ICACHE
static inline void flush_icache(void)
{
    asm volatile (
        /* Clean entire data cache */
        "mcr p15, 0, %0, c7, c10, 0 \n"
        /* Data synchronization barrier */
        "mcr p15, 0, %0, c7, c10, 4 \n"
        : : "r"(0)
    );
}

struct ARM_REGS {
	int r0;
	int r1;
	int r2;
	int r3;
	int r4;
	int r5;
	int r6;
	int r7;
	int r8;
	int r9;
	int r10;
	int r11;
	int r12;
	int sp;
	int lr;
	int pc;
	int cpsr;
} regs;

inline void dumpregs(void);

#endif /* SYSTEM_TARGET_H */
