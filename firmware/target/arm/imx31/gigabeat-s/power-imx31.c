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
#include "system.h"
#include "power.h"
#include "backlight.h"
#include "backlight-target.h"
#include "avic-imx31.h"
#include "mc13783.h"

#ifndef SIMULATOR

void power_init(void)
{
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
    (void)on;
}

bool ide_powered(void)
{
    return true;
}

void power_off(void)
{
    mc13783_set(MC13783_POWER_CONTROL0, MC13783_USEROFFSPI);

    disable_interrupt(IRQ_FIQ_STATUS);
    while (1);
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

