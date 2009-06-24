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
#include "system.h"
#include "kernel.h"
#include "timer.h"
#include "thread.h"

/* NB: PCK_TCT must previously have been set to 2Mhz by calling clock_init() */
void tick_start(unsigned int interval_in_ms)
{
    /* disable Timer0 */
    TCFG(0) &= ~TCFG_EN;

    /* set counter reference value based on 1Mhz tick */
    TREF(0) = interval_in_ms * 1000;

    /* Timer0 = reset to 0, divide=2, IRQ enable, enable (continuous) */
    TCFG(0) = TCFG_CLEAR | (0 << TCFG_SEL) | TCFG_IEN | TCFG_EN;
}


/* Timer interrupt processing - all timers (inc. tick) share a single IRQ */
void TIMER0(void)
{
    if (TIREQ & TIREQ_TF0)    /* Timer0 reached ref value */
    {
        /* Run through the list of tick tasks */
        call_tick_tasks();
        
        /* reset Timer 0 IRQ & ref flags */
        TIREQ = TIREQ_TI0 | TIREQ_TF0;
    }

    if (TIREQ & TIREQ_TF4)    /* Timer4 reached ref value */
    {
        /* dispatch user timer */
        if (pfn_timer != NULL)
            pfn_timer();

        TIREQ = TIREQ_TI4 | TIREQ_TF4;
    }
}
