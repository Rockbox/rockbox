/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Björn Stenberg
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
#ifndef _SCREENS_H_
#define _SCREENS_H_

#include "config.h"
#include "timefuncs.h"
#include "metadata.h"

struct screen;

#if CONFIG_CHARGING || defined(SIMULATOR)
void charging_splash(void);
#endif

#if (CONFIG_STORAGE & STORAGE_MMC)
int mmc_remove_request(void);
#endif

#if CONFIG_RTC
bool set_time_screen(const char* title, struct tm *tm, bool set_date);
#endif

bool browse_id3(struct mp3entry *id3, int playlist_display_index, int playlist_amount);
int  view_runtime(void);

#ifdef HAVE_TOUCHSCREEN
int calibrate(void);
int reset_mapping(void);
#endif

#endif

