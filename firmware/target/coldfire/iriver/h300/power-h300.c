/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
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


#if CONFIG_TUNER

bool tuner_power(bool status)
{
    (void)status;
    return true;
}

#endif /* #if CONFIG_TUNER */

#ifndef SIMULATOR

void power_init(void)
{
    or_l(0x00080000, &GPIO1_OUT);
    or_l(0x00080000, &GPIO1_ENABLE);
    or_l(0x00080000, &GPIO1_FUNCTION);

#ifndef BOOTLOADER
    /* The boot loader controls the power */
    ide_power_enable(true);
#endif
    or_l(0x80000000, &GPIO_ENABLE);
    or_l(0x80000000, &GPIO_FUNCTION);
    pcf50606_init();
}


#if CONFIG_CHARGING
bool charger_inserted(void)
{     
    return (GPIO1_READ & 0x00400000)?true:false;
}
#endif /* CONFIG_CHARGING */

/* Returns true if the unit is charging the batteries. */
bool charging_state(void) {
    return (GPIO_READ & 0x00800000)?true:false;
}


void ide_power_enable(bool on)
{
    if(on)
        and_l(~0x80000000, &GPIO_OUT);
    else
        or_l(0x80000000, &GPIO_OUT);
}


bool ide_powered(void)
{
    return (GPIO_OUT & 0x80000000)?false:true;
}


void power_off(void)
{
    set_irq_level(DISABLE_INTERRUPTS);
    and_l(~0x00080000, &GPIO1_OUT);
    asm("halt");
    while(1);
}

#endif /* SIMULATOR */
