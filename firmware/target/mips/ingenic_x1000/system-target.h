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

/* For the sake of system.h CACHEALIGN macros.
 * We need this to align DMA buffers, etc.
 */
#define CACHEALIGN_BITS 5
#define HAVE_CPU_CACHE_ALIGN

#define CACHE_SIZE       (16*1024)
#define CACHE_LINE_SIZE  32

#include "mmu-mips.h"
#include "mipsregs.h"

/* Get physical address for DMA */
#define PHYSADDR(addr)  (((unsigned long)(addr)) & 0x1fffffff)

#define NEED_GENERIC_BYTESWAPS

#define HIGHEST_IRQ_LEVEL 0

/* Rockbox API */
#define enable_irq()        set_c0_status(ST0_IE)
#define disable_irq()       clear_c0_status(ST0_IE)
#define disable_irq_save()  set_irq_level(0)
#define restore_irq(arg)    write_c0_status(arg)

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

static inline void core_sleep(void)
{
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
    enable_irq();
}

/* IRQ control */
extern void system_enable_irq(int irq);
extern void system_disable_irq(int irq);

/* Clock rate query */
enum x1000_clk_t {
    X1000_CLK_APLL,
    X1000_CLK_MPLL,
    X1000_CLK_SCLK_A,
    X1000_CLK_CPU,
    X1000_CLK_L2CACHE,
    X1000_CLK_AHB0,
    X1000_CLK_AHB2,
    X1000_CLK_PCLK,
    X1000_CLK_LCD,
    X1000_CLK_MSC0,
    X1000_CLK_MSC1,
    X1000_CLK_COUNT,
};

/* Note to targets: if changing a cached clock, write '1' to the cache
 * entry to mark it invalid; otherwise clk_get() may give wrong results.
 * (It's assumed that no clock can run at 1 Hz.)
 *
 * The preferred way to do this is by calling clk_notify_change().
 */
extern unsigned clk_rate_cache[X1000_CLK_COUNT];
extern unsigned clk_get(enum x1000_clk_t clk);

static inline void clk_notify_change(enum x1000_clk_t clk)
{
    clk_rate_cache[clk] = 1;
}

#define clk_get_apll()    clk_get(X1000_CLK_APLL)
#define clk_get_mpll()    clk_get(X1000_CLK_MPLL)
#define clk_get_sclk_a()  clk_get(X1000_CLK_SCLK_A)
#define clk_get_cpu()     clk_get(X1000_CLK_CPU)
#define clk_get_l2cache() clk_get(X1000_CLK_L2CACHE)
#define clk_get_ahb0()    clk_get(X1000_CLK_AHB0)
#define clk_get_ahb2()    clk_get(X1000_CLK_AHB2)
#define clk_get_pclk()    clk_get(X1000_CLK_PCLK)

/* Simple delay API */
#define OST_CYCLES_PER_US (X1000_EXCLK_FREQ/1000000)

static inline unsigned long long ost_read(void)
{
    unsigned lcnt = *((volatile unsigned*)0xb2000020); /* OST_2CNTL */
    unsigned hcnt = *((volatile unsigned*)0xb2000024); /* OST_2CNTHB */
    return ((unsigned long long)lcnt)|(((unsigned long long)hcnt) << 32);
}

static inline void udelay(unsigned us)
{
    unsigned long long deadline = ost_read();
    deadline += us * OST_CYCLES_PER_US;
    while(ost_read() < deadline);
}

/* WARNING: udelay/mdelay should not be used except for short delays.
 * These functions are only for satisfying hardware timing requirements.
 */
#define mdelay(ms) udelay((ms)*1000)

#endif /* __SYSTEM_TARGET_H__ */
