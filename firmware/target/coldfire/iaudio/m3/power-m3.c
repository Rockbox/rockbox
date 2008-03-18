/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Jens Arnold
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

#ifndef SIMULATOR

void power_init(void)
{   
    /* Set KEEPACT */
    or_l(0x00040000, &GPIO_OUT); 
    or_l(0x00040000, &GPIO_ENABLE);
    or_l(0x00040000, &GPIO_FUNCTION);

    /* Charger detect */
    and_l(~0x00000020, &GPIO1_ENABLE);
    or_l(0x00000020, &GPIO1_FUNCTION);

#ifndef BOOTLOADER
    /* FIXME: Just disable the multi-colour LED for now. */
    and_l(~0x00000210, &GPIO1_OUT);
    and_l(~0x00008000, &GPIO_OUT);
#endif
}

bool charger_inserted(void)
{     
    return (GPIO1_READ & 0x00000020) == 0;
}

void ide_power_enable(bool on)
{
    if (on)
    {
        or_l(0x00800000, &GPIO_OUT);
    }
    else
    {
        and_l(~0x00800000, &GPIO_OUT);
    }
}

bool ide_powered(void)
{
    return (GPIO_OUT & 0x00800000) != 0;
}

void power_off(void)
{
    lcd_poweroff();
    set_irq_level(DISABLE_INTERRUPTS);
    and_l(~0x00040000, &GPIO_OUT); /* Set KEEPACT low */
    asm("halt");
}

#endif /* SIMULATOR */

bool tuner_power(bool status)
{
    (void)status;
    return true;
}
