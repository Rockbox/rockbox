/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 by Aidan MacDonald
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
#include "storage.h"
#include "sdmmc.h"
#include "sd.h"

int sd_init(void)
{
    return 0;
}

void sd_enable(bool on)
{
    (void)on;
}

int sd_event(long id, intptr_t data)
{
    return storage_event_default_handler(id, data, 0, STORAGE_SD);
}

long sd_last_disk_activity(void)
{
    return 0;
}

bool sd_present(IF_MD_NONVOID(int drive))
{
    IF_MD((void)drive);
    return true;
}

bool sd_removable(IF_MD_NONVOID(int card_no))
{
    IF_MD((void)card_no);
    return true;
}

tCardInfo *card_get_info_target(int card_no)
{
    (void)card_no;

    static tCardInfo null_info = {0};
    return &null_info;
}

int sd_read_sectors(IF_MD(int drive,) sector_t start, int count, void *buf)
{
    IF_MD((void)drive);
    (void)start;
    (void)count;
    (void)buf;
    return -1;
}

int sd_write_sectors(IF_MD(int drive,) sector_t start, int count, const void *buf)
{
    IF_MD((void)drive);
    (void)start;
    (void)count;
    (void)buf;
    return -1;
}
