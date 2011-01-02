/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id: timer-s5l8700.c 23103 2009-10-11 11:35:14Z theseven $
*
* Copyright (C) 2009 Bertrik Sikken
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
#include "s5l8702.h"
#include "system.h"
#include "timer.h"

//TODO: This needs calibration once we figure out the clocking

void INT_TIMERC(void)
{
    /* clear interrupt */
    TCCON = TCCON;
    
    if (pfn_timer != NULL) {
        pfn_timer();
    }
}

bool timer_set(long cycles, bool start)
{
    static const int cs_table[] = {1, 2, 4, 6};
    int prescale, cs;
    long count;

    /* stop and clear timer */
    TCCMD = (1 << 1);   /* TD_CLR */

    /* optionally unregister any previously registered timer user */
    if (start) {
        if (pfn_unregister != NULL) {
            pfn_unregister();
            pfn_unregister = NULL;
        }
    }

    /* scale the count down with the clock select */
    for (cs = 0; cs < 4; cs++) {
        count = cycles >> cs_table[cs];
        if ((count < 65536) || (cs == 3)) {
            break;
        }
    }
    
    /* scale the count down with the prescaler */
    prescale = 1;
    while (count >= 65536) {
        count >>= 1;
        prescale <<= 1;
    }

    /* configure timer */
    TCCON = (1 << 12) |     /* TD_INT0_EN */
            (cs << 8) |     /* TS_CS */
            (0 << 4);       /* TD_MODE_SEL, 0 = interval mode */
    TCPRE = prescale - 1;
    TCDATA0 = count;
    TCCMD = (1 << 0);       /* TD_ENABLE */
    
    return true;
}

bool timer_start(void)
{
    TCCMD = (1 << 0);       /* TD_ENABLE */
    return true;
}

void timer_stop(void)
{
    TCCMD = (0 << 0);       /* TD_ENABLE */
}

