/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006-2007 Robert Keevil
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "system.h"
#include "kernel.h"
#include "piezo.h"

static unsigned int duration;
static bool beeping;

void INT_TIMERD(void)
{
    /* clear interrupt */
    TDCON = TDCON;
    if (!(--duration))
    {
        beeping = 0;
        TDCMD = (1 << 1);   /* TD_CLR */
    }
}

void piezo_start(unsigned short cycles, unsigned short periods)
{
#ifndef SIMULATOR
    duration = periods;
    beeping = 1;
    /* configure timer for 100 kHz */
    TDCMD = (1 << 1);   /* TD_CLR */
    TDPRE = 30 - 1;    /* prescaler */
    TDCON = (1 << 13) | /* TD_INT1_EN */
            (0 << 12) | /* TD_INT0_EN */
            (0 << 11) | /* TD_START */
            (2 << 8) |  /* TD_CS = PCLK / 16 */
            (1 << 4);   /* TD_MODE_SEL = PWM mode */
    TDDATA0 = cycles;   /* set interval period */
    TDDATA1 = cycles << 1; /* set interval period */
    TDCMD = (1 << 0);   /* TD_EN */

    /* enable timer interrupt */
    INTMSK |= INTMSK_TIMERD;
#endif
}

void piezo_stop(void)
{
#ifndef SIMULATOR
    TDCMD = (1 << 1);   /* TD_CLR */
#endif
}

void piezo_clear(void)
{
    piezo_stop();
}

bool piezo_busy(void)
{
    return beeping;
}

void piezo_init(void)
{
    beeping = 0;
}

void piezo_button_beep(bool beep, bool force)
{
    if (force)
        while (beeping)
            yield();

    if (!beeping)
    {
        if (beep)
            piezo_start(22, 457);
        else
            piezo_start(40, 4);
    }
}
