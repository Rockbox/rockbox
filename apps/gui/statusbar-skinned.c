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


/* currently only one wps_state is needed */
extern struct wps_state     wps_state; /* from wps.c */
static struct gui_wps       sb_skin[NB_SCREENS]      = {{ .data = NULL }};
static struct wps_data      sb_skin_data[NB_SCREENS] = {{ .wps_loaded = 0 }};
static struct wps_sync_data sb_skin_sync_data        = { .do_full_update = false };

/* initial setup of wps_data  */
static int update_delay = DEFAULT_UPDATE_DELAY;
static int set_title_worker(char* title, enum themable_icons icon, 
                            struct wps_data *data, struct skin_element *root)
{
    int retval = 0;
    struct skin_element *element = root;
    while (element)
    {
        struct wps_token *token = NULL;
        if (element->type == CONDITIONAL)
        {
            struct conditional *cond = (struct conditional *)element->data;
            token = cond->token;
        }
        else if (element->type == TAG)
        {
            token = (struct wps_token *)element->data;
        }
        if (token)
        {
            if (token->type == SKIN_TOKEN_LIST_TITLE_TEXT)
            {
                token->value.data = title;
                retval = 1;
            }
            else if (token->type == SKIN_TOKEN_LIST_TITLE_ICON)
            {
                /* Icon_NOICON == -1 which the skin engine wants at position 1, so + 2 */
                token->value.i = icon+2;
            }
            else if (element->params_count)
            {
                int i;
                for (i=0; i<element->params_count; i++)
                {
                    if (element->params[i].type == CODE)
                        retval |= set_title_worker(title, icon, data, 
                                                   element->params[i].data.code);
                }
            }
        }
        if (element->children_count)
        {
            int i;
            for (i=0; i<element->children_count; i++)
                retval |= set_title_worker(title, icon, data, element->children[i]);
        }
        element = element->next;
    }
    return retval;
}

bool sb_set_title_text(char* title, enum themable_icons icon, enum screen_type screen)
{
    bool retval = set_title_worker(title, icon, &sb_skin_data[screen], 
                                   sb_skin_data[screen].tree) > 0;
    return retval;
}
    

void sb_skin_data_load(enum screen_type screen, const char *buf, bool isfile)
{
    struct wps_data *data = sb_skin[screen].data;

    int success;
    /* We need to disable the theme here or else viewport_set_defaults() 
     * (which is called in the viewport tag parser) will crash because
     * the theme is enabled but sb_set_info_vp() isnt set untill after the sbs
     * is parsed. This only affects the default viewport which is ignored
     * int he sbs anyway */
    viewportmanager_theme_enable(screen, false, NULL);
    success = buf && skin_data_load(screen, data, buf, isfile);

    if (success)
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
                success = false;
            }
            /* hide this viewport, forever */
            vp->hidden_flags = VP_NEVER_VISIBLE;
        }
        sb_set_info_vp(screen, VP_DEFAULT_LABEL);
    }

    if (!success && isfile)
        sb_create_from_settings(screen);
    viewportmanager_theme_undo(screen, false);
}
static char *infovp_label[NB_SCREENS];
static char *oldinfovp_label[NB_SCREENS];
void sb_set_info_vp(enum screen_type screen, char *label)
{
    infovp_label[screen] = label;
}
    
struct viewport *sb_skin_get_info_vp(enum screen_type screen)
{
    if (oldinfovp_label[screen] &&
        strcmp(oldinfovp_label[screen], infovp_label[screen]))
    {
        /* UI viewport changed, so force a redraw */
        oldinfovp_label[screen] = infovp_label[screen];
        viewportmanager_theme_enable(screen, false, NULL);
        viewportmanager_theme_undo(screen, true);
    }        
    return &find_viewport(infovp_label[screen], true, sb_skin[screen].data)->vp;
}

#if (LCD_DEPTH > 1) || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)
char* sb_get_backdrop(enum screen_type screen)
{
    return sb_skin[screen].data->backdrop;
}

bool sb_set_backdrop(enum screen_type screen, char* filename)
{
    if (!filename)
    {
        sb_skin[screen].data->backdrop = NULL;
        return true;
    }
    else if (!sb_skin[screen].data->backdrop)
    {
        /* need to make room on the buffer */
        size_t buf_size;
#if defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)
        if (screen == SCREEN_REMOTE)
            buf_size = REMOTE_LCD_BACKDROP_BYTES;
        else
#endif
            buf_size = LCD_BACKDROP_BYTES;
        sb_skin[screen].data->backdrop = (char*)skin_buffer_alloc(buf_size);
        if (!sb_skin[screen].data->backdrop)
            return false;          
    }
  
    if (!screens[screen].backdrop_load(filename, sb_skin[screen].data->backdrop))
        sb_skin[screen].data->backdrop = NULL;
    return sb_skin[screen].data->backdrop != NULL;
}
        
#endif
void sb_skin_update(enum screen_type screen, bool force)
{
    static long next_update[NB_SCREENS] = {0};
    int i = screen;
    if (!sb_skin_data[screen].wps_loaded)
        return;
    if (TIME_AFTER(current_tick, next_update[i]) || force)
    {
#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
        /* currently, all remotes are readable without backlight
         * so still update those */
        if (lcd_active() || (i != SCREEN_MAIN))
#endif
        {
            bool full_update = sb_skin[i].sync_data->do_full_update;
#if NB_SCREENS > 1
            if (i==SCREEN_MAIN && sb_skin[i].sync_data->do_full_update)
            {
                sb_skin[i].sync_data->do_full_update = false;
                /* we need to make sure the remote gets a full update
                 * next time it is drawn also. so quick n dirty hack */
                next_update[SCREEN_REMOTE] = 0;
            }
            else if (next_update[SCREEN_REMOTE] == 0)
            {
                full_update = true;
            }
#else
            sb_skin[i].sync_data->do_full_update = false;
#endif
            skin_update(&sb_skin[i], force || full_update?
                    SKIN_REFRESH_ALL : SKIN_REFRESH_NON_STATIC);
        }
        next_update[i] = current_tick + update_delay; /* don't update too often */
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

/* This creates and loads a ".sbs" based on the user settings for:
 *  - regular statusbar
 *  - colours
 *  - ui viewport
 *  - backdrop
 */
void sb_create_from_settings(enum screen_type screen)
{
    char buf[128], *ptr, *ptr2;
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
    sb_skin_data_load(screen, buf, false);
}

void sb_skin_init(void)
{
    int i;
    FOR_NB_SCREENS(i)
    {
        oldinfovp_label[i] = NULL;
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
        skin_disarm_touchregions(&sb_skin_data[SCREEN_MAIN]);
    last_context = context;
    button = skin_get_touchaction(&sb_skin_data[SCREEN_MAIN], &offset, &region);
    
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
            const struct settings_list *setting = region->extradata;
            option_select_next_val(setting, button == ACTION_SETTINGS_DEC, true);
        }
        return ACTION_REDRAW;
        /* TODO */
    }
    return button;
}
#endif
