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
    int tf_en = TFCMD & (1 << 0);   /* save TF_EN status */

    /* stop timer */
    TFCMD = (0 << 0);       /* TF_EN = disable */

    /* optionally unregister any previously registered timer user */
    if (start) {
        if (pfn_unregister != NULL) {
            pfn_unregister();
            pfn_unregister = NULL;
        }
    }

    /* configure timer */
    TFCON = (1 << 12) |     /* TF_INT0_EN */
            (4 << 8) |      /* TF_CS = ECLK / 1 */
            (1 << 6) |      /* select ECLK (12 MHz) */
            (0 << 4);       /* TF_MODE_SEL = interval mode */
    TFPRE = 0;              /* no prescaler */
    TFDATA0 = cycles;       /* set interval period */

    /* After the configuration, we must write '1' in TF_CLR to
     * initialize the timer (s5l8700 DS):
     *  - Clear the counter register.
     *  - The value of TF_START is set to TF_OUT.
     *  - TF_DATA0 and TF_DATA1 are updated to the internal buffers.
     *  - Initialize the state of the previously captured signal.
     */
    TFCMD = (1 << 1) |      /* TF_CLR = initialize timer */
            (tf_en << 0);   /* TF_EN = restore previous status */

    return true;
}

bool timer_start(void)
{
    TFCMD = (1 << 0);       /* TF_EN = enable */
    return true;
}

void timer_stop(void)
{
    TFCMD = (0 << 0);       /* TF_EN = disable */
}
