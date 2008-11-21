/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
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
#include "ata.h"
#include "ata-sd-target.h"
#include "ata-nand-target.h"
#include "panic.h"

int ata_read_sectors(IF_MV2(int drive,) unsigned long start, int count, void* buf)
{
    switch(drive)
    {
        case 0:
            return nand_read_sectors(start, count, buf);
        case 1:
            return _sd_read_sectors(start, count, buf);
        default:
            panicf("ata_read_sectors: Drive %d unhandled!", drive);
            return -1;
    }
}

int ata_write_sectors(IF_MV2(int drive,) unsigned long start, int count, const void* buf)
{
    switch(drive)
    {
        case 0:
            return nand_write_sectors(start, count, buf);
        case 1:
            return _sd_write_sectors(start, count, buf);
        default:
            panicf("ata_write_sectors: Drive %d unhandled!", drive);
            return -1;
    }
}

int ata_init(void)
{
    if(_sd_init() != 0)
        return -1;
    if(nand_init() != 0)
        return -2;
    
    return 0;
}

void ata_spindown(int seconds)
{
    /* null */
    (void)seconds;
}

bool ata_disk_is_active(void)
{
    /* null */
    return false;
}

void ata_sleep(void)
{
    /* null */
}

void ata_spin(void)
{
    /* null */
}

int ata_hard_reset(void)
{
    /* null */
    return 0;
}

int ata_soft_reset(void)
{
    /* null */
    return 0;
}

void ata_enable(bool on)
{
    /* null - flash controller is enabled/disabled as needed. */
    (void)on;
}
