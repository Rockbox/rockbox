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
#include "thread.h"
#include "i2s.h"

#if NUM_CORES > 1
struct mutex boostctrl_mtx NOCACHEBSS_ATTR;
#endif

#ifndef BOOTLOADER
extern void TIMER1(void);
extern void TIMER2(void);

#if defined(IPOD_MINI) /* mini 1 only, mini 2G uses iPod 4G code */
extern void ipod_mini_button_int(void);

void irq(void)
{
    if(CURRENT_CORE == CPU)
    {
        if (CPU_INT_STAT & TIMER1_MASK)
            TIMER1();
        else if (CPU_INT_STAT & TIMER2_MASK)
            TIMER2();
        else if (CPU_HI_INT_STAT & GPIO_MASK)
            ipod_mini_button_int();
    } else {
        if (COP_INT_STAT & TIMER1_MASK)
            TIMER1();
        else if (COP_INT_STAT & TIMER2_MASK)
            TIMER2();
        else if (COP_HI_INT_STAT & GPIO_MASK)
            ipod_mini_button_int();
    }
}
#elif (defined IRIVER_H10) || (defined IRIVER_H10_5GB) || defined(ELIO_TPJ1022) \
    || (defined SANSA_E200)
/* TODO: this should really be in the target tree, but moving it there caused
   crt0.S not to find it while linking */
/* TODO: Even if it isn't in the target tree, this should be the default case */
#ifdef SANSA_E200
extern void button_int(void);
extern void clickwheel_int(void);
extern void microsd_int(void);
#endif

void irq(void)
{
    if(CURRENT_CORE == CPU) {
        if (CPU_INT_STAT & TIMER1_MASK) {
            TIMER1();
        }
        else if (CPU_INT_STAT & TIMER2_MASK) {
            TIMER2();
        }
#ifdef SANSA_E200
        else if (CPU_HI_INT_STAT & GPIO0_MASK) {
            if (GPIOA_INT_STAT & 0x80)
                microsd_int();
        }
        else if (CPU_HI_INT_STAT & GPIO1_MASK) {
            if (GPIOF_INT_STAT & 0xff)
                button_int();
            if (GPIOH_INT_STAT & 0xc0)
                clickwheel_int();
        }
#endif
    } else {
        if (COP_INT_STAT & TIMER1_MASK)
            TIMER1();
        else if (COP_INT_STAT & TIMER2_MASK)
            TIMER2();
    }
}
#else
extern void ipod_4g_button_int(void);

void irq(void)
{
    if(CURRENT_CORE == CPU)
    {
        if (CPU_INT_STAT & TIMER1_MASK)
            TIMER1();
        else if (CPU_INT_STAT & TIMER2_MASK)
            TIMER2();
        else if (CPU_HI_INT_STAT & I2C_MASK)
            ipod_4g_button_int();
    } else {
        if (COP_INT_STAT & TIMER1_MASK)
            TIMER1();
        else if (COP_INT_STAT & TIMER2_MASK)
            TIMER2();
        else if (COP_HI_INT_STAT & I2C_MASK)
            ipod_4g_button_int();
    }
}
#endif
#endif /* BOOTLOADER */

/* TODO: The following function has been lifted straight from IPL, and
   hence has a lot of numeric addresses used straight. I'd like to use
   #defines for these, but don't know what most of them are for or even what
   they should be named. Because of this I also have no way of knowing how
   to extend the funtions to do alternate cache configurations. */

#ifndef BOOTLOADER
static void ipod_init_cache(void)
{
/* Initialising the cache in the iPod bootloader prevents Rockbox from starting */
    unsigned i;

    /* cache init mode? */
    CACHE_CTL = CACHE_INIT;

    /* PP5002 has 8KB cache */
    for (i = 0xf0004000; i < 0xf0006000; i += 16) {
        outl(0x0, i);
    }

    outl(0x0, 0xf000f040);
    outl(0x3fc0, 0xf000f044);

    /* enable cache */
    CACHE_CTL = CACHE_ENABLE;

    for (i = 0x10000000; i < 0x10002000; i += 16)
        inb(i);
}

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
void set_cpu_frequency(long frequency)
#else
static void pp_set_cpu_frequency(long frequency)
#endif
{
    unsigned long clcd_clock_src;
    bool use_pll = true;

#if defined(HAVE_ADJUSTABLE_CPU_FREQ) && (NUM_CORES > 1)
    /* Using mutex or spinlock isn't safe here. */
    while (test_and_set(&boostctrl_mtx.locked, 1)) ;
#endif

#ifdef SANSA_E200
    i2s_scale_attn_level(CPUFREQ_DEFAULT);
#endif

    cpu_frequency = frequency;
    clcd_clock_src = CLCD_CLOCK_SRC; /* save selected color LCD clock source */

    CLOCK_SOURCE = (CLOCK_SOURCE & ~0xf000000f) | 0x10000002;
                                /* set clock source 1 to 24MHz and select it */
    CLCD_CLOCK_SRC &= ~0xc0000000; /* select 24MHz as color LCD clock source */

    switch (frequency)
    {
#if CONFIG_CPU == PP5020
      case CPUFREQ_MAX:
        DEV_TIMING1 = 0x00000808;
        PLL_CONTROL = 0x8a020a03; /* 10/3 * 24MHz */
        PLL_STATUS  = 0xd19b;     /* unlock frequencies > 66MHz */
        PLL_CONTROL = 0x8a020a03; /* repeat setup */
        udelay(500);              /* wait for relock */
        break;

      case CPUFREQ_NORMAL:
        DEV_TIMING1 = 0x00000303;
        PLL_CONTROL = 0x8a020504; /* 5/4 * 24MHz */
        udelay(500);              /* wait for relock */
        break;

      default:
        DEV_TIMING1 = 0x00000303;
        PLL_CONTROL &= ~0x80000000; /* disable PLL */
        use_pll = false;
        cpu_frequency = CPUFREQ_DEFAULT;
        break;

#elif (CONFIG_CPU == PP5022) || (CONFIG_CPU == PP5024)
      /* Note: The PP5022 PLL must be run at >= 96MHz
       * Bits 20..21 select the post divider (1/2/4/8).
       * PP5026 is similar to PP5022 except it doesn't
       * have this limitation (and the post divider?) */
      case CPUFREQ_MAX:
        DEV_TIMING1 = 0x00000808;
        PLL_CONTROL = 0x8a121403; /* (20/3 * 24MHz) / 2 */
        udelay(250);
        while (!(PLL_STATUS & 0x80000000)); /* wait for relock */
        break;

      case CPUFREQ_NORMAL:
        DEV_TIMING1 = 0x00000303;
        PLL_CONTROL = 0x8a220501; /* (5/1 * 24MHz) / 4 */
        udelay(250);
        while (!(PLL_STATUS & 0x80000000)); /* wait for relock */
        break;

      default:
        DEV_TIMING1 = 0x00000303;
        PLL_CONTROL &= ~0x80000000; /* disable PLL */
        use_pll = false;
        cpu_frequency = CPUFREQ_DEFAULT;
        break;
#endif
    }
    if (use_pll)                  /* set clock source 2 to PLL and select it */
        CLOCK_SOURCE = (CLOCK_SOURCE & ~0xf00000f0) | 0x20000070;

    CLCD_CLOCK_SRC;             /* dummy read (to sync the write pipeline??) */
    CLCD_CLOCK_SRC = clcd_clock_src; /* restore saved value */

#ifdef SANSA_E200
    i2s_scale_attn_level(frequency);
#endif

#if defined(HAVE_ADJUSTABLE_CPU_FREQ) && (NUM_CORES > 1)
    boostctrl_mtx.locked = 0;
#endif
}
#endif /* !BOOTLOADER */

void system_init(void)
{
#ifndef BOOTLOADER
    if (CURRENT_CORE == CPU)
    {
#ifdef SANSA_E200
        /* Reset all devices */
        DEV_RS = 0x3bfffef8;
        outl(0xffffffff, 0x60006008);
        DEV_RS = 0;
        outl(0x00000000, 0x60006008);
#endif
        /* Remap the flash ROM from 0x00000000 to 0x20000000. */
        MMAP3_LOGICAL  = 0x20000000 | 0x3a00;
        MMAP3_PHYSICAL = 0x00000000 | 0x3f84;

        /* disable all irqs */
        COP_HI_INT_CLR      = -1;
        CPU_HI_INT_CLR      = -1;
        HI_INT_FORCED_CLR   = -1;
        
        COP_INT_CLR         = -1;
        CPU_INT_CLR         = -1;
        INT_FORCED_CLR      = -1;

        GPIOA_INT_EN        = 0;
        GPIOB_INT_EN        = 0;
        GPIOC_INT_EN        = 0;
        GPIOD_INT_EN        = 0;
        GPIOE_INT_EN        = 0;
        GPIOF_INT_EN        = 0;
        GPIOG_INT_EN        = 0;
        GPIOH_INT_EN        = 0;
        GPIOI_INT_EN        = 0;
        GPIOJ_INT_EN        = 0;
        GPIOK_INT_EN        = 0;
        GPIOL_INT_EN        = 0;

#ifdef SANSA_E200
        /* outl(0x00000000, 0x6000b000); */
        outl(inl(0x6000a000) | 0x80000000, 0x6000a000); /* Init DMA controller? */
#endif 

        DEV_INIT |= 1 << 30; /* enable PLL power */

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
#if NUM_CORES > 1
        spinlock_init(&boostctrl_mtx);
#endif
#else
        pp_set_cpu_frequency(CPUFREQ_MAX);
#endif
    }
    ipod_init_cache();

#endif /* BOOTLOADER */
}

void system_reboot(void)
{
    /* Reboot */
    DEV_RS |= DEV_SYSTEM;
}

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}
