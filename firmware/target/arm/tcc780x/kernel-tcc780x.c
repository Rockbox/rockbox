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
#include "system.h"
#include "kernel.h"
#include "timer.h"
#include "thread.h"

/* NB: PCK_TCT must previously have been set to 2Mhz by calling clock_init() */
void tick_start(unsigned int interval_in_ms)
{
    /* disable Timer0 */
    TCFG0 &= ~1;

    /* set counter reference value based on 1Mhz tick */
    TREF0 = interval_in_ms * 1000;

    /* Timer0 = reset to 0, divide=2, IRQ enable, enable (continuous) */
    TCFG0 = (1<<8) | (0<<4) | (1<<3) | 1;

    /* Unmask timer IRQ */
    MIRQ &= ~TIMER_IRQ_MASK;
}

/* NB: Since the 7801 has a single timer IRQ, the tick tasks are dispatched
   as part of the central timer IRQ processing in timer-tcc780x.c */
