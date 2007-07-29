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

void irq(void)
{
    if(CURRENT_CORE == CPU)
    {
        if (CPU_INT_STAT & TIMER1_MASK)
            TIMER1();
        else if (CPU_INT_STAT & TIMER2_MASK)
            TIMER2();
        else if (CPU_INT_STAT & GPIO_MASK)
            ipod_3g_button_int();
    } else {
        if (COP_INT_STAT & TIMER1_MASK)
            TIMER1();
        else if (COP_INT_STAT & TIMER2_MASK)
            TIMER2();
    }
}

#endif

unsigned int current_core(void)
{
    if(((*(volatile unsigned long *)(0xc4000000)) & 0xff) == 0x55)
    {
        return CPU;
    }
    return COP;
}


/* TODO: The following two function have been lifted straight from IPL, and
   hence have a lot of numeric addresses used straight. I'd like to use
   #defines for these, but don't know what most of them are for or even what
   they should be named. Because of this I also have no way of knowing how
   to extend the funtions to do alternate cache configurations and/or
   some other CPU frequency scaling. */

#ifndef BOOTLOADER
static void ipod_init_cache(void)
{
    int i =0;
/* Initialising the cache in the iPod bootloader prevents Rockbox from starting */
    outl(inl(0xcf004050) & ~0x700, 0xcf004050);
    outl(0x4000, 0xcf004020);

    outl(0x2, 0xcf004024);

    /* PP5002 has 8KB cache */
    for (i = 0xf0004000; i < (int)(0xf0006000); i += 16) {
        outl(0x0, i);
    }

    outl(0x0, 0xf000f020);
    outl(0x3fc0, 0xf000f024);

    outl(0x3, 0xcf004024);
}
    
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
void set_cpu_frequency(long frequency)
{
    unsigned long postmult;

    if (CURRENT_CORE == CPU)
    {
        if (frequency == CPUFREQ_NORMAL)
            postmult = CPUFREQ_NORMAL_MULT;
        else if (frequency == CPUFREQ_MAX)
            postmult = CPUFREQ_MAX_MULT;
        else
            postmult = CPUFREQ_DEFAULT_MULT;
        cpu_frequency = frequency;

        outl(0xd19b, 0xcf005038);

        outl(inl(0xcf005010) | 0x6000, 0xcf005010);
        outl(0x01, 0xcf005008);
        outl(0xa9, 0xcf00500c);
        outl(0xe000, 0xcf005010);

        /* Clock frequency = (24/4)*postmult */
        outl(4, 0xcf005018);
        outl(postmult, 0xcf00501c);

        /* Wait for PLL relock? */
        udelay(200);

        outl(0x02, 0xcf005008);
    }
}
#elif !defined(BOOTLOADER)
static void ipod_set_cpu_speed(void)
{
    outl(0xd19b, 0xcf005038);

    outl(0x02, 0xcf005008);
    outl(0x55, 0xcf00500c);
    outl(0x6000, 0xcf005010);

    /* 78  MHz (24*13/4) */
    outl(4, 0xcf005018);
    outl(13, 0xcf00501c);

    outl(0xe000, 0xcf005010);

    udelay(2000);

    outl(0xa8, 0xcf00500c);
}
#endif

void system_init(void)
{
#ifndef BOOTLOADER
    if (CURRENT_CORE == CPU)
    {
        /* Remap the flash ROM from 0x00000000 to 0x20000000. */
        MMAP3_LOGICAL  = 0x20000000 | 0x3a00;
        MMAP3_PHYSICAL = 0x00000000 | 0x3f84;

        INT_FORCED_CLR = -1;
        CPU_INT_CLR    = -1;
        COP_INT_CLR    = -1;

        GPIOA_INT_EN   = 0;
        GPIOB_INT_EN   = 0;
        GPIOC_INT_EN   = 0;
        GPIOD_INT_EN   = 0;

#ifndef HAVE_ADJUSTABLE_CPU_FREQ
        ipod_set_cpu_speed();
#endif
    }
    ipod_init_cache();
#endif
}

void system_reboot(void)
{
    outl(inl(0xcf005030) | 0x4, 0xcf005030);
}

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}


