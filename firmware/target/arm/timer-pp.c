/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2006 Thom Johansen
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

#include "cpu.h"
#include "system.h"
#include "timer.h"

static long SHAREDBSS_ATTR cycles_new = 0;

void TIMER2(void)
{
    TIMER2_VAL; /* ACK interrupt */
    if (cycles_new > 0)
    {
        TIMER2_CFG = 0xc0000000 | (cycles_new - 1);
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
}

bool timer_set(long cycles, bool start)
{
    if (cycles > 0x20000000 || cycles < 2)
        return false;

    if (start)
    {
        if (pfn_unregister != NULL)
        {
            pfn_unregister();
            pfn_unregister = NULL;
        }
        CPU_INT_DIS = TIMER2_MASK;
        COP_INT_DIS = TIMER2_MASK;
    }
    if (start || (cycles_new == -1))  /* within isr, cycles_new is "locked" */
        TIMER2_CFG = 0xc0000000 | (cycles - 1);    /* enable timer */
    else
        cycles_new = cycles;

    return true;
}

bool timer_start(IF_COP_VOID(int core))
{
    /* unmask interrupt source */
#if NUM_CORES > 1
    if (core == COP)
        COP_INT_EN = TIMER2_MASK;
    else
#endif
        CPU_INT_EN = TIMER2_MASK;
    return true;
}

void timer_stop(void)
{
    TIMER2_CFG = 0;         /* stop timer 2 */
    CPU_INT_DIS = TIMER2_MASK;
    COP_INT_DIS = TIMER2_MASK;
}
