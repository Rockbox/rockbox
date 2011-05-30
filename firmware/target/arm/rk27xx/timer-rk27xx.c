/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Marcin Bukat
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

#include "inttypes.h"
#include "system.h"
#include "timer.h"

void INT_TIMER1(void)
{
    /* clear interrupt */
    TMR1CON &= ~(1<<2);

    if (pfn_timer != NULL)
    {
        pfn_timer();
    }
}

bool timer_set(long cycles, bool start)
{
    /* optionally unregister any previously registered timer user */
    if (start)
    {
        if (pfn_unregister != NULL)
        {
            pfn_unregister();
            pfn_unregister = NULL;
        }
    }

    TMR1LR = cycles;
    TMR1CON = (1<<7) | (1<<1); /* periodic, 1/1 */

    /* unmask timer1 interrupt */
    INTC_IMR |= (1<<3);

    /* enable timer1 interrupt */
    INTC_IECR |= (1<<3);

    return true;
}

bool timer_start(void)
{
    TMR1CON |= (1 << 8);       /* timer1 enable */

    return true;
}

void timer_stop(void)
{
    TMR1CON &= ~(1 << 8);       /* timer1 disable */
}

