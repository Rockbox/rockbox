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

/* fades the volume, e.g. on pause or stop */
void fade(bool fade_in, bool updatewps);

/* Initially display the wps, can fall back to the built-in wps
 * if the chosen wps is invalid.
 *
 * Return true on success, otherwise false */
bool gui_wps_display(struct gui_wps *gui_wps);

/* return true if screen restore is needed
   return false otherwise */
bool update_onvol_change(struct gui_wps * gwps);

/* Update track info related stuff, handle cue sheets as well, and redraw */
bool gui_wps_update(struct gui_wps *gwps);

bool ffwd_rew(int button);
void display_keylock_text(bool locked);

/* Refresh the WPS according to refresh_mode.
 *
 * Return true on success, otherwise false */
bool gui_wps_redraw(struct gui_wps *gwps,
                     int ffwd_offset,
                     unsigned refresh_mode);
#endif
