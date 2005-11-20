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

static bool disk_led_status;
static long last_on; /* timestamp of switching off */



void disk_led_on(void)
{
    disk_led_status=true;
#if CONFIG_LED == LED_REAL
#ifdef GMINI_ARCH
    P2 |= 1;
#else
    or_b(0x40, &PBDRL);
#endif
#endif
}

void disk_led_off(void)
{
    if(disk_led_status)
    {
        last_on = current_tick;/* remember for off delay */
        disk_led_status=false;
#if CONFIG_LED == LED_REAL
#ifdef GMINI_ARCH
        P2 &= ~1;
#else
        and_b(~0x40, &PBDRL);
#endif
#endif
    }
}

void led(bool on)
{
    if ( on )
        disk_led_on();
    else
        disk_led_off();
}

bool led_read(int delayticks)
{
    /* reading "off" is delayed by user-supplied monoflop value */
    return (disk_led_status ||
        TIME_BEFORE(current_tick, last_on+delayticks));
}
