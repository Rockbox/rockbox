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

#include <stdbool.h>
#include "sh7034.h"
#include "led.h"
#include "system.h"

#ifdef HAVE_LED

static bool xor;
static bool current;

void led(bool on)
{
    current = on;
    if ( on ^ xor )
    {
        or_b(0x40, &PBDRL);
    }
    else
    {
        and_b(~0x40, &PBDRL);
    }
}

void invert_led(bool on)
{
    if ( on )
    {
        xor = 1;
    }
    else
    {
        xor = 0;
    }
    led(current);
}

#else /* no LED, just dummies */

void led(bool on)
{
    (void)on;
}

void invert_led(bool on)
{
    (void)on;
}

#endif // #ifdef HAVE_LED
