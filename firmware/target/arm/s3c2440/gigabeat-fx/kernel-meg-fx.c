/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2007 by Michael Sevakis
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

void tick_start(unsigned int interval_in_ms)
{
    /*
     * Based on default PCLK of 49.1568MHz - scaling chosen to give
     * remainder-free result for tick interval of 10ms (100Hz)
     * Timer input clock frequency =
     *      fPCLK / {prescaler value+1} / {divider value}
     * TIMER_FREQ = 49156800 / 2
     * 146300 = TIMER_FREQ / 21 / 8
     * 49156800 = 19*11*(7)*7*5*5*(3)*2*2*2*2*2*2
     * 21 = 7*3
     */

    /* stop timer 4 */
    TCON &= ~(1 << 20);
    /* Set the count for timer 4 */
    TCNTB4 = (TIMER_FREQ / TIMER234_PRESCALE / 8) * interval_in_ms / 1000;
    /* Set the the prescaler value for timers 2,3, and 4 */
    TCFG0 = (TCFG0 & ~0xff00) | ((TIMER234_PRESCALE-1) << 8);
    /* DMA mode off, MUX4 = 1/16 */
    TCFG1 = (TCFG1 & ~0xff0000) | 0x030000;
    /* set manual bit */
    TCON |= 1 << 21;
    /* reset manual bit */
    TCON &= ~(1 << 21);
    /* interval mode */
    TCON |= 1 << 22;
    /* start timer 4 */
    TCON |= (1 << 20);

    /* timer 4 unmask interrupts */
    INTMSK &= ~TIMER4_MASK;
}

void TIMER4(void)
{
    /* Run through the list of tick tasks */
    call_tick_tasks();

    SRCPND = TIMER4_MASK;
    INTPND = TIMER4_MASK;
}
