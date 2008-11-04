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
#ifndef __SD_H__
#define __SD_H__

#include <stdbool.h>
#include "mv.h" /* for HAVE_MULTIVOLUME or not */

struct storage_info;

void sd_enable(bool on);
void sd_spindown(int seconds);
void sd_sleep(void);
bool sd_disk_is_active(void);
int sd_soft_reset(void);
int sd_init(void);
void sd_close(void);
int sd_read_sectors(IF_MV2(int drive,) unsigned long start, int count, void* buf);
int sd_write_sectors(IF_MV2(int drive,) unsigned long start, int count, const void* buf);
void sd_spin(void);

#ifdef STORAGE_GET_INFO
void sd_get_info(IF_MV2(int drive,) struct storage_info *info);
#endif
#ifdef HAVE_HOTSWAP
bool sd_removable(IF_MV_NONVOID(int drive));
bool sd_present(IF_MV_NONVOID(int drive));
#endif

long sd_last_disk_activity(void);

#endif
