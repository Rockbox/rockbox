/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: gwps.h 22003 2009-07-22 22:10:25Z kugel $
 *
 * Copyright (C) 2007 Nicolas Pennequin
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
 
 /** Use this for stuff which external code needs to include **/
 
#ifndef _WPS_ENGINE_H
#define _WPS_ENGINE_H
#include <stdbool.h>
#include "wps_internals.h" /* TODO: remove this line.. shoudlnt be needed */


#ifdef HAVE_TOUCHSCREEN
int wps_get_touchaction(struct wps_data *data);
#endif

#ifdef HAVE_ALBUMART
/* gives back if WPS contains an albumart tag */
bool gui_sync_wps_uses_albumart(void);
#endif

/* setup and display a WPS for the first time */
bool gui_wps_display(struct gui_wps *gwps);
/* do a requested redraw */
bool gui_wps_redraw(struct gui_wps *gwps,
                     int ffwd_offset,
                     unsigned refresh_mode);
/* do a partial redraw, or full if required, also do any housekeeping
 * which might be needed */
bool gui_wps_update(struct gui_wps *gwps);
 
#endif
