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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "system.h"

#ifndef BOOTLOADER
extern void TIMER1(void);
extern void TIMER2(void);
extern void ipod_3g_button_int(void);
extern void ipod_2g_adc_int(void);

void irq(void)
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

/* TODO: The following two function have been lifted straight from IPL, and
   hence have a lot of numeric addresses used straight. I'd like to use
   #defines for these, but don't know what most of them are for or even what
   they should be named. Because of this I also have no way of knowing how
   to extend the funtions to do alternate cache configurations and/or
   some other CPU frequency scaling. */

#ifndef BOOTLOADER
void flush_icache(void) ICODE_ATTR;
void flush_icache(void)
{
    intptr_t b, e;

    for (b = (intptr_t)&CACHE_FLUSH_BASE, e = b + CACHE_SIZE;
         b < e; b += 16) {
        outl(0x0, b);
    }
}

void invalidate_icache(void) ICODE_ATTR;
void invalidate_icache(void)
{
    intptr_t b, e;

    /* Flush */
    for (b = (intptr_t)&CACHE_FLUSH_BASE, e = b + CACHE_SIZE;
         b < e; b += 16) {
        outl(0x0, b);
    }

    /* Invalidate */
    for (b = (intptr_t)&CACHE_INVALIDATE_BASE, e = b + CACHE_SIZE;
         b < e; b += 16) {
        outl(0x0, b);
    }
}

static void ipod_init_cache(void)
{
    intptr_t b, e;

/* Initialising the cache in the iPod bootloader prevents Rockbox from starting */
    PROC_STAT &= ~0x700;
    outl(0x4000, 0xcf004020);

    CACHE_CTL = CACHE_INIT;

    for (b = (intptr_t)&CACHE_INVALIDATE_BASE, e = b + CACHE_SIZE;
         b < e; b += 16) {
        outl(0x0, b);
    }

    /* Cache if (addr & mask) >> 16 == (mask & match) >> 16:
     * yes: 0x00000000 - 0x03ffffff
     *  no: 0x04000000 - 0x1fffffff
     * yes: 0x20000000 - 0x23ffffff
     *  no: 0x24000000 - 0x3fffffff <= range containing uncached alias
     */
    CACHE_MASK = 0x00001c00;
    CACHE_OPERATION = 0x3fc0;

    CACHE_CTL = CACHE_INIT | CACHE_RUN;
}
    
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
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
        CPU_INT_CLR    = -1;
        COP_INT_CLR    = -1;

        GPIOA_INT_EN   = 0;
        GPIOB_INT_EN   = 0;
        GPIOC_INT_EN   = 0;
        GPIOD_INT_EN   = 0;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
#if NUM_CORES > 1
        cpu_boost_init();
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
}

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}
