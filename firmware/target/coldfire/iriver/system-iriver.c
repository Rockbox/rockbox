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
#include "pcf50606.h"

/* Settings for all possible clock frequencies (with properly working timers)
 * NOTE: Some 5249 chips don't like having PLLDIV set to 0. We must avoid that!
 *
 *                        xxx_REFRESH_TIMER below
 * system.h, CPUFREQ_xxx_MULT        |
 *              |                    |
 *              V                    V
 *                   PLLCR &    Refreshtim.                         IDECONFIG1/IDECONFIG2
 * CPUCLK/Hz  MULT ~0x70400000  16MB  32MB  CSCR0   CSCR1   CSCR3   CS2Pre CS2Post CS2Wait
 * ---------------------------------------------------------------------------------------
 *  11289600    1   0x00800200    4     1   0x0180  0x0180  0x0180     1      1       0
 *  22579200    2   0x0589e025   10     4   0x0180  0x0180  0x0180     1      1       0
 *  33868800    3   0x0388e025   15     7   0x0180  0x0180  0x0180     1      1       0
 *  45158400    4   0x0589e021   21    10   0x0580  0x0180  0x0580     1      1       0
 *  56448000    5   0x0289e025   26    12   0x0580  0x0580  0x0980     2      1       0
 *  67737600    6   0x0388e021   32    15   0x0980  0x0980  0x0d80     2      1       0
 *  79027200    7   0x038a6021   37    18   0x0980  0x0d80  0x1180     2      1       0
 *  90316800    8   0x038be021   43    21   0x0d80  0x0d80  0x1580     2      1       0
 * 101606400    9   0x01892025   48    23   0x0d80  0x1180  0x1980     2      1       0
 * 112896000   10   0x0189e025   54    26   0x1180  0x1580  0x1d80     3      1       0
 * 124185600   11   0x018ae025   59    29   0x1180  0x1580  0x2180     3      1       1
 */

#if MEM < 32
#define MAX_REFRESH_TIMER     54
#define NORMAL_REFRESH_TIMER  10
#define DEFAULT_REFRESH_TIMER 4
#else
#define MAX_REFRESH_TIMER     26
#define NORMAL_REFRESH_TIMER  4
#define DEFAULT_REFRESH_TIMER 1
#endif

#ifdef HAVE_SERIAL
#define BAUD_RATE 57600
#define BAUDRATE_DIV_DEFAULT (CPUFREQ_DEFAULT/(BAUD_RATE*32*2))
#define BAUDRATE_DIV_NORMAL (CPUFREQ_NORMAL/(BAUD_RATE*32*2))
#define BAUDRATE_DIV_MAX (CPUFREQ_MAX/(BAUD_RATE*32*2))
#endif

static bool pll_initialized = false;

static void init_pll(void)
{
    /* Refresh timer for bypass frequency */
    PLLCR &= ~1;  /* Bypass mode */
    PLLCR = 0x0189e025 | (PLLCR & 0x70400000); /* set 112 MHz */

    /* Wait until the PLL has locked. This may take up to 10ms! */
    while(!(PLLCR & 0x80000000)) {};

    pll_initialized = true;
}


#ifdef HAVE_ADJUSTABLE_CPU_FREQ
void set_cpu_frequency (long) __attribute__ ((section (".icode")));
void set_cpu_frequency(long frequency)
#else
void cf_set_cpu_frequency (long) __attribute__ ((section (".icode")));
void cf_set_cpu_frequency(long frequency)
#endif    
{
    if (!pll_initialized)
        init_pll();

    switch(frequency)
    {
        case CPUFREQ_MAX:
            CSCR0 = 0x00001180; /* Flash: 4 wait states */
            CSCR1 = 0x00001580; /* LCD: 5 wait states */
#if CONFIG_USBOTG == USBOTG_ISP1362
            CSCR3 = 0x00002180; /* USBOTG: 8 wait states */
#endif
#if CONFIG_RTC == RTC_PCF50606
            pcf50606_i2c_recalc_delay(CPUFREQ_MAX);
#endif
            timers_adjust_prescale(CPUFREQ_MAX_MULT, true);
            DCR = (0x8200 | MAX_REFRESH_TIMER);          /* Refresh timer */
            IDECONFIG1 = 0x10100000 | (1 << 13) | (3 << 10);
            /* SRE active on write (H300 USBOTG) | BUFEN2 enable | CS2Post | CS2Pre */
            IDECONFIG2 = 0x40000 | (1 << 8); /* TA enable 2 + CS2wait */

            PLLCR = (PLLCR & ~0x07000000) | (1 << 24); /* set CPUDIV */
            DCR = (0x8200 | MAX_REFRESH_TIMER); /* DRAM refresh timer */
            cpu_frequency = CPUFREQ_MAX;
            break;

        case CPUFREQ_NORMAL:
            PLLCR = (PLLCR & ~0x07000000) | (5 << 24); /* set CPUDIV */
            DCR = (0x8000 | NORMAL_REFRESH_TIMER); /* DRAM refresh timer */
            cpu_frequency = CPUFREQ_MAX;

            CSCR0 = 0x00000580; /* Flash: 1 wait state */
            CSCR1 = 0x00000180; /* LCD: 0 wait states */
#if CONFIG_USBOTG == USBOTG_ISP1362
            CSCR3 = 0x00000580; /* USBOTG: 1 wait state */
#endif
#if CONFIG_RTC == RTC_PCF50606
            pcf50606_i2c_recalc_delay(CPUFREQ_NORMAL);
#endif
            timers_adjust_prescale(CPUFREQ_NORMAL_MULT, true);

            IDECONFIG1 = 0x10100000 | (1 << 13) | (1 << 10);
            /* SRE active on write (H300 USBOTG) | BUFEN2 enable | CS2Post | CS2Pre */
            IDECONFIG2 = 0x40000; /* TA enable 2 */
            break;
            
        default:
            DCR = (DCR & ~0x01ff) | DEFAULT_REFRESH_TIMER;
            /* Refresh timer for bypass frequency */
            PLLCR &= ~1;  /* Bypass mode */
            timers_adjust_prescale(CPUFREQ_DEFAULT_MULT, true);
#if CONFIG_RTC == RTC_PCF50606
            pcf50606_i2c_recalc_delay(CPUFREQ_DEFAULT_MULT);
#endif
            /* Power down PLL, but keep CRSEL and CLSEL */
            PLLCR = 0x00800200 | (PLLCR & 0x70400000);
            CSCR0 = 0x00000180; /* Flash: 0 wait states */
            CSCR1 = 0x00000180; /* LCD: 0 wait states */
#if CONFIG_USBOTG == USBOTG_ISP1362
            CSCR3 = 0x00000180; /* USBOTG: 0 wait states */
#endif
            DCR = (0x8000 | DEFAULT_REFRESH_TIMER);       /* Refresh timer */
            cpu_frequency = CPUFREQ_DEFAULT;
            IDECONFIG1 = 0x10100000 | (1 << 13) | (1 << 10);
            /* SRE active on write (H300 USBOTG) | BUFEN2 enable | CS2Post | CS2Pre */
            IDECONFIG2 = 0x40000; /* TA enable 2 */

            pll_initialized = false;
            break;
    }

#ifdef HAVE_SERIAL
    UBG10 = BAUDRATE_DIV_NORMAL >> 8;
    UBG20 = BAUDRATE_DIV_NORMAL & 0xff;
#endif
}
