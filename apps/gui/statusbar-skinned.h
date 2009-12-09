/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Thomas Martitz
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
#ifndef __STATUSBAR_SKINNED_H__
#define __STATUSBAR_SKINNED_H__

#define DEFAULT_UPDATE_DELAY (HZ/7)

#ifdef HAVE_LCD_BITMAP

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"


void sb_skin_data_load(enum screen_type screen, const char *buf, bool isfile);

/* probably temporary, to shut the classic statusbar up */
bool sb_skin_get_state(enum screen_type screen);
void sb_skin_init(void);
struct viewport *sb_skin_get_info_vp(enum screen_type screen);
void sb_skin_update(enum screen_type screen, bool force);

void sb_skin_set_update_delay(int delay);

#else /* CHARCELL */
#define sb_skin_init()
#define sb_skin_data_load(a,b,c)
#define sb_skin_set_update_delay(a)
#define sb_skin_set_state(a,b)
#define sb_skin_get_state(a)
#endif
void do_sbs_update_callback(void *param);
#endif /* __STATUSBAR_SKINNED_H__ */
