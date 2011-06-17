/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2005 Jens Arnold
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

#include <stdlib.h>

#include "config.h"
#include "system.h"
#include "cpu.h"
#include "timer.h"

static int base_prescale;

void TIMER1(void) __attribute__ ((interrupt_handler));
void TIMER1(void)
{
    if (pfn_timer != NULL)
        pfn_timer();
    TER1 = 0xff; /* clear all events */
}

bool timer_set(long cycles, bool start)
{
    int phi = 0; /* bits for the prescaler */
    int prescale = 1;

    while (cycles > 0x10000)
    {
        prescale <<= 1;
        cycles >>= 1;
    }

    if (prescale > 4096/CPUFREQ_MAX_MULT)
        return false;

    if (prescale > 256/CPUFREQ_MAX_MULT)
    {
        phi = 0x05;      /* prescale sysclk/16, timer enabled */
        prescale >>= 4;
    }
    else
        phi = 0x03;      /* prescale sysclk, timer enabled */

    base_prescale = prescale;
    prescale *= (cpu_frequency / CPU_FREQ);

    if (start)
    {
        if (pfn_unregister != NULL)
        {
            pfn_unregister();
            pfn_unregister = NULL;
        }
        phi &= ~1;       /* timer disabled at start */

        /* If it is already enabled, writing a 0 to the RST bit will clear
           the register, so we clear RST explicitly before writing the real
           data. */
        TMR1 = 0;
    }

    /* We are using timer 1 */
    TMR1 = 0x0018 | (unsigned short)phi | ((unsigned short)(prescale - 1) << 8);
    TRR1 = (unsigned short)(cycles - 1);
    if (start || (TCN1 >= TRR1))
        TCN1 = 0; /* reset the timer */
    TER1 = 0xff;  /* clear all events */

    return true;
}

bool timer_start(void)
{
    ICR2 = 0x90;       /* interrupt on level 4.0 */
    coldfire_imr_mod(0, 1 << 10);
    TMR1 |= 1;         /* start timer */
    return true;
}

void timer_stop(void)
{
    TMR1 = 0;               /* disable timer 1 */
    coldfire_imr_mod(1 << 10, 1 << 10); /* disable interrupt */
}

void timers_adjust_prescale(int multiplier, bool enable_irq)
{
    /* tick timer */
    TMR0 = (TMR0 & 0x00ef)
         | ((unsigned short)(multiplier - 1) << 8)
         | (enable_irq ? 0x10 : 0);

    if (pfn_timer)
    {
        /* user timer */
        int prescale = base_prescale * multiplier;
        TMR1 = (TMR1 & 0x00ef)
             | ((unsigned short)(prescale - 1) << 8)
             | (enable_irq ? 0x10 : 0);
    }
}
