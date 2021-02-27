/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include "timer.h"
#include "x1000/tcu.h"

#define TIMER_CHN 5

bool timer_set(long cycles, bool start)
{
    if(cycles <= 0)
        return false;

    /* Calculate timer interval */
    unsigned long counter = cycles;
    unsigned prescale = 0;
    while(counter > 0xffff && prescale < 5) {
        counter /= 4;
        prescale += 1;
    }

    /* Duration too long */
    if(counter > 0xffff)
        return false;

    /* Unregister old function */
    if(start && pfn_unregister) {
        pfn_unregister();
        pfn_unregister = 0;
    }

    /* Configure the timer */
    jz_clr(TCU_STOP, 1 << TIMER_CHN);
    jz_clr(TCU_ENABLE, 1 << TIMER_CHN);
    jz_overwritef(TCU_CTRL(TIMER_CHN), SOURCE_V(EXT), PRESCALE(prescale));
    jz_write(TCU_CMP_FULL(TIMER_CHN), counter);
    jz_write(TCU_CMP_HALF(TIMER_CHN), 0);
    jz_clr(TCU_FLAG, 1 << TIMER_CHN);
    jz_clr(TCU_MASK, 1 << TIMER_CHN);

    if(start)
        return timer_start();
    else
        return true;
}

bool timer_start(void)
{
    jz_set(TCU_ENABLE, 1 << TIMER_CHN);
    return true;
}

void timer_stop(void)
{
    jz_clr(TCU_ENABLE, 1 << TIMER_CHN);
    jz_set(TCU_MASK, 1 << TIMER_CHN);
    jz_clr(TCU_FLAG, 1 << TIMER_CHN);
    jz_set(TCU_STOP, 1 << TIMER_CHN);
}

void TCU1(void)
{
    jz_clr(TCU_FLAG, 1 << TIMER_CHN);

    if(pfn_timer)
        pfn_timer();
}
