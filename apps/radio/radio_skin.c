/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Jonathan Gordon
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
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "skin_engine/skin_engine.h"
#include "settings.h"
#include "radio.h"
#include "action.h"
#include "appevents.h"
#include "statusbar-skinned.h"
#include "option_select.h"


extern struct wps_state     wps_state; /* from wps.c */
static struct gui_wps       fms_skin[NB_SCREENS]      = {{ .data = NULL }};
static struct wps_data      fms_skin_data[NB_SCREENS] = {{ .wps_loaded = 0 }};
static struct wps_sync_data fms_skin_sync_data        = { .do_full_update = false };

void fms_data_load(enum screen_type screen, const char *buf, bool isfile)
{
    struct wps_data *data = fms_skin[screen].data;
    int success;
    success = buf && skin_data_load(screen, data, buf, isfile);

    if (!success ) /* load the default */
    {  
        const char default_fms[] =  "%s%?Ti<%Ti. |>%?Tn<%Tn|%Tf>\n"
                                    "%Sx(Station:) %tf MHz\n"
                                    "%?St(force fm mono)<%Sx(Force Mono)|%?ts<%Sx(Stereo)|%Sx(Mono)>>\n"
                                    "%Sx(Mode:) %?tm<%Sx(Scan)|%Sx(Preset)>\n"
#if CONFIG_CODEC != SWCODEC && !defined(SIMULATOR)
                                    "%?Rr<%Sx(Time:) %Rh:%Rn:%Rs|%?St(prerecording time)<%pm|%Sx(Prerecord Time) %Rs>>\n"
#endif
                                    "%pb\n"
#ifdef HAVE_RDS_CAP
                                    "\n%s%ty\n"
                                    "%s%tz\n"
#endif
                                    ;
        skin_data_load(screen, data, default_fms, false);
    }
}
void fms_fix_displays(enum fms_exiting toggle_state)
{
    int i;
    FOR_NB_SCREENS(i)
    {
        if (toggle_state == FMS_ENTER)
        {
            viewportmanager_theme_enable(i, skin_has_sbs(i, fms_skin[i].data), NULL);  
#if LCD_DEPTH > 1 || defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
            screens[i].backdrop_show(fms_skin[i].data->backdrop);
#endif
            screens[i].clear_display();
            /* force statusbar/skin update since we just cleared the whole screen */
            send_event(GUI_EVENT_ACTIONUPDATE, (void*)1);
        }
        else
        {
            screens[i].stop_scroll();
#if LCD_DEPTH > 1 || defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
            screens[i].backdrop_show(sb_get_backdrop(i));
#endif
            viewportmanager_theme_undo(i, skin_has_sbs(i, fms_skin[i].data));
        }
    }
#ifdef HAVE_TOUCHSCREEN
    if (!fms_skin[SCREEN_MAIN].data->touchregions)
        touchscreen_set_mode(toggle_state == FMS_ENTER ? 
                              TOUCHSCREEN_BUTTON : global_settings.touch_mode);
#endif
}
        

void fms_skin_init(void)
{
    int i;
    FOR_NB_SCREENS(i)
    {
#ifdef HAVE_ALBUMART
        fms_skin_data[i].albumart = NULL;
        fms_skin_data[i].playback_aa_slot = -1;
#endif
        fms_skin[i].data = &fms_skin_data[i];
        fms_skin[i].display = &screens[i];
        /* Currently no seperate wps_state needed/possible
           so use the only available ( "global" ) one */
        fms_skin[i].state = &wps_state;
        fms_skin[i].sync_data = &fms_skin_sync_data;
    }
}

int fms_do_button_loop(bool update_screen)
{
    int button = skin_wait_for_action(fms_skin, CONTEXT_FM, 
                                      update_screen ? TIMEOUT_NOBLOCK : HZ/5);
#ifdef HAVE_TOUCHSCREEN
    struct touchregion *region;
    int offset;
    if (button == ACTION_TOUCHSCREEN)
        button = skin_get_touchaction(&fms_skin_data[SCREEN_MAIN], &offset, &region);
    switch (button)
    {
        case ACTION_WPS_STOP:
            return ACTION_FM_STOP;
        case ACTION_STD_CANCEL:
            return ACTION_FM_EXIT;
        case ACTION_WPS_VOLUP:
            return ACTION_SETTINGS_INC;
        case ACTION_WPS_VOLDOWN:
            return ACTION_SETTINGS_DEC;
        case ACTION_WPS_PLAY:
            return ACTION_FM_PLAY;
        case ACTION_STD_MENU:
            return ACTION_FM_MENU;
        case WPS_TOUCHREGION_SCROLLBAR:
            /* TODO */
            break;
        case ACTION_SETTINGS_INC:
        case ACTION_SETTINGS_DEC:
        {
            const struct settings_list *setting = region->extradata;
            option_select_next_val(setting, button == ACTION_SETTINGS_DEC, true);
        }
        return ACTION_REDRAW;
    }   
#endif
    return button;
}

struct gui_wps *fms_get(enum screen_type screen)
{
    return &fms_skin[screen];
}
