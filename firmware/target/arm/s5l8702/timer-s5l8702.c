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

void INT_TIMERF(void)
{
    /* clear interrupt */
    TSTAT = (0x07 << 16);

    if (pfn_timer != NULL) {
        pfn_timer();
    }
}

bool timer_set(long cycles, bool start)
{
    /* stop timer */
    TFCMD = (0 << 0);       /* TF_ENABLE */

    /* optionally unregister any previously registered timer user */
    if (start) {
        if (pfn_unregister != NULL) {
            pfn_unregister();
            pfn_unregister = NULL;
        }
    }

    /* There is an odd behaviour when the 32-bit timers are launched
       for the first time, the interrupt status bits are set and an
       unexpected interrupt is generated if they are enabled. A way to
       workaround this is to write the data registers before clearing
       the counter. */
    TFDATA0 = cycles;
    TFCMD = (1 << 1);       /* TF_CLR */

    /* configure timer */
    TFCON = (1 << 12) |     /* TF_INT0_EN */
            (4 << 8) |      /* TF_CS, 4 = ECLK / 1 */
            (1 << 6) |      /* use ECLK (12MHz) */
            (0 << 4);       /* TF_MODE_SEL, 0 = interval mode */
    TFPRE = 0;              /* no prescaler */

    TFCMD = (1 << 0);       /* TF_ENABLE */

    return true;
}

bool timer_start(void)
{
    TFCMD = (1 << 0);       /* TF_ENABLE */
    return true;
}

void timer_stop(void)
{
    TFCMD = (0 << 0);       /* TF_ENABLE */
}
