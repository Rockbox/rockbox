/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "cpu.h"
#include <stdbool.h>
#include "kernel.h"
#include "system.h"
#include "power.h"
#include "pcf50606.h"
#include "backlight.h"
#include "backlight-target.h"

#ifndef SIMULATOR

void power_init(void)
{
    /* Initialize IDE power pin */
    GPGCON=( GPGCON&~(1<<23) ) | (1<<22); /* Make the pin an output */
    GPGUP |= 1<<11;  /* Disable pullup in SOC as we are now driving */
    ide_power_enable(true);
    /* Charger detect */
}

bool charger_inserted(void)
{
    return (GPFDAT & (1 << 4)) ? false : true;
}

/* Returns true if the unit is charging the batteries. */
bool charging_state(void) {
    return (GPGDAT & (1 << 8)) ? false : true;
}

void ide_power_enable(bool on)
{
    if (on)
        GPGDAT |= (1 << 11);
    else
        GPGDAT &= ~(1 << 11);
}

bool ide_powered(void)
{
    return (GPGDAT & (1 << 11)) != 0;
}

void power_off(void)
{
    /* turn off backlight and wait for 1 second */
    _backlight_off();
    _buttonlight_off();
    sleep(HZ);
    /* set SLEEP bit to on in CLKCON to turn off */
    CLKCON |=(1<<3);
}

#else /* SIMULATOR */

bool charger_inserted(void)
{
    return false;
}

void charger_enable(bool on)
{
    (void)on;
}

void power_off(void)
{
}

void ide_power_enable(bool on)
{
   (void)on;
}

#endif /* SIMULATOR */

