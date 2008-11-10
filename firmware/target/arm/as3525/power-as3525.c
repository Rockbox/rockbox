/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2008 Rafaël Carré
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
#include "as3525-codec.h"
#include <stdbool.h>

void power_off(void)
{
    /* clear bit 0 of system register */
    ascodec_write(0x20, ascodec_read(0x20) & ~1);

    /* TODO : turn off peripherals properly ? */

    while(1); /* wait for system to shut down */
}

void power_init(void)
{
}

bool charger_inserted(void)
{
    if(ascodec_read(0x25) & (1<<5))
        return true;
    else
        return false;
}

void ide_power_enable(bool on)
{
    (void)on;
}

#if CONFIG_TUNER
bool tuner_power(bool status)
{
    (void)status;
    return false;
}
#endif
