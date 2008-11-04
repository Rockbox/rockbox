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
#ifndef __MMC_H__
#define __MMC_H__

#include <stdbool.h>
#include "mv.h" /* for HAVE_MULTIVOLUME or not */

struct storage_info;

void mmc_enable(bool on);
void mmc_spindown(int seconds);
void mmc_sleep(void);
void mmc_sleepnow(void);
bool mmc_disk_is_active(void);
int mmc_soft_reset(void);
int mmc_init(void);
void mmc_close(void);
int mmc_read_sectors(IF_MV2(int drive,) unsigned long start, int count, void* buf);
int mmc_write_sectors(IF_MV2(int drive,) unsigned long start, int count, const void* buf);
void mmc_spin(void);
int mmc_spinup_time(void);

#if (CONFIG_LED == LED_REAL)
void mmc_set_led_enabled(bool enabled);
#endif

#ifdef STORAGE_GET_INFO
void mmc_get_info(IF_MV2(int drive,) struct storage_info *info);
#endif
#ifdef HAVE_HOTSWAP
bool mmc_removable(IF_MV_NONVOID(int drive));
bool mmc_present(IF_MV_NONVOID(int drive));
#endif

long mmc_last_disk_activity(void);

#endif
