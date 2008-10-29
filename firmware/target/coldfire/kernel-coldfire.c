/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Björn Stenberg
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
#include "panic.h"

void tick_start(unsigned int interval_in_ms)
{
    unsigned long count;
    int prescale;

    count = CPU_FREQ/2 * interval_in_ms / 1000 / 16;

    if(count > 0x10000)
    {
        panicf("Error! The tick interval is too long (%d ms)\n",
               interval_in_ms);
        return;
    }

    prescale = cpu_frequency / CPU_FREQ;
    /* Note: The prescaler is later adjusted on-the-fly on CPU frequency
       changes within timer.c */
    
    /* We are using timer 0 */

    TRR0 = (unsigned short)(count - 1); /* The reference count */
    TCN0 = 0; /* reset the timer */
    TMR0 = 0x001d | ((unsigned short)(prescale - 1) << 8); 
           /* restart, CLK/16, enabled, prescaler */

    TER0 = 0xff; /* Clear all events */

    ICR1 = 0x8c; /* Interrupt on level 3.0 */
    IMR &= ~0x200;
}

void TIMER0(void) __attribute__ ((interrupt_handler));
void TIMER0(void)
{
    /* Run through the list of tick tasks */
    call_tick_tasks();

    TER0 = 0xff; /* Clear all events */
}
