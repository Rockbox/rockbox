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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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

/* Settings for all possible clock frequencies (with properly working timers)
 * NOTE: Some 5249 chips don't like having PLLDIV set to 0. We must avoid that!
 *
 *                    xxx_REFRESH_TIMER below
 * system.h, CPUFREQ_xxx_MULT    |
 *              |                |
 *              V                V
 *                   PLLCR &   Rftim.         IDECONFIG1/IDECONFIG2
 * CPUCLK/Hz  MULT ~0x70400000  16MB  CSCR0   CS2Pre CS2Post CS2Wait
 * -----------------------------------------------------------------
 *  11289600    1   0x00800200    4   0x0180     1      1       0
 *  22579200    2   0x0589e025   10   0x0180     1      1       0
 *  33868800    3   0x0388e025   15   0x0180     1      1       0
 *  45158400    4   0x0589e021   21   0x0580     1      1       0
 *  56448000    5   0x0289e025   26   0x0580     2      1       0
 *  67737600    6   0x0388e021   32   0x0980     2      1       0
 *  79027200    7   0x038a6021   37   0x0980     2      1       0
 *  90316800    8   0x038be021   43   0x0d80     2      1       0
 * 101606400    9   0x01892025   48   0x0d80     2      1       0
 * 112896000   10   0x0189e025   54   0x1180     3      1       0
 * 124185600   11   0x018ae025   59   0x1180     3      1       1
 */

#define MAX_REFRESH_TIMER     59
#define NORMAL_REFRESH_TIMER  21
#define DEFAULT_REFRESH_TIMER 4

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
void set_cpu_frequency (long) __attribute__ ((section (".icode")));
void set_cpu_frequency(long frequency)
#else
void cf_set_cpu_frequency (long) __attribute__ ((section (".icode")));
void cf_set_cpu_frequency(long frequency)
#endif    
{
    switch(frequency)
    {
    case CPUFREQ_MAX:
        DCR = (0x8200 | DEFAULT_REFRESH_TIMER);
              /* Refresh timer for bypass frequency */
        PLLCR &= ~1;  /* Bypass mode */
        timers_adjust_prescale(CPUFREQ_DEFAULT_MULT, false);
        PLLCR = 0x018ae025 | (PLLCR & 0x70400000);
        CSCR0 = 0x00001180; /* Flash: 4 wait states */
        while(!(PLLCR & 0x80000000)) {}; /* Wait until the PLL has locked.
                                            This may take up to 10ms! */
        timers_adjust_prescale(CPUFREQ_MAX_MULT, true);
        DCR = (0x8200 | MAX_REFRESH_TIMER);          /* Refresh timer */
        cpu_frequency = CPUFREQ_MAX;
        IDECONFIG1 = 0x100000 | (1 << 13) | (3 << 10);
                   /* SBUFEN2 enable | CS2Post | CS2Pre */
        IDECONFIG2 = 0x40000 | (1 << 8); /* TA enable + CS2wait */
        break;

    case CPUFREQ_NORMAL:
        DCR = (DCR & ~0x01ff) | DEFAULT_REFRESH_TIMER;
              /* Refresh timer for bypass frequency */
        PLLCR &= ~1;  /* Bypass mode */
        timers_adjust_prescale(CPUFREQ_DEFAULT_MULT, false);
        PLLCR = 0x0589e021 | (PLLCR & 0x70400000);
        CSCR0 = 0x00000580; /* Flash: 1 wait state */
        while(!(PLLCR & 0x80000000)) {}; /* Wait until the PLL has locked.
                                            This may take up to 10ms! */
        timers_adjust_prescale(CPUFREQ_NORMAL_MULT, true);
        DCR = (0x8000 | NORMAL_REFRESH_TIMER);       /* Refresh timer */
        cpu_frequency = CPUFREQ_NORMAL;
        IDECONFIG1 = 0x100000 | (1 << 13) | (1 << 10);
                    /* BUFEN2 enable | CS2Post | CS2Pre */
        IDECONFIG2 = 0x40000 | (0 << 8); /* TA enable + CS2wait */
        break;

    default:
        DCR = (DCR & ~0x01ff) | DEFAULT_REFRESH_TIMER;
              /* Refresh timer for bypass frequency */
        PLLCR &= ~1;  /* Bypass mode */
        timers_adjust_prescale(CPUFREQ_DEFAULT_MULT, true);
        /* Power down PLL, but keep CRSEL and CLSEL */
        PLLCR = 0x00800200 | (PLLCR & 0x70400000);
        CSCR0 = 0x00000180; /* Flash: 0 wait states */
        DCR = (0x8000 | DEFAULT_REFRESH_TIMER);       /* Refresh timer */
        cpu_frequency = CPUFREQ_DEFAULT;
        IDECONFIG1 = 0x100000 | (1 << 13) | (1 << 10);
                    /* BUFEN2 enable | CS2Post | CS2Pre */
        IDECONFIG2 = 0x40000 | (0 << 8); /* TA enable + CS2wait */
        break;
    }
}
