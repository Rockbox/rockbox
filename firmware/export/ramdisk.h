/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr
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
#ifndef __RAMDISK_H__
#define __RAMDISK_H__

#include <stdbool.h>
#include "config.h"
#include "mv.h" /* for HAVE_MULTIDRIVE or not */

struct storage_info;

void ramdisk_enable(bool on);
void ramdisk_spindown(int seconds);
void ramdisk_sleep(void);
bool ramdisk_disk_is_active(void);
int ramdisk_soft_reset(void);
int ramdisk_init(void) STORAGE_INIT_ATTR;
void ramdisk_close(void);
int ramdisk_read_sectors(IF_MD(int drive,) unsigned long start, int count, void* buf);
int ramdisk_write_sectors(IF_MD(int drive,) unsigned long start, int count, const void* buf);
void ramdisk_spin(void);
void ramdisk_sleepnow(void);
int ramdisk_spinup_time(void);

#ifdef STORAGE_GET_INFO
void ramdisk_get_info(IF_MD(int drive,) struct storage_info *info);
#endif

long ramdisk_last_disk_activity(void);

#ifdef CONFIG_STORAGE_MULTI
int ramdisk_num_drives(int first_drive);
#endif

#ifdef HAVE_HOTSWAP
bool ramdisk_removable(IF_MD(int drive));
bool ramdisk_present(IF_MD(int drive));
#endif

#endif
