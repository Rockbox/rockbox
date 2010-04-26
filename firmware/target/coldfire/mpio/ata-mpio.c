/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
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
#include "ata-target.h"

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
    (void)on;
}

/* to be fixed */
bool ata_is_coldstart(void)
{
    return true;
}

void ata_device_init(void)
{
    /* ATA reset line config */
    or_l((1<<19), &GPIO_OUT);
    or_l((1<<19), &GPIO_ENABLE);
    or_l((1<<19), &GPIO_FUNCTION);
}
