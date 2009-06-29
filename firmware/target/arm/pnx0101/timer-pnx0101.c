/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2007 Tomasz Malesinski
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
#include "timer.h"

static long cycles_new = 0;

void TIMER1_ISR(void)
{
    if (cycles_new > 0)
    {
        TIMER1.load = cycles_new - 1;
        cycles_new = 0;
    }
    if (pfn_timer != NULL)
    {
        cycles_new = -1;
        /* "lock" the variable, in case timer_set_period()
         * is called within pfn_timer() */
        pfn_timer();
        cycles_new = 0;
    }
    TIMER1.clr = 1; /* clear the interrupt */
}

bool timer_set(long cycles, bool start)
{
    if (start)
    {
        if (pfn_unregister != NULL)
        {
            pfn_unregister();
            pfn_unregister = NULL;
        }
        TIMER1.ctrl &= ~0x80; /* disable the counter */
        TIMER1.ctrl |= 0x40;  /* reload after counting down to zero */
        TIMER1.ctrl &= ~0xc;  /* no prescaler */
        TIMER1.clr = 1;       /* clear an interrupt event */
    }
    if (start || (cycles_new == -1)) /* within isr, cycles_new is "locked" */
    {                                /* enable timer */
        TIMER1.load = cycles - 1;
        TIMER1.ctrl |= 0x80;        /* enable the counter */
    }
    else
        cycles_new = cycles;

    return true;
}

bool timer_start(void)
{
    irq_set_int_handler(IRQ_TIMER1, TIMER1_ISR);
    irq_enable_int(IRQ_TIMER1);
    return true;
}

void timer_stop(void)
{
    TIMER1.ctrl &= ~0x80;  /* disable timer 1 */
    irq_disable_int(IRQ_TIMER1);
}
