/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Barry Wardell
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

/* ATA stuff was taken from the iPod code */

#include <stdbool.h>
#include "system.h"
#include "ata-target.h"

void ata_reset() 
{

}

void ata_enable(bool on)
{
    /* TODO: Implement ata_enable() */
    (void)on;
}

bool ata_is_coldstart()
{
    return false;
    /* TODO: Implement coldstart variable */
}

void ata_device_init()
{
    /* From ipod-ide.c:ipod_ide_register() */
    IDE0_CFG |= (1<<5);
#ifdef IPOD_NANO
    IDE0_CFG |= (0x10000000); /* cpu > 65MHz */
#else
    IDE0_CFG &=~(0x10000000); /* cpu < 65MHz */
#endif

    IDE0_PRI_TIMING0 = 0x10;
    IDE0_PRI_TIMING1 = 0x80002150;
}
