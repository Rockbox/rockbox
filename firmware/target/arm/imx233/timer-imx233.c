/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#include "kernel.h"
#include "timrot-imx233.h"
#include "timer.h"

static long timer_cycles = 0;

static void timer_fn(void)
{
    if(pfn_timer)
        pfn_timer();
}

bool timer_set(long cycles, bool start)
{
    timer_stop();
    
    if(start && pfn_unregister)
    {
        pfn_unregister();
        pfn_unregister = NULL;
    }

    timer_cycles = cycles;

    return true;
}

bool timer_start(IF_COP_VOID(int core))
{
    imx233_timrot_setup(TIMER_USER, true, timer_cycles,
        BV_TIMROT_TIMCTRLn_SELECT__32KHZ_XTAL, BV_TIMROT_TIMCTRLn_PRESCALE__DIV_BY_1,
        false, &timer_fn);
    return true;
}

void timer_stop(void)
{
    imx233_timrot_setup(TIMER_USER, false, 0, BV_TIMROT_TIMCTRLn_SELECT__NEVER_TICK,
        BV_TIMROT_TIMCTRLn_PRESCALE__DIV_BY_1, false, NULL);
}
