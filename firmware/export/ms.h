/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Mauricio G.
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
#ifndef __MS_H__
#define __MS_H__

#include <stdbool.h>
#include "config.h"
#include "mv.h" /* for HAVE_MULTIDRIVE or not */

struct storage_info;

void ms_enable(bool on);
void ms_spindown(int seconds);
bool ms_disk_is_active(void);
int ms_soft_reset(void);
int ms_init(void) STORAGE_INIT_ATTR;
void ms_close(void);
int ms_read_sectors(IF_MD(int drive,) sector_t start, int count, void* buf);
int ms_write_sectors(IF_MD(int drive,) sector_t start, int count, const void* buf);
void ms_spin(void);
void ms_sleepnow(void);
int ms_spinup_time(void);

#ifdef STORAGE_GET_INFO
void ms_get_info(IF_MD(int drive,) struct storage_info *info);
#endif

long ms_last_disk_activity(void);

#ifdef HAVE_HOTSWAP
bool ms_removable(IF_MD(int drive));
bool ms_present(IF_MD(int drive));
#endif

#endif
