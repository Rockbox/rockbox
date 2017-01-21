/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr
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
#include "system.h"

#ifndef BOOTLOADER
#include "adc-target.h"
#include "button-target.h"

#if defined(HAVE_ADJUSTABLE_CPU_FREQ) && (NUM_CORES > 1)
#include "corelock.h"
static struct corelock cpufreq_cl SHAREDBSS_ATTR;
#endif

extern void TIMER1(void);
extern void TIMER2(void);

void __attribute__((interrupt("IRQ"))) irq_handler(void)
{
    if(CURRENT_CORE == CPU)
    {
        if (CPU_INT_STAT & TIMER1_MASK)
            TIMER1();
        else if (CPU_INT_STAT & TIMER2_MASK)
            TIMER2();
        else if (CPU_INT_STAT & GPIO_MASK)
        {
            if (GPIOA_INT_STAT)
                ipod_3g_button_int();
#ifdef IPOD_1G2G
            if (GPIOB_INT_STAT & 0x04)
                ipod_2g_adc_int();
#endif
        }
    } 
    else
    {
        if (COP_INT_STAT & TIMER2_MASK)
            TIMER2();
    }
}

#endif

#ifndef BOOTLOADER
void ICODE_ATTR __attribute__((naked)) commit_dcache(void)
{
    asm volatile(
        "mov    r0, #0xf0000000 \n"
        "add    r0, r0, #0xc000 \n" /* r0 = CACHE_FLUSH_BASE */
        "add    r1, r0, #0x2000 \n" /* r1 = CACHE_FLUSH_BASE + CACHE_SIZE */
        "mov    r2, #0          \n"
    "1:                         \n"
        "str    r2, [r0], #16   \n" /* Commit */
        "cmp    r0, r1          \n"
        "blo    1b              \n"
        "bx     lr              \n"
    );
}

void ICODE_ATTR  __attribute__((naked)) commit_discard_idcache(void)
{
    asm volatile(
        "mov    r0, #0xf0000000 \n"
        "add    r2, r0, #0x4000 \n" /* r1 = CACHE_INVALIDATE_BASE */
        "add    r0, r0, #0xc000 \n" /* r0 = CACHE_FLUSH_BASE */
        "add    r1, r0, #0x2000 \n" /* r2 = CACHE_FLUSH_BASE + CACHE_SIZE */
        "mov    r3, #0          \n"
    "1:                         \n"
        "str    r3, [r0], #16   \n" /* Commit */
        "str    r3, [r2], #16   \n" /* Discard */
        "cmp    r0, r1          \n"
        "blo    1b              \n"
        "bx     lr              \n"
    );
}

void commit_discard_dcache(void) __attribute__((alias("commit_discard_idcache")));

static void ipod_init_cache(void)
{
/* Initialising the cache in the iPod bootloader prevents Rockbox from starting */
    PROC_STAT &= ~0x700;
    outl(0x4000, 0xcf004020);

    CACHE_CTL = CACHE_CTL_INIT;

    asm volatile(
        "mov    r0, #0xf0000000 \n"
        "add    r0, r0, #0x4000 \n" /* r0 = CACHE_INVALIDATE_BASE */
        "add    r1, r0, #0x2000 \n" /* r1 = CACHE_INVALIDATE_BASE + CACHE_SIZE */
        "mov    r2, #0          \n"
    "1:                         \n"
        "str    r2, [r0], #16   \n" /* Invalidate */
        "cmp    r0, r1          \n"
        "blo    1b              \n"
        : : : "r0", "r1", "r2"
    );

    /* Cache if (addr & mask) >> 16 == (mask & match) >> 16:
     * yes: 0x00000000 - 0x03ffffff
     *  no: 0x04000000 - 0x1fffffff
     * yes: 0x20000000 - 0x23ffffff
     *  no: 0x24000000 - 0x3fffffff <= range containing uncached alias
     */
    CACHE_MASK = 0x00001c00;
    CACHE_OPERATION = 0x3fc0;

    CACHE_CTL = CACHE_CTL_INIT | CACHE_CTL_RUN;
}
    
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
#if NUM_CORES > 1
void set_cpu_frequency__lock(void)
{
    corelock_lock(&cpufreq_cl);
}

void set_cpu_frequency__unlock(void)
{
    corelock_unlock(&cpufreq_cl);
}
#endif /* NUM_CORES > 1 */

void set_cpu_frequency(long frequency)
#else
static void pp_set_cpu_frequency(long frequency)
#endif
{
    cpu_frequency = frequency;

    PLL_CONTROL |= 0x6000;     /* make sure some enable bits are set */
    CLOCK_ENABLE = 0x01;       /* select source #1 */

    switch (frequency)
    {
      case CPUFREQ_MAX:
        PLL_UNLOCK   = 0xd19b; /* unlock frequencies > 66MHz */
        CLOCK_SOURCE = 0xa9;   /* source #1: 24 Mhz, source #2..#4: PLL */
        PLL_CONTROL  = 0xe000; /* PLL enabled */
        PLL_DIV      = 3;      /* 10/3 * 24MHz */
        PLL_MULT     = 10;
        udelay(200);           /* wait for relock */
        break;

      case CPUFREQ_NORMAL:
        CLOCK_SOURCE = 0xa9;   /* source #1: 24 Mhz, source #2..#4: PLL */
        PLL_CONTROL  = 0xe000; /* PLL enabled */
        PLL_DIV      = 4;      /* 5/4 * 24MHz */
        PLL_MULT     = 5;
        udelay(200);           /* wait for relock */
        break;
        
      case CPUFREQ_SLEEP:
        CLOCK_SOURCE = 0x51;   /* source #2: 32kHz, #1, #2, #4: 24MHz */
        PLL_CONTROL  = 0x6000; /* PLL disabled */
        udelay(10000);         /* let 32kHz source stabilize? */
        break;

      default:
        CLOCK_SOURCE = 0x55;   /* source #1..#4: 24 Mhz */
        PLL_CONTROL  = 0x6000; /* PLL disabled */
        cpu_frequency = CPUFREQ_DEFAULT;
        break;
    }
    CLOCK_ENABLE = 0x02;       /* select source #2 */
}
#endif /* !BOOTLOADER */

void system_init(void)
{
#ifndef BOOTLOADER
    if (CURRENT_CORE == CPU)
    {
        /* Remap the flash ROM on CPU, keep hidden from COP:
         * 0x00000000-0x03ffffff = 0x20000000-0x23ffffff */
        MMAP1_LOGICAL  = 0x20003c00;
        MMAP1_PHYSICAL = 0x00003f84;

#if defined(IPOD_1G2G) || defined(IPOD_3G)
        DEV_EN = 0x0b9f; /* don't clock unused PP5002 hardware components */
        outl(0x0035, 0xcf005004);  /* DEV_EN2 ? */
#endif

        INT_FORCED_CLR = -1;
        CPU_INT_DIS    = -1;
        COP_INT_DIS    = -1;

        GPIOA_INT_EN   = 0;
        GPIOB_INT_EN   = 0;
        GPIOC_INT_EN   = 0;
        GPIOD_INT_EN   = 0;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
#if NUM_CORES > 1
        corelock_init(&cpufreq_cl);
#endif
#else
        pp_set_cpu_frequency(CPUFREQ_MAX);
#endif
    }
    ipod_init_cache();
#endif
}

void system_reboot(void)
{
    DEV_RS |= 4;
    while (1);
}

void system_exception_wait(void)
{
    /* FIXME: we just need the right buttons */
    CPU_INT_DIS = -1;
    COP_INT_DIS = -1;

    /* Halt */
    sleep_core(CURRENT_CORE);
    while (1);
}

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}
