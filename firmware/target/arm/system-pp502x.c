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
#include "thread.h"
#include "i2s.h"
#include "i2c-pp.h"
#include "as3514.h"
#include "ata-sd-target.h"
#include "button-target.h"
#ifdef HAVE_USBSTACK
#include "usb_drv.h"
#endif

#ifndef BOOTLOADER
extern void TIMER1(void);
extern void TIMER2(void);
extern void SERIAL0(void);
extern void ipod_mini_button_int(void); /* iPod Mini 1st gen only */
extern void ipod_4g_button_int(void);   /* iPod 4th gen and higher only */

void irq(void)
{
    if(CURRENT_CORE == CPU)
    {
        if (CPU_INT_STAT & TIMER1_MASK) {
            TIMER1();
        } else if (CPU_INT_STAT & TIMER2_MASK)
            TIMER2();
#if defined(IPOD_MINI) /* Mini 1st gen only, mini 2nd gen uses iPod 4G code */
        else if (CPU_HI_INT_STAT & GPIO0_MASK)
            ipod_mini_button_int();
#elif CONFIG_KEYPAD == IPOD_4G_PAD /* except Mini 1st gen, handled above */
        else if (CPU_HI_INT_STAT & I2C_MASK)
            ipod_4g_button_int();
#elif defined(SANSA_E200)
#ifdef HAVE_HOTSWAP
        else if (CPU_HI_INT_STAT & GPIO0_MASK) {
            if (GPIOA_INT_STAT & 0x80)
                microsd_int();
        }
#endif
        else if (CPU_HI_INT_STAT & GPIO1_MASK) {
            if (GPIOF_INT_STAT & 0xff)
                button_int();
            if (GPIOH_INT_STAT & 0xc0)
                clickwheel_int();
        }
#elif defined(SANSA_C200) && defined(HAVE_HOTSWAP)
        else if (CPU_HI_INT_STAT & GPIO2_MASK) {
            if (GPIOL_INT_STAT & 0x08)
                microsd_int();
        }
#elif defined(MROBE_100)
        else if (CPU_HI_INT_STAT & GPIO0_MASK) {
            if (GPIOD_INT_STAT & 0x2)
                button_int();
        }        
#elif defined(PHILIPS_HDD1630)
        else if (CPU_HI_INT_STAT & GPIO0_MASK) {
            if (GPIOA_INT_STAT & 0x20)
                button_int();
        }        
#endif
#ifdef IPOD_ACCESSORY_PROTOCOL
	else if (CPU_HI_INT_STAT & SER0_MASK) {
	    SERIAL0();
	}
#endif
#ifdef HAVE_USBSTACK
        else if (CPU_INT_STAT & USB_MASK) {
            usb_drv_int();
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
#endif /* !BOOTLOADER */

/* We need this for Sansas since we boost the cpu in their bootloader */
#if !defined(BOOTLOADER) || defined(SANSA_E200) || defined(SANSA_C200)
void scale_suspend_core(bool suspend) ICODE_ATTR;
void scale_suspend_core(bool suspend)
{
    unsigned int core = CURRENT_CORE;
    IF_COP( unsigned int othercore = 1 - core; )
    static int oldstatus IBSS_ATTR;

    if (suspend)
    {
        oldstatus = disable_interrupt_save(IRQ_FIQ_STATUS);
        IF_COP( PROC_CTL(othercore) = 0x40000000; nop; )
        PROC_CTL(core) = 0x48000003; nop;
    }
    else
    {
        PROC_CTL(core) = 0x4800001f; nop;
        IF_COP( PROC_CTL(othercore) = 0x00000000; nop; )
        restore_interrupt(oldstatus);
    }
}

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
void set_cpu_frequency(long frequency) ICODE_ATTR;
void set_cpu_frequency(long frequency)
#else
static void pp_set_cpu_frequency(long frequency)
#endif
{      
#if defined(HAVE_ADJUSTABLE_CPU_FREQ) && (NUM_CORES > 1)
    spinlock_lock(&boostctrl_spin);
#endif

    switch (frequency)
    {
      /* Note1: The PP5022 PLL must be run at >= 96MHz
       * Bits 20..21 select the post divider (1/2/4/8).
       * PP5026 is similar to PP5022 except it doesn't
       * have this limitation (and the post divider?) 
       * Note2: CLOCK_SOURCE is set via 0=32kHz, 1=16MHz,
       * 2=24MHz, 3=33MHz, 4=48MHz, 5=SLOW, 6=FAST, 7=PLL. 
       * SLOW = 24MHz / (DIV_SLOW + 1), DIV = Bits 16-19
       * FAST = PLL   / (DIV_FAST + 1), DIV = Bits 20-23 */
      case CPUFREQ_SLEEP:
        cpu_frequency =  CPUFREQ_SLEEP;
        PLL_CONTROL  |=  0x0c000000;
        scale_suspend_core(true);
        CLOCK_SOURCE  =  0x20000000; /* source #1, #2, #3, #4: 32kHz (#2 active) */
        scale_suspend_core(false);
        PLL_CONTROL  &= ~0x80000000; /* disable PLL */
        DEV_INIT2    &= ~INIT_PLL;   /* disable PLL power */
        break;
        
      case CPUFREQ_MAX:
        cpu_frequency = CPUFREQ_MAX;
        DEV_INIT2    |= INIT_PLL;   /* enable PLL power */
        PLL_CONTROL  |= 0x88000000; /* enable PLL */
        scale_suspend_core(true);
        CLOCK_SOURCE  = 0x20002222; /* source #1, #2, #3, #4: 24MHz (#2 active) */
        DEV_TIMING1   = 0x00000303;
        scale_suspend_core(false);
#if   defined(IPOD_MINI2G)
        MLCD_SCLK_DIV = 0x00000001; /* Mono LCD bridge serial clock divider */
#elif defined(IPOD_NANO)
        IDE0_CFG     |= 0x10000000; /* set ">65MHz" bit */
#endif
#if CONFIG_CPU == PP5020
        PLL_CONTROL   = 0x8a020a03; /* 80 MHz = 10/3 * 24MHz */
        PLL_STATUS    = 0xd19b;     /* unlock frequencies > 66MHz */
        PLL_CONTROL   = 0x8a020a03; /* repeat setup */
        udelay(500);                /* wait for relock */
#elif (CONFIG_CPU == PP5022) || (CONFIG_CPU == PP5024)
        PLL_CONTROL   = 0x8a121403; /*  80 MHz = (20/3 * 24MHz) / 2 */
        while (!(PLL_STATUS & 0x80000000)); /* wait for relock */
#endif
        scale_suspend_core(true);
        DEV_TIMING1   = 0x00000808;
        CLOCK_SOURCE  = 0x20007777; /* source #1, #2, #3, #4: PLL (#2 active) */
        scale_suspend_core(false);
        break;
#if 0 /******** CPUFREQ_NORMAL = 24MHz without PLL ********/
      case CPUFREQ_NORMAL:
        cpu_frequency =  CPUFREQ_NORMAL;
        PLL_CONTROL  |=  0x08000000;
        scale_suspend_core(true);
        CLOCK_SOURCE  =  0x20002222; /* source #1, #2, #3, #4: 24MHz (#2 active) */
        DEV_TIMING1   =  0x00000303;
#if   defined(IPOD_MINI2G)
        MLCD_SCLK_DIV =  0x00000000; /* Mono LCD bridge serial clock divider */
#elif defined(IPOD_NANO)
        IDE0_CFG     &= ~0x10000000; /* clear ">65MHz" bit */
#endif
        scale_suspend_core(false);
        PLL_CONTROL  &= ~0x80000000; /* disable PLL */
        DEV_INIT2    &= ~INIT_PLL;   /* disable PLL power */
        break;
#else /******** CPUFREQ_NORMAL = 30MHz with PLL ********/
    case CPUFREQ_NORMAL:
        cpu_frequency = CPUFREQ_NORMAL;
        DEV_INIT2    |= INIT_PLL;   /* enable PLL power */
        PLL_CONTROL  |= 0x88000000; /* enable PLL */
        scale_suspend_core(true);
        CLOCK_SOURCE  = 0x20002222; /* source #1, #2, #3, #4: 24MHz (#2 active) */
        DEV_TIMING1   = 0x00000303;
        scale_suspend_core(false);
#if   defined(IPOD_MINI2G)
        MLCD_SCLK_DIV =  0x00000000; /* Mono LCD bridge serial clock divider */
#elif defined(IPOD_NANO)
        IDE0_CFG     &= ~0x10000000; /* clear ">65MHz" bit */
#endif
#if CONFIG_CPU == PP5020
        PLL_CONTROL   = 0x8a020504; /* 30 MHz = 5/4 * 24MHz */
        udelay(500);                /* wait for relock */
#elif (CONFIG_CPU == PP5022) || (CONFIG_CPU == PP5024)
        PLL_CONTROL   = 0x8a220501; /* 30 MHz = (5/1 * 24MHz) / 4 */
        while (!(PLL_STATUS & 0x80000000)); /* wait for relock */
#endif
        scale_suspend_core(true);
        DEV_TIMING1   = 0x00000303;
        CLOCK_SOURCE  = 0x20007777; /* source #1, #2, #3, #4: PLL (#2 active) */
        scale_suspend_core(false);
        break;
#endif /******** CPUFREQ_NORMAL end ********/
      default:
        cpu_frequency =  CPUFREQ_DEFAULT;
        PLL_CONTROL  |=  0x08000000;
        scale_suspend_core(true);
        CLOCK_SOURCE  =  0x20002222; /* source #1, #2, #3, #4: 24MHz (#2 active) */
        DEV_TIMING1   =  0x00000303;
#if   defined(IPOD_MINI2G)
        MLCD_SCLK_DIV =  0x00000000; /* Mono LCD bridge serial clock divider */
#elif defined(IPOD_NANO)
        IDE0_CFG     &= ~0x10000000; /* clear ">65MHz" bit */
#endif
        scale_suspend_core(false);
        PLL_CONTROL  &= ~0x80000000; /* disable PLL */
        DEV_INIT2    &= ~INIT_PLL;   /* disable PLL power */
        break;
    }
    
#if defined(HAVE_ADJUSTABLE_CPU_FREQ) && (NUM_CORES > 1)
    spinlock_unlock(&boostctrl_spin);
#endif
}
#endif /* !BOOTLOADER || SANSA_E200 || SANSA_C200 */

void system_init(void)
{
#ifndef BOOTLOADER
    if (CURRENT_CORE == CPU)
    {
#if defined (IRIVER_H10) || defined(IRIVER_H10_5GB) || defined(IPOD_COLOR)
        /* set minimum startup configuration */
        DEV_EN         = 0xc2000124;
        DEV_EN2        = 0x00002000;
        CACHE_PRIORITY = 0x0000003f;
        GPO32_VAL      = 0x20000000;
        DEV_INIT1      = 0xdc000000;
        DEV_INIT2      = 0x40000000;

        /* reset all allowed devices */
        DEV_RS         = 0x3ffffef8;
        DEV_RS2        = 0xffffdfff;
        DEV_RS         = 0x00000000;
        DEV_RS2        = 0x00000000;
#elif defined (IPOD_VIDEO)    
        /* set minimum startup configuration */
        DEV_EN         = 0xc2000124;
        DEV_EN2        = 0x00000000;
        CACHE_PRIORITY = 0x0000003f;
        GPO32_VAL      = 0x00004000;
        DEV_INIT1      = 0x00000000;
        DEV_INIT2      = 0x40000000;

        /* reset all allowed devices */
        DEV_RS         = 0x3ffffef8;
        DEV_RS2        = 0xffffffff;
        DEV_RS         = 0x00000000;
        DEV_RS2        = 0x00000000;
#elif defined (IPOD_NANO)    
        /* set minimum startup configuration */
        DEV_EN         = 0xc2000124;
        DEV_EN2        = 0x00002000;
        CACHE_PRIORITY = 0x0000003f;
        GPO32_VAL      = 0x50000000;
        DEV_INIT1      = 0xa8000000;
        DEV_INIT2      = 0x40000000;

        /* reset all allowed devices */
        DEV_RS         = 0x3ffffef8;
        DEV_RS2        = 0xffffdfff;
        DEV_RS         = 0x00000000;
        DEV_RS2        = 0x00000000;
#elif defined(SANSA_C200) || defined (SANSA_E200)
        /* set minimum startup configuration */
        DEV_EN         = 0xc4000124;
        DEV_EN2        = 0x00000000;
        CACHE_PRIORITY = 0x0000003f;
        GPO32_VAL      = 0x10000000;
        DEV_INIT1      = 0x54000000;
        DEV_INIT2      = 0x40000000;

        /* reset all allowed devices */
        DEV_RS         = 0x3bfffef8;
        DEV_RS2        = 0xffffffff;
        DEV_RS         = 0x00000000;
        DEV_RS2        = 0x00000000;
#elif defined(IPOD_4G)
        /* set minimum startup configuration */
        DEV_EN         = 0xc2020124;
        DEV_EN2        = 0x00000000;
        CACHE_PRIORITY = 0x0000003f;
        GPO32_VAL      = 0x02000000;
        DEV_INIT1      = 0x00000000;
        DEV_INIT2      = 0x40000000;

        /* reset all allowed devices */
        DEV_RS         = 0x3ffdfef8;
        DEV_RS2        = 0xffffffff;
        DEV_RS         = 0x00000000;
        DEV_RS2        = 0x00000000;
#elif defined (IPOD_MINI)
        /* to be done */
#elif defined (IPOD_MINI2G)
        /* to be done */
#elif defined (MROBE_100)
        /* to be done */
#elif defined (ELIO_TPJ1022)
        /* to be done */
#endif

#if !defined(SANSA_E200) && !defined(SANSA_C200) && !defined(PHILIPS_SA9200)
        /* Remap the flash ROM on CPU, keep hidden from COP:
         * 0x00000000-0x3fffffff = 0x20000000-0x23ffffff */
        MMAP1_LOGICAL  = 0x20003c00;
        MMAP1_PHYSICAL = 0x00003084 |
            MMAP_PHYS_READ_MASK | MMAP_PHYS_WRITE_MASK |
            MMAP_PHYS_DATA_MASK | MMAP_PHYS_CODE_MASK;
#endif

        /* disable all irqs */
        COP_HI_INT_DIS      = -1;
        CPU_HI_INT_DIS      = -1;
        HI_INT_FORCED_CLR   = -1;
        
        COP_INT_DIS         = -1;
        CPU_INT_DIS         = -1;
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

#if defined(SANSA_E200) || defined(SANSA_C200) || defined(PHILIPS_SA9200)
        /* outl(0x00000000, 0x6000b000); */
        outl(inl(0x6000a000) | 0x80000000, 0x6000a000); /* Init DMA controller? */
#endif 

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
#if NUM_CORES > 1
        cpu_boost_init();
#endif
#else
        pp_set_cpu_frequency(CPUFREQ_MAX);
#endif
    }

    init_cache();
#else /* BOOTLOADER */
    if (CURRENT_CORE == CPU)
    {
#if defined(SANSA_C200) || defined (SANSA_E200)
        pp_set_cpu_frequency(CPUFREQ_MAX);
#endif
    }
#endif /* BOOTLOADER */
}

void system_reboot(void)
{
    /* Reboot */
#if defined(SANSA_E200) || defined(SANSA_C200) || defined(PHILIPS_SA9200)
    CACHE_CTL &= ~CACHE_CTL_VECT_REMAP;

    /* Magic used by the c200 OF: 0x23066000
       Magic used by the c200 BL: 0x23066b7b
       In both cases, the OF executes these 2 commands from iram. */
    STRAP_OPT_A = 0x23066b7b;
    DEV_RS = DEV_SYSTEM;
#else
    DEV_RS |= DEV_SYSTEM;
#endif
    /* wait until reboot kicks in */
    while (1);
}

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}
