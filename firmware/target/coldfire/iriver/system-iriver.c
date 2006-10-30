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
#include "pcf50606.h"

#if MEM < 32
#define MAX_REFRESH_TIMER     59
#define NORMAL_REFRESH_TIMER  21
#define DEFAULT_REFRESH_TIMER 4
#else
#define MAX_REFRESH_TIMER     29
#define NORMAL_REFRESH_TIMER  10
#define DEFAULT_REFRESH_TIMER 1
#endif

#ifdef IRIVER_H300_SERIES
#define RECALC_DELAYS(f) \
        pcf50606_i2c_recalc_delay(f)
#else
#define RECALC_DELAYS(f)
#endif

#ifdef HAVE_SERIAL
#define BAUD_RATE 57600
#define BAUDRATE_DIV_DEFAULT (CPUFREQ_DEFAULT/(BAUD_RATE*32*2))
#define BAUDRATE_DIV_NORMAL (CPUFREQ_NORMAL/(BAUD_RATE*32*2))
#define BAUDRATE_DIV_MAX (CPUFREQ_MAX/(BAUD_RATE*32*2))
#endif

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
        RECALC_DELAYS(CPUFREQ_MAX);
        PLLCR = 0x11c56005;
        CSCR0 = 0x00001180; /* Flash: 4 wait states */
        CSCR1 = 0x00000980; /* LCD: 2 wait states */
#if CONFIG_USBOTG == USBOTG_ISP1362
        CSCR3 = 0x00002180; /* USBOTG: 8 wait states */
#endif
        while(!(PLLCR & 0x80000000)) {}; /* Wait until the PLL has locked.
                                            This may take up to 10ms! */
        timers_adjust_prescale(CPUFREQ_MAX_MULT, true);
        DCR = (0x8200 | MAX_REFRESH_TIMER);          /* Refresh timer */
        cpu_frequency = CPUFREQ_MAX;
        IDECONFIG1 = 0x10100000 | (1 << 13) | (2 << 10);
        /* SRE active on write (H300 USBOTG) | BUFEN2 enable | CS2Post | CS2Pre */
        IDECONFIG2 = 0x40000 | (2 << 8); /* TA enable + CS2wait */

#ifdef HAVE_SERIAL
        UBG10 = BAUDRATE_DIV_MAX >> 8;
        UBG20 = BAUDRATE_DIV_MAX & 0xff;
#endif
        break;

    case CPUFREQ_NORMAL:
        DCR = (DCR & ~0x01ff) | DEFAULT_REFRESH_TIMER;
              /* Refresh timer for bypass frequency */
        PLLCR &= ~1;  /* Bypass mode */
        timers_adjust_prescale(CPUFREQ_DEFAULT_MULT, false);
        RECALC_DELAYS(CPUFREQ_NORMAL);
        PLLCR = 0x13c5e005;
        CSCR0 = 0x00000580; /* Flash: 1 wait state */
        CSCR1 = 0x00000180; /* LCD: 0 wait states */
#if CONFIG_USBOTG == USBOTG_ISP1362
        CSCR3 = 0x00000580; /* USBOTG: 1 wait state */
#endif
        while(!(PLLCR & 0x80000000)) {}; /* Wait until the PLL has locked.
                                            This may take up to 10ms! */
        timers_adjust_prescale(CPUFREQ_NORMAL_MULT, true);
        DCR = (0x8000 | NORMAL_REFRESH_TIMER);       /* Refresh timer */
        cpu_frequency = CPUFREQ_NORMAL;
        IDECONFIG1 = 0x10100000 | (0 << 13) | (1 << 10);
        /* SRE active on write (H300 USBOTG) | BUFEN2 enable | CS2Post | CS2Pre */
        IDECONFIG2 = 0x40000 | (0 << 8); /* TA enable + CS2wait */

#ifdef HAVE_SERIAL
        UBG10 = BAUDRATE_DIV_NORMAL >> 8;
        UBG20 = BAUDRATE_DIV_NORMAL & 0xff;
#endif
        break;
    default:
        DCR = (DCR & ~0x01ff) | DEFAULT_REFRESH_TIMER;
              /* Refresh timer for bypass frequency */
        PLLCR &= ~1;  /* Bypass mode */
        timers_adjust_prescale(CPUFREQ_DEFAULT_MULT, true);
        RECALC_DELAYS(CPUFREQ_DEFAULT);
        PLLCR = 0x10c00200; /* Power down PLL, but keep CLSEL and CRSEL */
        CSCR0 = 0x00000180; /* Flash: 0 wait states */
        CSCR1 = 0x00000180; /* LCD: 0 wait states */
#if CONFIG_USBOTG == USBOTG_ISP1362
        CSCR3 = 0x00000180; /* USBOTG: 0 wait states */
#endif
        DCR = (0x8000 | DEFAULT_REFRESH_TIMER);       /* Refresh timer */
        cpu_frequency = CPUFREQ_DEFAULT;
        IDECONFIG1 = 0x10100000 | (0 << 13) | (1 << 10);
        /* SRE active on write (H300 USBOTG) | BUFEN2 enable | CS2Post | CS2Pre */
        IDECONFIG2 = 0x40000 | (0 << 8); /* TA enable + CS2wait */

#ifdef HAVE_SERIAL
        UBG10 = BAUDRATE_DIV_DEFAULT >> 8;
        UBG20 = BAUDRATE_DIV_DEFAULT & 0xff;
#endif
        break;
    }
}
