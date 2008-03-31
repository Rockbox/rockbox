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
    /*
     * Based on default PCLK of 49.1568MHz - scaling chosen to give
     * remainder-free result for tick interval of 10ms (100Hz)
     * Timer input clock frequency =
     *      fPCLK / {prescaler value+1} / {divider value}
     * TIMER_FREQ = 49156800 / 2
     * 13300 = TIMER_FREQ / 231 / 8
     * 49156800 = 19*(11)*(7)*7*5*5*(3)*2*2*2*2*2*2
     * 231 = 11*7*3
     */

    /* stop timer 4 */
    TCON &= ~(1 << 20);
    /* Set the count for timer 4 */
    TCNTB4 = (TIMER_FREQ / 231 / 8) * interval_in_ms / 1000;
    /* Set the the prescaler value for timers 2,3, and 4 */
    TCFG0 = (TCFG0 & ~0xff00) | ((231-1) << 8);
    /* MUX4 = 1/16 */
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

    SRCPND = TIMER4_MASK;
    INTPND = TIMER4_MASK;
}
