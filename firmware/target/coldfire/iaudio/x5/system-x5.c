/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "cpu.h"
#include "kernel.h"
#include "system.h"
#include "power.h"
#include "timer.h"

#define MAX_REFRESH_TIMER     59
#define NORMAL_REFRESH_TIMER  21
#define DEFAULT_REFRESH_TIMER 4

void set_cpu_frequency (long) __attribute__ ((section (".icode")));
void set_cpu_frequency(long frequency)
{
    switch(frequency)
    {
    case CPUFREQ_MAX:
        DCR = (0x8200 | DEFAULT_REFRESH_TIMER);
              /* Refresh timer for bypass frequency */
        PLLCR &= ~1;  /* Bypass mode */
        timers_adjust_prescale(CPUFREQ_DEFAULT_MULT, false);
        PLLCR = 0x13442045;
        CSCR0 = 0x00001180; /* Flash: 4 wait states */
        CSCR1 = 0x00000980; /* LCD: 2 wait states */
        while(!(PLLCR & 0x80000000)) {}; /* Wait until the PLL has locked.
                                            This may take up to 10ms! */
        timers_adjust_prescale(CPUFREQ_MAX_MULT, true);
        DCR = (0x8200 | MAX_REFRESH_TIMER);          /* Refresh timer */
        cpu_frequency = CPUFREQ_MAX;
        IDECONFIG1 = 0x106000 | (5 << 10); /* BUFEN2 enable + CS2Pre/CS2Post */
        IDECONFIG2 = 0x40000 | (1 << 8); /* TA enable + CS2wait */
        break;
        
    case CPUFREQ_NORMAL:
        DCR = (DCR & ~0x01ff) | DEFAULT_REFRESH_TIMER;
              /* Refresh timer for bypass frequency */
        PLLCR &= ~1;  /* Bypass mode */
        timers_adjust_prescale(CPUFREQ_DEFAULT_MULT, false);
        PLLCR = 0x16430045;
        CSCR0 = 0x00000580; /* Flash: 1 wait state */
        CSCR1 = 0x00000180; /* LCD: 0 wait states */
        while(!(PLLCR & 0x80000000)) {}; /* Wait until the PLL has locked.
                                            This may take up to 10ms! */
        timers_adjust_prescale(CPUFREQ_NORMAL_MULT, true);
        DCR = (0x8000 | NORMAL_REFRESH_TIMER);       /* Refresh timer */
        cpu_frequency = CPUFREQ_NORMAL;
        IDECONFIG1 = 0x106000 | (5 << 10); /* BUFEN2 enable + CS2Pre/CS2Post */
        IDECONFIG2 = 0x40000 | (0 << 8); /* TA enable + CS2wait */
        break;
    default:
        DCR = (DCR & ~0x01ff) | DEFAULT_REFRESH_TIMER;
              /* Refresh timer for bypass frequency */
        PLLCR &= ~1;  /* Bypass mode */
        timers_adjust_prescale(CPUFREQ_DEFAULT_MULT, true);
        PLLCR = 0x10400200; /* Power down PLL, but keep CLSEL and CRSEL */
        CSCR0 = 0x00000180; /* Flash: 0 wait states */
        CSCR1 = 0x00000180; /* LCD: 0 wait states */
        DCR = (0x8000 | DEFAULT_REFRESH_TIMER);       /* Refresh timer */
        cpu_frequency = CPUFREQ_DEFAULT;
        IDECONFIG1 = 0x106000 | (1 << 10); /* BUFEN2 enable + CS2Pre/CS2Post */
        IDECONFIG2 = 0x40000 | (0 << 8); /* TA enable + CS2wait */
        break;
    }
}
