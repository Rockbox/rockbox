/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Karl Kurbjun
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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

extern void (*tick_funcs[MAX_NUM_TICK_TASKS])(void);

void tick_start(unsigned int interval_in_ms)
{
    IO_TIMER1_TMMD = CONFIG_TIMER1_TMMD_STOP;
    
    /*  Setup the Prescalar (Divide by 10)
     *  Based on linux/include/asm-arm/arch-integrator/timex.h 
     */
    IO_TIMER1_TMPRSCL = 0x0009;

    /* Setup the Divisor */
    IO_TIMER1_TMDIV = (TIMER_FREQ / (10*1000))*interval_in_ms;

    /* Turn Timer1 to Free Run mode */
    IO_TIMER1_TMMD = CONFIG_TIMER1_TMMD_FREE_RUN;

    /* Enable the interrupt */
    IO_INTC_EINT0 |= 1<<IRQ_TIMER1;
}

void TIMER1(void)
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

    IO_INTC_IRQ0 = 1<<IRQ_TIMER1;
}
