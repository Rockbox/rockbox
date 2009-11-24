/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2008 Rafaël Carré
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

#include "as3525.h"
#include "timer.h"
#include "stdlib.h"

void INT_TIMER1(void)
{
    if (pfn_timer != NULL)
        pfn_timer();

    TIMER1_INTCLR = 0; /* clear interrupt */
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
    }

    TIMER1_LOAD = TIMER1_BGLOAD = cycles;
    /* /!\ bit 4 (reserved) must not be modified
     * periodic mode, interrupt enabled, no prescale, 32 bits counter */
    TIMER1_CONTROL = (TIMER1_CONTROL & (1<<4)) |
                     TIMER_ENABLE |
                     TIMER_PERIODIC |
                     TIMER_INT_ENABLE |
                     TIMER_32_BIT;
    return true;
}

bool timer_start(void)
{
    CGU_PERI |= CGU_TIMER1_CLOCK_ENABLE;    /* enable peripheral */
    VIC_INT_ENABLE = INTERRUPT_TIMER1;
    return true;
}

void timer_stop(void)
{
    TIMER1_CONTROL &= 0x10; /* disable timer 1 (don't modify bit 4) */
    VIC_INT_EN_CLEAR = INTERRUPT_TIMER1;  /* disable interrupt */
    CGU_PERI &= ~CGU_TIMER1_CLOCK_ENABLE;   /* disable peripheral */
}
