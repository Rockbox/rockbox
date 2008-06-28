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
#include "pcf50606.h"

void ata_reset(void)
{
    and_l(~0x00080000, &GPIO_OUT);
    sleep(1); /* > 25us */

    or_l(0x00080000, &GPIO_OUT);
    sleep(1); /* > 25us */
}

void ata_enable(bool on)
{
    if(on)
        and_l(~0x00040000, &GPIO_OUT);
    else
        or_l(0x00040000, &GPIO_OUT);

    or_l(0x00040000, &GPIO_ENABLE);
    or_l(0x00040000, &GPIO_FUNCTION);
}

bool ata_is_coldstart(void)
{
    return (GPIO_FUNCTION & 0x00080000) == 0;
}

void ata_device_init(void)
{
#ifdef HAVE_ATA_LED_CTRL
    /* Enable disk LED & ISD chip power control */
    and_l(~0x00000240, &GPIO_OUT);
    or_l(  0x00000240, &GPIO_ENABLE);
    or_l(  0x00000200, &GPIO_FUNCTION);
#endif

    /* ATA reset */
    or_l(0x00080000, &GPIO_OUT);
    or_l(0x00080000, &GPIO_ENABLE);
    or_l(0x00080000, &GPIO_FUNCTION);

}
