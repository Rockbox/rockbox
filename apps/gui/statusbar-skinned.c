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

#include "config.h"

#include "system.h"
#include "settings.h"
#include "appevents.h"
#include "screens.h"
#include "screen_access.h"
#include "skin_engine/skin_engine.h"
#include "skin_engine/wps_internals.h"
#include "viewport.h"
#include "statusbar.h"
#include "statusbar-skinned.h"
#include "debug.h"


/* currently only one wps_state is needed */
extern struct wps_state     wps_state; /* from wps.c */
static struct gui_wps       sb_skin[NB_SCREENS]      = {{ .data = NULL }};
static struct wps_data      sb_skin_data[NB_SCREENS] = {{ .wps_loaded = 0 }};
static struct wps_sync_data sb_skin_sync_data        = { .do_full_update = false };

/* initial setup of wps_data  */

static bool loaded_ok[NB_SCREENS] = { false };
static int update_delay = DEFAULT_UPDATE_DELAY;


void sb_skin_data_load(enum screen_type screen, const char *buf, bool isfile)
{
    struct wps_data *data = sb_skin[screen].data;

    int success;
    success = buf && skin_data_load(screen, data, buf, isfile);

    if (success)
    {  /* hide the sb's default viewport because it has nasty effect with stuff
        * not part of the statusbar,
        * hence .sbs's without any other vps are unsupported*/
        struct skin_viewport *vp = find_viewport(VP_DEFAULT_LABEL, data);
        struct skin_token_list *next_vp = data->viewports->next;

        if (!next_vp)
        {    /* no second viewport, let parsing fail */
            success = false;
        }
        /* hide this viewport, forever */
        vp->hidden_flags = VP_NEVER_VISIBLE;
    }

    loaded_ok[screen] = success;
}

/* temporary viewport structs while the non-skinned bar is in the build */
static struct viewport inbuilt[NB_SCREENS];
struct viewport *sb_skin_get_info_vp(enum screen_type screen)
{
    int bar_setting = statusbar_position(screen);
    if (bar_setting == STATUSBAR_CUSTOM)
        return &find_viewport(VP_INFO_LABEL, sb_skin[screen].data)->vp;
    else if (bar_setting == STATUSBAR_OFF)
        return NULL;
    else
    {
        viewport_set_fullscreen(&inbuilt[screen], screen);
        /* WE need to return the UI area.. NOT the statusbar area! */
        if (bar_setting == STATUSBAR_TOP)
            inbuilt[screen].y = STATUSBAR_HEIGHT;
        inbuilt[screen].height -= STATUSBAR_HEIGHT;
        return &inbuilt[screen];
    }
}

inline bool sb_skin_get_state(enum screen_type screen)
{
    /* Temp fix untill the hardcoded bar is removed */
    int bar_setting = global_settings.statusbar;
#if NB_SCREENS > 1
    if (screen == SCREEN_REMOTE)
        bar_setting = global_settings.remote_statusbar;
#endif
    switch (bar_setting)
    {
        case STATUSBAR_CUSTOM:
            return loaded_ok[screen];
        case STATUSBAR_TOP:
        case STATUSBAR_BOTTOM:
           return true;
        case STATUSBAR_OFF:
            return false;
    }
    return false; /* Should never actually get here */
}

void sb_skin_update(enum screen_type screen, bool force)
{
    static long next_update = 0;
    int i = screen;
    if (TIME_AFTER(current_tick, next_update) || force)
    {
        if (sb_skin_get_state(i))
        {
#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
            /* currently, all remotes are readable without backlight
             * so still update those */
            if (lcd_active() || (i != SCREEN_MAIN))
#endif
                skin_update(&sb_skin[i], force?
                        WPS_REFRESH_ALL : WPS_REFRESH_NON_STATIC);
        }
        next_update = current_tick + update_delay; /* don't update too often */
        sb_skin[SCREEN_MAIN].sync_data->do_full_update = false;
    }
}

void do_sbs_update_callback(void *param)
{
    (void)param;
    /* the WPS handles changing the actual id3 data in the id3 pointers
     * we imported, we just want a full update */
    sb_skin_sync_data.do_full_update = true;
    /* force timeout in wps main loop, so that the update is instantly */
    queue_post(&button_queue, BUTTON_NONE, 0);
}

void sb_skin_set_update_delay(int delay)
{
    update_delay = delay;
}


void sb_skin_init(void)
{
    int i;
    FOR_NB_SCREENS(i)
    {
#ifdef HAVE_ALBUMART
        sb_skin_data[i].albumart = NULL;
        sb_skin_data[i].playback_aa_slot = -1;
#endif
        sb_skin[i].data = &sb_skin_data[i];
        sb_skin[i].display = &screens[i];
        /* Currently no seperate wps_state needed/possible
           so use the only available ( "global" ) one */
        sb_skin[i].state = &wps_state;
        sb_skin[i].sync_data = &sb_skin_sync_data;
    }
}
