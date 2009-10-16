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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"

#include "system.h"
#include "settings.h"
#include "appevents.h"
#include "screens.h"
#include "screen_access.h"
#include "skin_engine/skin_engine.h"
#include "skin_engine/wps_internals.h"
#include "debug.h"


/* currently only one wps_state is needed */
extern struct wps_state wps_state;
static struct gui_wps sb_skin[NB_SCREENS];
static struct wps_data sb_skin_data[NB_SCREENS];

/* initial setup of wps_data  */
static void sb_skin_update(void*);
static bool loaded_ok = false;

void sb_skin_data_load(enum screen_type screen, const char *buf, bool isfile)
{

    loaded_ok = buf && skin_data_load(sb_skin[screen].data,
                                        &screens[screen], buf, isfile);


    if (loaded_ok)
        add_event(GUI_EVENT_ACTIONUPDATE, false, sb_skin_update);
    else
        remove_event(GUI_EVENT_ACTIONUPDATE, sb_skin_update);

#ifdef HAVE_REMOVE_LCD
    sb_skin[screen].data->remote_wps = !(screen == SCREEN_MAIN);
#endif
}

void sb_skin_data_init(enum screen_type screen)
{
    skin_data_init(sb_skin[screen].data);
}

bool sb_skin_active(void)
{
    return loaded_ok;
}

void sb_skin_update(void* param)
{
    int i;
    (void)param;
    FOR_NB_SCREENS(i)
    {
        skin_update(&sb_skin[i], wps_state.do_full_update?
                    WPS_REFRESH_ALL : WPS_REFRESH_NON_STATIC);
    }
}

void sb_skin_init(void)
{
    int i;
    FOR_NB_SCREENS(i)
    {
        skin_data_init(&sb_skin_data[i]);
#ifdef HAVE_ALBUMART
        sb_skin_data[i].wps_uses_albumart = 0;
#endif
#ifdef HAVE_REMOTE_LCD
        sb_skin_data[i].remote_wps = (i == SCREEN_REMOTE);
#endif
        sb_skin[i].data = &sb_skin_data[i];
        sb_skin[i].display = &screens[i];
        sb_skin[i].data->debug = true;
        DEBUGF("data in init: %p, debug: %d\n", &sb_skin_data[i], sb_skin_data[i].debug);
        /* Currently no seperate wps_state needed/possible
           so use the only available ( "global" ) one */
        sb_skin[i].state = &wps_state;
    }
#ifdef HAVE_LCD_BITMAP
/*
    add_event(GUI_EVENT_STATUSBAR_TOGGLE, false, statusbar_toggle_handler);
*/
#endif
}

#ifdef HAVE_ALBUMART
bool sb_skin_uses_statusbar(int *width, int *height)
{
    int  i;
    FOR_NB_SCREENS(i) {
        struct gui_wps *gwps = &sb_skin[i];
        if (gwps->data && (gwps->data->wps_uses_albumart != WPS_ALBUMART_NONE))
        {
            if (width)
                *width = sb_skin[0].data->albumart_max_width;
            if (height)
                *height = sb_skin[0].data->albumart_max_height;
            return true;
        }
    }
    return false;
}
#endif
