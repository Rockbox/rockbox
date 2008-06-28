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
#ifndef _GWPS_COMMON_
#define _GWPS_COMMON_
#include <stdbool.h>
#include <sys/types.h> /* for size_t */

#include "gwps.h"

void fade(bool fade_in, bool updatewps);
bool gui_wps_display(void);
bool update_onvol_change(struct gui_wps * gwps);
bool update(struct gui_wps *gwps);
void play_hop(int direction);
bool ffwd_rew(int button);
void display_keylock_text(bool locked);

bool gui_wps_refresh(struct gui_wps *gwps,
                     int ffwd_offset,
                     unsigned char refresh_mode);
#endif
