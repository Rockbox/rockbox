/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Nicolas Pennequin
 * Copyright (C) 2009 Jonathan Gordon
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
 
#ifndef _SKIN_ENGINE_H
#define _SKIN_ENGINE_H

#include "skin_buffer.h"

#include "wps_internals.h" /* TODO: remove this line.. shoudlnt be needed */

#ifdef HAVE_TOUCHSCREEN
int wps_get_touchaction(struct wps_data *data);
#endif

/* setup and display a WPS for the first time */
bool gui_wps_display(struct gui_wps *gwps);
 

/* Do a update_type update of the skinned screen */
bool skin_update(struct gui_wps *gwps, unsigned int update_type);

/*
 * setup up the skin-data from a format-buffer (isfile = false)
 * or from a skinfile (isfile = true)
 */
bool skin_data_load(enum screen_type screen, struct wps_data *wps_data,
                    const char *buf, bool isfile);


/* initial setup of wps_data */
void skin_data_init(struct wps_data *wps_data);

/* call this in statusbar toggle handlers if needed */
void skin_statusbar_changed(struct gui_wps*);
#endif
