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

unsigned int ipod_hw_rev;

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
extern void button_int(void);
extern void clickwheel_int(void);

void irq(void)
{
    if(CURRENT_CORE == CPU) {
        if (CPU_INT_STAT & TIMER1_MASK) {
#ifdef SANSA_E200
            if (GPIOF_INT_STAT & 0xff)
                button_int();
            if (GPIOH_INT_STAT & 0xc0)
                clickwheel_int();
#endif
            TIMER1();
        }
        else if (CPU_INT_STAT & TIMER2_MASK)
            TIMER2();
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

/* TODO: The following two function have been lifted straight from IPL, and
   hence have a lot of numeric addresses used straight. I'd like to use
   #defines for these, but don't know what most of them are for or even what
   they should be named. Because of this I also have no way of knowing how
   to extend the funtions to do alternate cache configurations and/or
   some other CPU frequency scaling. */

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
#endif

/* Not all iPod targets support CPU freq. boosting yet */
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
void set_cpu_frequency(long frequency)
{
    unsigned long postmult;

# if NUM_CORES > 1
    /* Using mutex or spinlock isn't safe here. */
    while (test_and_set(&boostctrl_mtx.locked, 1)) ;
# endif
    
    if (frequency == CPUFREQ_NORMAL)
        postmult = CPUFREQ_NORMAL_MULT;
    else if (frequency == CPUFREQ_MAX)
        postmult = CPUFREQ_MAX_MULT;
    else
        postmult = CPUFREQ_DEFAULT_MULT;
    cpu_frequency = frequency;

    /* Enable PLL? */
    outl(inl(0x70000020) | (1<<30), 0x70000020);
    
    /* Select 24MHz crystal as clock source? */
    outl((inl(0x60006020) & 0x0fffff0f) | 0x20000020, 0x60006020);
    
    /* Clock frequency = (24/8)*postmult */
    outl(0xaa020000 | 8 | (postmult << 8), 0x60006034);
    
    /* Wait for PLL relock? */
    udelay(2000);
    
    /* Select PLL as clock source? */
    outl((inl(0x60006020) & 0x0fffff0f) | 0x20000070, 0x60006020);
    
# if defined(IPOD_COLOR) || defined(IPOD_4G) || defined(IPOD_MINI) || defined(IRIVER_H10) || defined(IRIVER_H10_5GB)
    /* We don't know why the timer interrupt gets disabled on the PP5020
     based ipods, but without the following line, the 4Gs will freeze
     when CPU frequency changing is enabled.

     Note also that a simple "CPU_INT_EN = TIMER1_MASK;" (as used
     elsewhere to enable interrupts) doesn't work, we need "|=".

     It's not needed on the PP5021 and PP5022 ipods.
     */

    /* unmask interrupt source */
    CPU_INT_EN |= TIMER1_MASK;
    COP_INT_EN |= TIMER1_MASK;
# endif
    
# if NUM_CORES > 1
    boostctrl_mtx.locked = 0;
# endif
}
#elif !defined(BOOTLOADER)
void ipod_set_cpu_frequency(void)
{
/* For e200, just use clocking set up by OF loader for now */
#ifndef SANSA_E200
    /* Enable PLL? */
    outl(inl(0x70000020) | (1<<30), 0x70000020);

    /* Select 24MHz crystal as clock source? */
    outl((inl(0x60006020) & 0x0fffff0f) | 0x20000020, 0x60006020);

    /* Clock frequency = (24/8)*25 = 75MHz */
    outl(0xaa020000 | 8 | (25 << 8), 0x60006034);
    /* Wait for PLL relock? */
    udelay(2000);

    /* Select PLL as clock source? */
    outl((inl(0x60006020) & 0x0fffff0f) | 0x20000070, 0x60006020);
#endif /* SANSA_E200 */
}
#endif

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

        /* The hw revision is written to the last 4 bytes of SDRAM by the
           bootloader - we save it before Rockbox overwrites it. */
        ipod_hw_rev = (*((volatile unsigned long*)(0x01fffffc)));

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
    }

    ipod_init_cache();
#else /* !sansa E200 */

# if NUM_CORES > 1 && defined(HAVE_ADJUSTABLE_CPU_FREQ)
        spinlock_init(&boostctrl_mtx);
# endif
        
#if (!defined HAVE_ADJUSTABLE_CPU_FREQ) && (NUM_CORES == 1)
        ipod_set_cpu_frequency();
#endif
    }
#if (!defined HAVE_ADJUSTABLE_CPU_FREQ) && (NUM_CORES > 1)
    else
    {
        ipod_set_cpu_frequency();
    }
#endif
    ipod_init_cache();

#endif /* SANSA_E200 */

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
