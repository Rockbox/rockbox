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
    outl(inl(0xc0003024) | (1 << 7), 0xc0003024);
    outl(inl(0xc0003024) & ~(1<<2), 0xc0003024);

    outl(0x10, 0xc0003000);
    outl(0x80002150, 0xc0003004);
}
