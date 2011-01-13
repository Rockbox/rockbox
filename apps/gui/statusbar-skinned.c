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

#include "action.h"
#include "system.h"
#include "settings.h"
#include "appevents.h"
#include "screens.h"
#include "screen_access.h"
#include "strlcpy.h"
#include "skin_parser.h"
#include "skin_buffer.h"
#include "skin_engine/skin_engine.h"
#include "skin_engine/wps_internals.h"
#include "viewport.h"
#include "statusbar.h"
#include "statusbar-skinned.h"
#include "debug.h"
#include "font.h"
#include "icon.h"
#include "option_select.h"
#ifdef HAVE_TOUCHSCREEN
#include "sound.h"
#include "misc.h"
#endif

/* initial setup of wps_data  */
static int update_delay = DEFAULT_UPDATE_DELAY;

static bool sbs_has_title[NB_SCREENS];
static char* sbs_title[NB_SCREENS];
static enum themable_icons sbs_icon[NB_SCREENS];

bool sb_set_title_text(char* title, enum themable_icons icon, enum screen_type screen)
{
    sbs_title[screen] = title;
    /* Icon_NOICON == -1 which the skin engine wants at position 1, so + 2 */
    sbs_icon[screen] = icon + 2;
    return sbs_has_title[screen];
}

void sb_skin_has_title(enum screen_type screen)
{
    sbs_has_title[screen] = true;
}

const char* sb_get_title(enum screen_type screen)
{
    return sbs_has_title[screen] ? sbs_title[screen] : NULL;
}
enum themable_icons sb_get_icon(enum screen_type screen)
{
    return sbs_has_title[screen] ? sbs_icon[screen] : Icon_NOICON + 2;
}
    
int sb_preproccess(enum screen_type screen, struct wps_data *data)
{
    (void)data;
    int i;
    FOR_NB_SCREENS(i)
        sbs_has_title[i] = false;
    viewportmanager_theme_enable(screen, false, NULL);
    return 1;
}
int sb_postproccess(enum screen_type screen, struct wps_data *data)
{
    if (data->wps_loaded)
    {  
        /* hide the sb's default viewport because it has nasty effect with stuff
        * not part of the statusbar,
        * hence .sbs's without any other vps are unsupported*/
        struct skin_viewport *vp = find_viewport(VP_DEFAULT_LABEL, false, data);
        struct skin_element *next_vp = data->tree->next;
        
        if (vp)
        {
            if (!next_vp)
            {    /* no second viewport, let parsing fail */
                return 0;
            }
            /* hide this viewport, forever */
            vp->hidden_flags = VP_NEVER_VISIBLE;
        }
        sb_set_info_vp(screen, VP_DEFAULT_LABEL);
    }
    viewportmanager_theme_undo(screen, false);
    return 1;
}

static char *infovp_label[NB_SCREENS];
static char *oldinfovp_label[NB_SCREENS];
void sb_set_info_vp(enum screen_type screen, char *label)
{
    infovp_label[screen] = label;
}
    
struct viewport *sb_skin_get_info_vp(enum screen_type screen)
{
    struct wps_data *data = skin_get_gwps(CUSTOM_STATUSBAR, screen)->data;
    if (oldinfovp_label[screen] &&
        strcmp(oldinfovp_label[screen], infovp_label[screen]))
    {
        /* UI viewport changed, so force a redraw */
        oldinfovp_label[screen] = infovp_label[screen];
        viewportmanager_theme_enable(screen, false, NULL);
        viewportmanager_theme_undo(screen, true);
    }        
    return &find_viewport(infovp_label[screen], true, data)->vp;
}

#if (LCD_DEPTH > 1) || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)
int sb_get_backdrop(enum screen_type screen)
{
    struct wps_data *data = skin_get_gwps(CUSTOM_STATUSBAR, screen)->data;
    if (data->wps_loaded)
        return data->backdrop_id;
    else
        return -1;
}
        
#endif
void sb_skin_update(enum screen_type screen, bool force)
{
    struct wps_data *data = skin_get_gwps(CUSTOM_STATUSBAR, screen)->data;
    static long next_update[NB_SCREENS] = {0};
    int i = screen;
    if (!data->wps_loaded)
        return;
    if (TIME_AFTER(current_tick, next_update[i]) || force)
    {
#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
        /* currently, all remotes are readable without backlight
         * so still update those */
        if (lcd_active() || (i != SCREEN_MAIN))
#endif
        {
            bool full_update = skin_do_full_update(CUSTOM_STATUSBAR, screen);
            skin_update(CUSTOM_STATUSBAR, screen, force || 
                        full_update ? SKIN_REFRESH_ALL : SKIN_REFRESH_NON_STATIC);
        }
        next_update[i] = current_tick + update_delay; /* don't update too often */
    }
}

void do_sbs_update_callback(void *param)
{
    (void)param;
    /* the WPS handles changing the actual id3 data in the id3 pointers
     * we imported, we just want a full update */
    skin_request_full_update(CUSTOM_STATUSBAR);
    /* force timeout in wps main loop, so that the update is instantly */
    queue_post(&button_queue, BUTTON_NONE, 0);
}

void sb_skin_set_update_delay(int delay)
{
    update_delay = delay;
}

/* This creates and loads a ".sbs" based on the user settings for:
 *  - regular statusbar
 *  - colours
 *  - ui viewport
 *  - backdrop
 */
char* sb_create_from_settings(enum screen_type screen)
{
    static char buf[128];
    char *ptr, *ptr2;
    int len, remaining = sizeof(buf);
    int bar_position = statusbar_position(screen);
    ptr = buf;
    ptr[0] = '\0';
    
    /* setup the inbuilt statusbar */
    if (bar_position != STATUSBAR_OFF)
    {
        int y = 0, height = STATUSBAR_HEIGHT;
        if (bar_position == STATUSBAR_BOTTOM)
        {
            y = screens[screen].lcdheight - STATUSBAR_HEIGHT;
        }
        len = snprintf(ptr, remaining, "%%V(0,%d,-,%d,0)\n%%wi\n", 
                       y, height);
        remaining -= len;
        ptr += len;
    }
    /* %Vi viewport, colours handled by the parser */
#if NB_SCREENS > 1
    if (screen == SCREEN_REMOTE)
        ptr2 = global_settings.remote_ui_vp_config;
    else
#endif
        ptr2 = global_settings.ui_vp_config;
    
    if (ptr2[0] && ptr2[0] != '-') /* from ui viewport setting */
    {
        char *comma = ptr;
        int param_count = 0;
        len = snprintf(ptr, remaining, "%%ax%%Vi(-,%s)\n", ptr2);
        /* The config put the colours at the end of the viewport,
         * they need to be stripped for the skin code though */
        do {
            param_count++;
            comma = strchr(comma+1, ',');
            
        } while (comma && param_count < 6);
        if (comma)
        {
            char *end = comma;
            char fg[8], bg[8];
            int i = 0;
            comma++;
            while (*comma != ',')
                fg[i++] = *comma++;
            fg[i] = '\0'; comma++; i=0;
            while (*comma != ')')
                bg[i++] = *comma++;
            bg[i] = '\0'; 
            len += snprintf(end, remaining-len, ") %%Vf(%s) %%Vb(%s)\n", fg, bg);
        }       
    }
    else
    {
        int y = 0, height;
        switch (bar_position)
        {
            case STATUSBAR_TOP:
                y = STATUSBAR_HEIGHT;
            case STATUSBAR_BOTTOM:
                height = screens[screen].lcdheight - STATUSBAR_HEIGHT;
                break;
            default:
                height = screens[screen].lcdheight;
        }
        len = snprintf(ptr, remaining, "%%ax%%Vi(-,0,%d,-,%d,1)\n", 
                       y, height);
    }
    return buf;
}

void sb_skin_init(void)
{
    int i;
    FOR_NB_SCREENS(i)
    {
        oldinfovp_label[i] = NULL;
    }
}

#ifdef HAVE_TOUCHSCREEN
static bool bypass_sb_touchregions = true;
void sb_bypass_touchregions(bool enable)
{
    bypass_sb_touchregions = enable;
}

int sb_touch_to_button(int context)
{
    struct touchregion *region;
    static int last_context = -1;
    int button, offset;
    if (bypass_sb_touchregions)
        return ACTION_TOUCHSCREEN;
    
    if (last_context != context)
        skin_disarm_touchregions(skin_get_gwps(CUSTOM_STATUSBAR, SCREEN_MAIN)->data);
    last_context = context;
    button = skin_get_touchaction(skin_get_gwps(CUSTOM_STATUSBAR, SCREEN_MAIN)->data,
                                  &offset, &region);
    
    switch (button)
    {
#ifdef HAVE_VOLUME_IN_LIST
        case ACTION_WPS_VOLUP:
            return ACTION_LIST_VOLUP;
        case ACTION_WPS_VOLDOWN:
            return ACTION_LIST_VOLDOWN;
#endif
        case ACTION_SETTINGS_INC:
        case ACTION_SETTINGS_DEC:
        {
            const struct settings_list *setting = region->data;
            option_select_next_val(setting, button == ACTION_SETTINGS_DEC, true);
        }
        return ACTION_REDRAW;
        case ACTION_TOUCH_MUTE:
        {
            const int min_vol = sound_min(SOUND_VOLUME);
            if (global_settings.volume == min_vol)
                global_settings.volume = region->value;
            else
            {
                region->value = global_settings.volume;
                global_settings.volume = min_vol;
            }
            setvol();
        }
        return ACTION_REDRAW;
        /* TODO */
    }
    return button;
}
#endif
