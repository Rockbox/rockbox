/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Karl Kurbjun
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
    /* set GIO17 (ATA power) on and output */
    ide_power_enable(true);
    IO_GIO_DIR1&=~(1<<1);
    /* Charger detect */
}

bool charger_inserted(void)
{
    return false;
}

/* Returns true if the unit is charging the batteries. */
bool charging_state(void) {
    return false;
}

void ide_power_enable(bool on)
{
    if (on)
        IO_GIO_BITCLR1=(1<<1);
    else
        IO_GIO_BITSET1=(1<<1);
}

bool ide_powered(void)
{
    return !(IO_GIO_BITSET1&(1<<1));
}

void power_off(void)
{
    /* turn off backlight and wait for 1 second */
    _backlight_off();
    sleep(HZ);
    /* Hard shutdown */
    IO_GIO_BITSET1|=1<<10;
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

