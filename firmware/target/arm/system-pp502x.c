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
#include "i2c-pp.h"

#ifndef BOOTLOADER
extern void TIMER1(void);
extern void TIMER2(void);
extern void ipod_mini_button_int(void); /* iPod Mini 1st gen only */
extern void ipod_4g_button_int(void);   /* iPod 4th gen and higher only */
#ifdef SANSA_E200
extern void button_int(void);
extern void clickwheel_int(void);
extern void microsd_int(void);
#endif

#ifdef HAVE_USBSTACK
#include "usbstack/core.h"
#endif

void irq(void)
{
    if(CURRENT_CORE == CPU)
    {
        if (CPU_INT_STAT & TIMER1_MASK) {
            TIMER1();
#ifdef HAVE_USBSTACK
            usb_stack_irq();
#endif
        } else if (CPU_INT_STAT & TIMER2_MASK)
            TIMER2();
#if defined(IPOD_MINI) /* Mini 1st gen only, mini 2nd gen uses iPod 4G code */
        else if (CPU_HI_INT_STAT & GPIO_MASK)
            ipod_mini_button_int();
#elif CONFIG_KEYPAD == IPOD_4G_PAD /* except Mini 1st gen, handled above */
        else if (CPU_HI_INT_STAT & I2C_MASK)
            ipod_4g_button_int();
#elif defined(SANSA_E200)
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
        if (COP_INT_STAT & TIMER2_MASK)
            TIMER2();
    }
}
#endif /* BOOTLOADER */

/* TODO: The following function has been lifted straight from IPL, and
   hence has a lot of numeric addresses used straight. I'd like to use
   #defines for these, but don't know what most of them are for or even what
   they should be named. Because of this I also have no way of knowing how
   to extend the funtions to do alternate cache configurations. */

#ifndef BOOTLOADER
void flush_icache(void) ICODE_ATTR;
void flush_icache(void)
{
    if (CACHE_CTL & CACHE_CTL_ENABLE)
    {
        CACHE_OPERATION |= CACHE_OP_FLUSH;
        while ((CACHE_CTL & CACHE_CTL_BUSY) != 0);
    }
}

void invalidate_icache(void) ICODE_ATTR;
void invalidate_icache(void)
{
    if (CACHE_CTL & CACHE_CTL_ENABLE)
    {
        CACHE_OPERATION |= CACHE_OP_FLUSH | CACHE_OP_INVALIDATE;
        while ((CACHE_CTL & CACHE_CTL_BUSY) != 0);
        nop; nop; nop; nop;
    }
}

static void init_cache(void)
{
/* Initialising the cache in the iPod bootloader prevents Rockbox from starting */

    /* cache init mode */
    CACHE_CTL |= CACHE_CTL_INIT;

    /* what's this do? */
    CACHE_PRIORITY |= CURRENT_CORE == CPU ? 0x10 : 0x20;

    /* Cache if (addr & mask) >> 16 == (mask & match) >> 16:
     * yes: 0x00000000 - 0x03ffffff
     *  no: 0x04000000 - 0x1fffffff
     * yes: 0x20000000 - 0x23ffffff
     *  no: 0x24000000 - 0x3fffffff
     */
    CACHE_MASK = 0x00001c00;
    CACHE_OPERATION = 0xfc0;

    /* enable cache */
    CACHE_CTL |= CACHE_CTL_INIT | CACHE_CTL_ENABLE | CACHE_CTL_RUN;
    nop; nop; nop; nop;
}

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
void scale_suspend_core(bool suspend) ICODE_ATTR;
void scale_suspend_core(bool suspend)
{
    unsigned int core = CURRENT_CORE;
    unsigned int othercore = 1 - core;
    static unsigned long proc_bits IBSS_ATTR;
    static int oldstatus IBSS_ATTR;

    if (suspend)
    {
        oldstatus = set_interrupt_status(IRQ_FIQ_DISABLED, IRQ_FIQ_STATUS);
        proc_bits = PROC_CTL(othercore) & 0xc0000000;
        PROC_CTL(othercore) = 0x40000000; nop;
        PROC_CTL(core) = 0x48000003; nop;
    }
    else
    {
        PROC_CTL(core) = 0x4800001f; nop;
        if (proc_bits == 0)
            PROC_CTL(othercore) = 0;
        set_interrupt_status(oldstatus, IRQ_FIQ_STATUS);
    }
}

void set_cpu_frequency(long frequency) ICODE_ATTR;
void set_cpu_frequency(long frequency)
#else
static void pp_set_cpu_frequency(long frequency)
#endif
{
#if defined(HAVE_ADJUSTABLE_CPU_FREQ) && (NUM_CORES > 1)
    spinlock_lock(&boostctrl_spin);
#endif

    scale_suspend_core(true);

    cpu_frequency = frequency;

    switch (frequency)
    {
      /* Note: The PP5022 PLL must be run at >= 96MHz
       * Bits 20..21 select the post divider (1/2/4/8).
       * PP5026 is similar to PP5022 except it doesn't
       * have this limitation (and the post divider?) */
      case CPUFREQ_MAX:
        CLOCK_SOURCE = 0x10007772;  /* source #1: 24MHz, #2, #3, #4: PLL */
        DEV_TIMING1  = 0x00000303;
#if CONFIG_CPU == PP5020
        PLL_CONTROL  = 0x8a020a03;  /* 10/3 * 24MHz */
        PLL_STATUS   = 0xd19b;      /* unlock frequencies > 66MHz */
        PLL_CONTROL  = 0x8a020a03;  /* repeat setup */
        scale_suspend_core(false);
        udelay(500);                /* wait for relock */
#elif (CONFIG_CPU == PP5022) || (CONFIG_CPU == PP5024)
        PLL_CONTROL  = 0x8a121403;  /* (20/3 * 24MHz) / 2 */
        scale_suspend_core(false);
        udelay(250);
        while (!(PLL_STATUS & 0x80000000)); /* wait for relock */
#endif
        scale_suspend_core(true);
        break;

      case CPUFREQ_NORMAL:
        CLOCK_SOURCE = 0x10007772;  /* source #1: 24MHz, #2, #3, #4: PLL */
        DEV_TIMING1  = 0x00000303;
#if CONFIG_CPU == PP5020
        PLL_CONTROL  = 0x8a020504;  /* 5/4 * 24MHz */
        scale_suspend_core(false);
        udelay(500);                /* wait for relock */
#elif (CONFIG_CPU == PP5022) || (CONFIG_CPU == PP5024)
        PLL_CONTROL  = 0x8a220501;  /* (5/1 * 24MHz) / 4 */
        scale_suspend_core(false);
        udelay(250);
        while (!(PLL_STATUS & 0x80000000)); /* wait for relock */
#endif
        scale_suspend_core(true);
        break;

      case CPUFREQ_SLEEP:
        CLOCK_SOURCE = 0x10002202;  /* source #2: 32kHz, #1, #3, #4: 24MHz */
        PLL_CONTROL &= ~0x80000000; /* disable PLL */
        scale_suspend_core(false);
        udelay(10000);              /* let 32kHz source stabilize? */
        scale_suspend_core(true);
        break;

      default:
        CLOCK_SOURCE = 0x10002222;  /* source #1, #2, #3, #4: 24MHz */
        DEV_TIMING1  = 0x00000303;
        PLL_CONTROL &= ~0x80000000; /* disable PLL */
        cpu_frequency = CPUFREQ_DEFAULT;
        PROC_CTL(CURRENT_CORE) = 0x4800001f; nop;
        break;
    }

    if (frequency == CPUFREQ_MAX)
        DEV_TIMING1 = 0x00000808;

    CLOCK_SOURCE = (CLOCK_SOURCE & ~0xf0000000) | 0x20000000;  /* select source #2 */

    scale_suspend_core(false);

#if defined(HAVE_ADJUSTABLE_CPU_FREQ) && (NUM_CORES > 1)
    spinlock_unlock(&boostctrl_spin);
#endif
}
#endif /* !BOOTLOADER */

void system_init(void)
{
#ifndef BOOTLOADER
    if (CURRENT_CORE == CPU)
    {
#if defined(SANSA_E200) || defined(SANSA_C200)
        /* Reset all devices */
        DEV_OFF_MASK |= 0x20;
        DEV_RS = 0x3bfffef8;
        DEV_OFF_MASK = -1;
        DEV_RS = 0;
        DEV_OFF_MASK = 0;
 #elif defined (IRIVER_H10)
        DEV_RS = 0x3ffffef8;
        DEV_OFF_MASK = -1;
        outl(inl(0x70000024) | 0xc0, 0x70000024);
        DEV_RS = 0;
        DEV_OFF_MASK = 0;
#endif

#if !defined(SANSA_E200) && !defined(SANSA_C200)
        /* Remap the flash ROM on CPU, keep hidden from COP:
         * 0x00000000-0x3fffffff = 0x20000000-0x23ffffff */
        MMAP1_LOGICAL  = 0x20003c00;
        MMAP1_PHYSICAL = 0x00003084 |
            MMAP_PHYS_READ_MASK | MMAP_PHYS_WRITE_MASK |
            MMAP_PHYS_DATA_MASK | MMAP_PHYS_CODE_MASK;
#endif

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

#if defined(SANSA_E200) || defined(SANSA_C200)
        /* outl(0x00000000, 0x6000b000); */
        outl(inl(0x6000a000) | 0x80000000, 0x6000a000); /* Init DMA controller? */
#endif 

        DEV_INIT |= 1 << 30; /* enable PLL power */

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
#if NUM_CORES > 1
        cpu_boost_init();
#endif
#else
        pp_set_cpu_frequency(CPUFREQ_MAX);
#endif
    }

    init_cache();
#endif /* BOOTLOADER */
}

void system_reboot(void)
{
    /* Reboot */
#ifdef SANSA_C200
    CACHE_CTL &= ~CACHE_CTL_VECT_REMAP;

    pp_i2c_send( 0x46, 0x23, 0x0); /* backlight off */

    /* Magic used by the c200 OF: 0x23066000
       Magic used by the c200 BL: 0x23066b7b
       In both cases, the OF executes these 2 commands from iram. */
    STRAP_OPT_A = 0x23066b7b;
    DEV_RS = DEV_SYSTEM;
#else
    DEV_RS |= DEV_SYSTEM;
#endif
}

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}
