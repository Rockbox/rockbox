/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Marcin Bukat
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
#include "cpu.h"
#include <stdbool.h>
#include "kernel.h"
#include "system.h"
#include "power.h"
#include "ata-driver.h"

void ata_reset(void)
{
    /* GPIO19 */
    and_l(~(1<<19), &GPIO_OUT);
    sleep(1); /* > 25us */

    or_l((1<<19), &GPIO_OUT);
    sleep(1); /* > 25us */
}

void ata_enable(bool on)
{
#ifndef BOOTLOADER
    static bool init = true;
#endif

    /* Ide power toggling is a nasty hack to allow USB bridge operation
     * in rockbox. For some reason GL811E bridge doesn't like the state
     * in which rockbox leaves drive (and vice versa). The only way
     * I found out to recover is to do disk power cycle (I tried toggling
     * reset line of the disk but it doesn't work).
     */

    /* GPO36  /reset line of GL811E */
    if (on)
    {
        and_l(~(1<<4), &GPIO1_OUT);
#ifndef BOOTLOADER
        if ( !init )
        {
            ide_power_enable(false);
            sleep(1);
            ide_power_enable(true);
        }
        init = false;
#endif
    }
    else
    {
#ifndef BOOTLOADER
        ide_power_enable(false);
        sleep(1);
        ide_power_enable(true);
#endif
        or_l((1<<4), &GPIO1_OUT);
    }
    or_l((1<<4), &GPIO1_ENABLE);
    or_l((1<<4), &GPIO1_FUNCTION);
}

bool ata_is_coldstart(void)
{
    /* check if ATA reset line is configured
     * as GPIO
     */
    return (GPIO_FUNCTION & (1<<19)) == 0;
}

void ata_device_init(void)
{
    /* ATA reset line config */
    or_l((1<<19), &GPIO_OUT);
    or_l((1<<19), &GPIO_ENABLE);
    or_l((1<<19), &GPIO_FUNCTION);
}
