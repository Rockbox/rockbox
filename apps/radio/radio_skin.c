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
#include "tuner.h"
#include "action.h"
#include "appevents.h"
#include "statusbar-skinned.h"
#include "option_select.h"
#ifdef HAVE_TOUCHSCREEN
#include "sound.h"
#include "misc.h"
#endif


char* default_radio_skin(enum screen_type screen)
{
    (void)screen;
    static char default_fms[] = 
        "%s%?Ti<%Ti. |>%?Tn<%Tn|%Tf>\n"
        "%Sx(Station:) %tf MHz\n"
        "%?St(force fm mono)<%Sx(Force Mono)|%?ts<%Sx(Stereo)|%Sx(Mono)>>\n"
        "%Sx(Mode:) %?tm<%Sx(Scan)|%Sx(Preset)>\n"
#ifdef HAVE_RADIO_RSSI
        "%Sx(Signal strength:) %tr dBuV\n"
#endif
#if CONFIG_CODEC != SWCODEC && !defined(SIMULATOR)
        "%?Rr<%Sx(Time:) %Rh:%Rn:%Rs|%?St(prerecording time)<%pm|%Sx(Prerecord Time) %Rs>>\n"
#endif
        "%pb\n"
#ifdef HAVE_RDS_CAP
        "\n%s%ty\n"
        "%s%tz\n"
#endif
        ;
    return default_fms;
}

void fms_fix_displays(enum fms_exiting toggle_state)
{
    int i;
    FOR_NB_SCREENS(i)
    {
        struct wps_data *data = skin_get_gwps(FM_SCREEN, i)->data;
        if (toggle_state == FMS_ENTER)
        {
            viewportmanager_theme_enable(i, skin_has_sbs(i, data), NULL);  
#if LCD_DEPTH > 1 || defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
            skin_backdrop_show(data->backdrop_id);
#endif
            screens[i].clear_display();
            /* force statusbar/skin update since we just cleared the whole screen */
            send_event(GUI_EVENT_ACTIONUPDATE, (void*)1);
        }
        else
        {
            screens[i].stop_scroll();
#if LCD_DEPTH > 1 || defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
            skin_backdrop_show(sb_get_backdrop(i));
#endif
            viewportmanager_theme_undo(i, skin_has_sbs(i, data));
        }
#ifdef HAVE_TOUCHSCREEN
        if (i==SCREEN_MAIN && !data->touchregions)
            touchscreen_set_mode(toggle_state == FMS_ENTER ? 
                                 TOUCHSCREEN_BUTTON : global_settings.touch_mode);
#endif
    }
}
        

int fms_do_button_loop(bool update_screen)
{
    int button = skin_wait_for_action(FM_SCREEN, CONTEXT_FM, 
                                      update_screen ? TIMEOUT_NOBLOCK : HZ/5);
#ifdef HAVE_TOUCHSCREEN
    struct touchregion *region;
    int offset;
    if (button == ACTION_TOUCHSCREEN)
        button = skin_get_touchaction(skin_get_gwps(FM_SCREEN, SCREEN_MAIN)->data,
                                      &offset, &region);
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
    }   
#endif
    return button;
}

struct gui_wps *fms_get(enum screen_type screen)
{
    return skin_get_gwps(FM_SCREEN, screen);
}
