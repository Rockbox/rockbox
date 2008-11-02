/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2008 by Frank Gevaerts
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
#include "config.h" /* for HAVE_MULTIVOLUME or not */
#include "storage.h"

#ifdef SIMULATOR
void storage_enable(bool on)
{
    (void)on;
}
void storage_sleep(void)
{
}
void storage_sleepnow(void)
{
}
bool storage_disk_is_active(void)
{
    return 0;
}
int storage_hard_reset(void)
{
    return 0;
}
int storage_soft_reset(void)
{
    return 0;
}
int storage_init(void)
{
    return 0;
}
void storage_close(void)
{
}
int storage_read_sectors(int drive, unsigned long start, int count, void* buf)
{
    (void)drive;
    (void)start;
    (void)count;
    (void)buf;
    return 0;
}
int storage_write_sectors(int drive, unsigned long start, int count, const void* buf)
{
    (void)drive;
    (void)start;
    (void)count;
    (void)buf;
    return 0;
}

void storage_spin(void)
{
}
void storage_spindown(int seconds)
{
    (void)seconds;
}
 
#if (CONFIG_LED == LED_REAL)
void storage_set_led_enabled(bool enabled)
{
    (void)enabled;
}
#endif /*LED*/

long storage_last_disk_activity(void)
{
    return 0;
}

int storage_spinup_time(void)
{
    return 0;
}

#ifdef STORAGE_GET_INFO
void storage_get_info(int drive, struct storage_info *info)
{
    (void)drive;
    (void)info;
}
#endif

#ifdef HAVE_HOTSWAP
bool storage_removable(int drive)
{
    (void)drive;
    return 0;
}

bool storage_present(int drive)
{
    (void)drive;
    return 0;
}
#endif /* HOTSWAP */

#endif
