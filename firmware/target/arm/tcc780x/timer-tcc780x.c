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
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
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
    #warning function not implemented
    
    (void)cycles;
    (void)start;
    return false;
}

bool __timer_register(void)
{
    #warning function not implemented
    
    return false;
}

void __timer_unregister(void)
{
    #warning function not implemented
}


/* Timer interrupt processing - all timers (inc. tick) have a single IRQ */

extern void (*tick_funcs[MAX_NUM_TICK_TASKS])(void);

void TIMER0(void)
{
    if (TIREQ & TF0)    /* Timer0 reached ref value */
    {
        int i;

        /* Run through the list of tick tasks */
        for(i = 0; i < MAX_NUM_TICK_TASKS; i++)
        {
            if(tick_funcs[i])
            {
                tick_funcs[i]();
            }
        }

        current_tick++;
        
        /* reset Timer 0 IRQ & ref flags */
        TIREQ |= TI0 | TF0;
    }

    if (TC32IRQ & (1<<3))    /* end of TC32 prescale */
    {
        /* dispatch timer */
    }
}
