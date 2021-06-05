/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#ifndef __SYSTEM_TARGET_H__
#define __SYSTEM_TARGET_H__

#ifdef DEBUG
/* Define this to get CPU idle stats, visible in the debug menu. */
# define X1000_CPUIDLE_STATS
#endif

#include "cpu.h"
#include "mmu-mips.h"
#include "mipsregs.h"
#include "mipsr2-endian.h"
#include <stdint.h>

/* Rockbox API */
#define enable_irq()        set_c0_status(ST0_IE)
#define disable_irq()       clear_c0_status(ST0_IE)
#define disable_irq_save()  set_irq_level(0)
#define restore_irq(arg)    write_c0_status(arg)
#define HIGHEST_IRQ_LEVEL   0

static inline int set_irq_level(int lev)
{
    unsigned reg, oldreg;
    reg = oldreg = read_c0_status();
    if(lev)
        reg |= ST0_IE;
    else
        reg &= ~ST0_IE;

    write_c0_status(reg);
    return oldreg;
}

#ifdef X1000_CPUIDLE_STATS
/* CPU idle stats, updated each kernel tick in kernel-x1000.c */
extern int __cpu_idle_avg;
extern int __cpu_idle_cur;
extern uint32_t __cpu_idle_ticks;
extern uint32_t __cpu_idle_reftick;
#endif

static inline uint32_t __ost_read32(void);
static inline void core_sleep(void)
{
#ifdef X1000_CPUIDLE_STATS
    uint32_t t1 = __ost_read32();
#endif

    __asm__ __volatile__(
        ".set push\n\t"
        ".set mips32r2\n\t"
        "mfc0 $8, $12\n\t"
        "move $9, $8\n\t"
        "la   $10, 0x8000000\n\t"
        "or   $8, $10\n\t"
        "mtc0 $8, $12\n\t"
        "wait\n\t"
        "mtc0 $9, $12\n\t"
        ".set pop\n\t"
        ::: "t0", "t1", "t2");

#ifdef X1000_CPUIDLE_STATS
    uint32_t t2 = __ost_read32();
    __cpu_idle_ticks += t2 - t1;
#endif

    enable_irq();
}

/* IRQ control */
typedef void(*irq_handler_t)(void);

extern irq_handler_t system_set_irq_handler(int irq, irq_handler_t handler);
extern void system_enable_irq(int irq);
extern void system_disable_irq(int irq);

/* Simple delay API */
#define OST_FREQUENCY       (X1000_EXCLK_FREQ / 4)
#define OST_TICKS_PER_US    (OST_FREQUENCY / 1000000)
#define MAX_OST_DELAY_ARG   0x7fffffff
#define MAX_UDELAY_ARG      (MAX_OST_DELAY_ARG / OST_TICKS_PER_US)
#define MAX_MDELAY_ARG      (MAX_UDELAY_ARG / 1000)

/* Macros adapted from include/linux/delay.h,
 *   Copyright (C) 1993 Linus Torvalds
 *
 * These optimize away all calculations to compile time for the common case
 * of small constant arguments, reducing to a single __ost_delay() call.
 */

#define udelay(n) \
    ((__builtin_constant_p(n) && (n) <= MAX_UDELAY_ARG) ? \
      __ost_delay((n) * OST_TICKS_PER_US) : __udelay((n)))

#define mdelay(n) \
    ((__builtin_constant_p(n) && (n) <= MAX_MDELAY_ARG) ? \
      __ost_delay((n) * 1000 * OST_TICKS_PER_US) : __mdelay((n)))

/* Slow path implementations which handle their full argument range by
 * looping and calling __ost_delay() repeatedly.
 */
extern void __udelay(uint32_t us);
extern void __mdelay(uint32_t ms);

/* Read full 64-bit OST counter value; this requires disabling IRQs
 * to safely read the counter.
 */
extern uint64_t __ost_read64(void);

static inline uint32_t __ost_read32(void)
{
    /* Read OST_2CNTL using raw address to avoid exposing internal headers.
     * The 64-bit counter is read with IRQs disabled and since threads are
     * not pre-emptive in Rockbox we won't trash anybody's 64-bit read by
     * reading the low count without locking.
     */
    return *(const volatile uint32_t*)0xb2000020;
}

/* NOTE: it is required that count < MAX_OST_DELAY_ARG, this is to provide
 * some slack in the 32-bit counter so we can reliably detect the timeout.
 */
static inline void __ost_delay(uint32_t count)
{
    /* Add one to ensure we delay for at least the time given */
    count += 1;
    uint32_t start = __ost_read32();
    while(__ost_read32() - start < count);
}

#endif /* __SYSTEM_TARGET_H__ */
