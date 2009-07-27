/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: gwps-common.h 20492 2009-03-23 17:19:48Z alle $
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
#ifndef _MUSICSCREEN_H_
#define _MUSICSCREEN_H_
#include <stdbool.h>
 
long gui_wps_show(void);
 
 
void gui_sync_wps_init(void);

/* fades the volume, e.g. on pause or stop */
void fade(bool fade_in, bool updatewps);

bool ffwd_rew(int button);
void display_keylock_text(bool locked);

bool is_wps_fading(void);
#endif
