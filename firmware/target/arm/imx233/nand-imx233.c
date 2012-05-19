/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#include "system.h"
#include "gpmi-imx233.h"
#include "pinctrl-imx233.h"
#include "button-target.h"
#include "fat.h"
#include "disk.h"
#include "usb.h"
#include "debug.h"
#include "nand.h"
#include "storage.h"

static int nand_first_drive;

int nand_init(void)
{
    return -1;
}
int nand_read_sectors(IF_MD2(int drive,) unsigned long start, int count,
                     void* buf)
{
    return -1;
}

int nand_write_sectors(IF_MD2(int drive,) unsigned long start, int count,
                     const void* buf)
{
    return -1;
}

int nand_num_drives(int first_drive)
{
    nand_first_drive = first_drive;
    return 1;
}

void nand_get_info(IF_MD2(int drive,) struct storage_info *info)
{
    IF_MD((void)drive);
    info->sector_size = SECTOR_SIZE;
    info->num_sectors = 0;
    info->vendor = "";
    info->product = "";
    info->revision = "";
}

/*
bool nand_present(IF_MD(int drive))
{
    IF_MD((void) drive);
    return true;
}

bool nand_removable(IF_MD(int drive))
{
    IF_MD((void) drive);
    return false;
}
*/
