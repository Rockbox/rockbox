/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include <stdbool.h>
#include "cpu.h"
#include "led.h"
#include "system.h"
#include "kernel.h"

#if CONFIG_LED == LED_REAL

void led(bool on)
{
    if ( on )
#ifdef GMINI_ARCH
        P2 |= 1;
    else
        P2 &= ~1;
#else
    {
        or_b(0x40, &PBDRL);
    }
    else
    {
        and_b(~0x40, &PBDRL);
    }
#endif
}

#elif (CONFIG_LED == LED_VIRTUAL) || defined(HAVE_REMOTE_LCD)

static bool current;
static long last_on; /* timestamp of switching off */

void led(bool on)
{
    if (current && !on) /* switching off */
    {
        last_on = current_tick; /* remember for off delay */
    }
    current = on;
}

bool led_read(int delayticks) /* read by status bar update */
{
    /* reading "off" is delayed by user-supplied monoflop value */
    return (current || TIME_BEFORE(current_tick, last_on+delayticks));
}

#else

void led(bool on)
{
    (void)on;
}
#endif /* CONFIG_LED, HAVE_REMOTE_LCD */
