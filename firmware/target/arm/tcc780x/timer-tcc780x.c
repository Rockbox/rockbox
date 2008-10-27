/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2008 by Rob Purchase
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
#include "system.h"
#include "timer.h"
#include "logf.h"

/* Use the TC32 counter [sourced by Xin:12Mhz] for this timer, as it's the
   only one that allows a 32-bit counter (Timer0-5 are 16/20 bit only). */

bool __timer_set(long cycles, bool start)
{
    (void)cycles;
    (void)start;
    return false;
}

bool __timer_register(void)
{
    return false;
}

void __timer_unregister(void)
{
}


/* Timer interrupt processing - all timers (inc. tick) have a single IRQ */
void TIMER0(void)
{
    if (TIREQ & TF0)    /* Timer0 reached ref value */
    {
        /* Run through the list of tick tasks */
        call_tick_tasks();
        
        /* reset Timer 0 IRQ & ref flags */
        TIREQ |= TI0 | TF0;
    }

    if (TC32IRQ & (1<<3))    /* end of TC32 prescale */
    {
        /* dispatch timer */
    }
}
