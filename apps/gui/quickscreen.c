/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Jonathan Gordon
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
#include "config.h"
#include "system.h"
#include "icons.h"
#include "font.h"
#include "kernel.h"
#include "misc.h"
#include "statusbar.h"
#include "action.h"
#include "settings_list.h"
#include "lang.h"
#include "playlist.h"
#include "dsp.h"
#include "viewport.h"
#include "audio.h"
#include "quickscreen.h"
#include "talk.h"
#include "list.h"
#include "splash.h"

static struct viewport vps[NB_SCREENS][QUICKSCREEN_ITEM_COUNT];
static struct viewport vp_icons[NB_SCREENS];
/* vp_icons will be used like this:
   the side icons will be aligned to the top of this vp and to their sides
   the bottom icon wil be aligned center and at the bottom of this vp */

#define MIN_LINES 4
#define MAX_NEEDED_LINES 8
#define CENTER_MARGIN 10 /* pixels between the 2 center items minimum */
#define CENTER_ICONAREA_WIDTH (CENTER_MARGIN+8*2)

static void quickscreen_fix_viewports(struct gui_quickscreen *qs,
                                      struct screen *display,
                                      struct viewport *parent)
{
#ifdef HAVE_REMOTE_LCD
    int screen = display->screen_type;
#else
    const int screen = 0;
#endif

    int char_height, i, width, pad = 0;
    int left_width, right_width, bottom_lines = 2;
    unsigned char *s;
    int nb_lines = viewport_get_nb_lines(parent);
    char_height = parent->height/nb_lines;

    /* center the icons VP first */
    vp_icons[screen] = *parent;
    vp_icons[screen].width = CENTER_ICONAREA_WIDTH; /* abosulte smallest allowed */
    vp_icons[screen].x = (parent->width-parent->x-CENTER_ICONAREA_WIDTH)/2;

    vps[screen][QUICKSCREEN_BOTTOM] = *parent;
    if (nb_lines <= MIN_LINES) /* make the bottom item use 1 line */
        bottom_lines = 1;
    else
        bottom_lines = 2;
    vps[screen][QUICKSCREEN_BOTTOM].height = bottom_lines*char_height;
    vps[screen][QUICKSCREEN_BOTTOM].y =
                    parent->y + parent->height - bottom_lines*char_height;
    if (nb_lines >= MAX_NEEDED_LINES)
    {
        vps[screen][QUICKSCREEN_BOTTOM].y -= char_height;
    }

    /* adjust the left/right items widths to fit the screen nicely */
    s = P2STR(ID2P(qs->items[QUICKSCREEN_LEFT]->lang_id));
    left_width = display->getstringsize(s, NULL, NULL);
    s = P2STR(ID2P(qs->items[QUICKSCREEN_RIGHT]->lang_id));
    right_width = display->getstringsize(s, NULL, NULL);
    nb_lines -= bottom_lines;
    
    width = MAX(left_width, right_width);
    if (width*2 + vp_icons[screen].width > display->lcdwidth)
        width = (display->lcdwidth - vp_icons[screen].width)/2;
    else /* add more gap in icons vp */
    {
        int excess = display->lcdwidth - vp_icons[screen].width - width*2;
        if (excess > CENTER_MARGIN*4)
        {
            pad = CENTER_MARGIN;
            excess -= CENTER_MARGIN*2;
        }
        vp_icons[screen].x -= excess/2;
        vp_icons[screen].width += excess;
    }
    vps[screen][QUICKSCREEN_LEFT] = *parent;
    vps[screen][QUICKSCREEN_LEFT].x = parent->x + pad;
    vps[screen][QUICKSCREEN_LEFT].width = width;
    
    vps[screen][QUICKSCREEN_RIGHT] = *parent;
    vps[screen][QUICKSCREEN_RIGHT].x = parent->x + parent->width - width - pad;
    vps[screen][QUICKSCREEN_RIGHT].width = width;
    
    /* shrink the icons vp by a few pixels if there is room so the arrows
       aren't drawn right next to the text */
    if (vp_icons[screen].width > CENTER_ICONAREA_WIDTH+8)
    {
        vp_icons[screen].width -= 8;
        vp_icons[screen].x += 4;
    }
    

    if (nb_lines <= MIN_LINES)
        i = 0;
    else
        i = nb_lines/2;
    vps[screen][QUICKSCREEN_LEFT].y = parent->y + (i*char_height);
    vps[screen][QUICKSCREEN_RIGHT].y = parent->y + (i*char_height);
    if (nb_lines >= 3)
        i = 3*char_height;
    else
        i = nb_lines*char_height;

    vps[screen][QUICKSCREEN_LEFT].height = i;
    vps[screen][QUICKSCREEN_RIGHT].height = i;
    vp_icons[screen].y = vps[screen][QUICKSCREEN_LEFT].y + (char_height/2);
    vp_icons[screen].height =
                vps[screen][QUICKSCREEN_BOTTOM].y - vp_icons[screen].y;
}

static void quickscreen_draw_text(char *s, int item, bool title,
                                  struct screen *display, struct viewport *vp)
{
    int nb_lines = viewport_get_nb_lines(vp);
    int w, h, line = 0, x = 0;
    display->getstringsize(s, &w, &h);

    if (nb_lines > 1 && !title)
        line = 1;
    switch (item)
    {
        case QUICKSCREEN_BOTTOM:
            x = (vp->width - w)/2;
            break;
        case QUICKSCREEN_LEFT:
            x = 0;
            break;
        case QUICKSCREEN_RIGHT:
            x = vp->width - w;
            break;
    }
    if (w>vp->width)
        display->puts_scroll(0, line, s);
    else
        display->putsxy(x, line*h, s);
}

static void gui_quickscreen_draw(struct gui_quickscreen *qs,
                                 struct screen *display,
                                 struct viewport *parent)
{
#ifdef HAVE_REMOTE_LCD
    int screen = display->screen_type;
#else
    const int screen = 0;
#endif

    int i;
    char buf[MAX_PATH];
    unsigned char *title, *value;
    void *setting;
    int temp;
    display->set_viewport(parent);
    display->clear_viewport();
    for (i=0; i<QUICKSCREEN_ITEM_COUNT; i++)
    {
        if (!qs->items[i])
            continue;
        display->set_viewport(&vps[screen][i]);
        display->scroll_stop(&vps[screen][i]);

        title = P2STR(ID2P(qs->items[i]->lang_id));
        setting = qs->items[i]->setting;
        temp = option_value_as_int(qs->items[i]);
        value = option_get_valuestring((struct settings_list*)qs->items[i],
                                       buf, MAX_PATH, temp);

        if (vps[screen][i].height < display->getcharheight()*2)
        {
            char text[MAX_PATH];
            snprintf(text, MAX_PATH, "%s: %s", title, value);
            quickscreen_draw_text(text, i, true, display, &vps[screen][i]);
        }
        else
        {
            quickscreen_draw_text(title, i, true, display, &vps[screen][i]);
            quickscreen_draw_text(value, i, false, display, &vps[screen][i]);
        }
        display->update_viewport();
    }
    /* draw the icons */
    display->set_viewport(&vp_icons[screen]);
    display->mono_bitmap(bitmap_icons_7x8[Icon_FastForward],
                         vp_icons[screen].width - 8, 0, 7, 8);
    display->mono_bitmap(bitmap_icons_7x8[Icon_FastBackward], 0, 0, 7, 8);
    display->mono_bitmap(bitmap_icons_7x8[Icon_DownArrow],
                         (vp_icons[screen].width/2) - 4, 
                          vp_icons[screen].height - 7, 7, 8);
    display->update_viewport();

    display->set_viewport(parent);
    display->update_viewport();
    display->set_viewport(NULL);
}

static void talk_qs_option(struct settings_list *opt, bool enqueue)
{
    if (global_settings.talk_menu) {
        if(!enqueue)
            talk_shutup();
        talk_id(opt->lang_id, true);
        option_talk_value(opt, option_value_as_int(opt), true);
    }
}

/*
 * Does the actions associated to the given button if any
 *  - qs : the quickscreen
 *  - button : the key we are going to analyse
 * returns : true if the button corresponded to an action, false otherwise
 */
static bool gui_quickscreen_do_button(struct gui_quickscreen * qs, int button)
{
    int item;
    switch(button)
    {
        case ACTION_QS_LEFT:
            item = QUICKSCREEN_LEFT;
            break;

        case ACTION_QS_DOWN:
        case ACTION_QS_DOWNINV:
            item = QUICKSCREEN_BOTTOM;
            break;

        case ACTION_QS_RIGHT:
            item = QUICKSCREEN_RIGHT;
            break;

        default:
            return false;
    }
    option_select_next_val((struct settings_list *)qs->items[item], false, true);
    talk_qs_option((struct settings_list *)qs->items[item], false);
    return true;
}

bool gui_syncquickscreen_run(struct gui_quickscreen * qs, int button_enter)
{
    int button, i;
    struct viewport vp[NB_SCREENS];
    bool changed = false;
    /* To quit we need either :
     *  - a second press on the button that made us enter
     *  - an action taken while pressing the enter button,
     *    then release the enter button*/
    bool can_quit = false;
    gui_syncstatusbar_draw(&statusbars, true);
    FOR_NB_SCREENS(i)
    {
        screens[i].set_viewport(NULL);
        screens[i].stop_scroll();
        viewport_set_defaults(&vp[i], i);
        quickscreen_fix_viewports(qs, &screens[i], &vp[i]);
        gui_quickscreen_draw(qs, &screens[i], &vp[i]);
    }
    /* Announce current selection on entering this screen. This is all
       queued up, but can be interrupted as soon as a setting is
       changed. */
    cond_talk_ids(VOICE_QUICKSCREEN);
    talk_qs_option((struct settings_list *)qs->items[QUICKSCREEN_LEFT], true);
    talk_qs_option((struct settings_list *)qs->items[QUICKSCREEN_BOTTOM], true);
    talk_qs_option((struct settings_list *)qs->items[QUICKSCREEN_RIGHT], true);
    while (true) {
        button = get_action(CONTEXT_QUICKSCREEN,HZ/5);
        if(default_event_handler(button) == SYS_USB_CONNECTED)
            return(true);
        if(gui_quickscreen_do_button(qs, button))
        {
            changed = true;
            can_quit=true;
            FOR_NB_SCREENS(i)
                gui_quickscreen_draw(qs, &screens[i], &vp[i]);
            if (qs->callback)
                qs->callback(qs);
        }
        else if(button==button_enter)
            can_quit=true;
            
        if((button == button_enter) && can_quit)
            break;
            
        if(button==ACTION_STD_CANCEL)
            break;
            
        gui_syncstatusbar_draw(&statusbars, false);
    }
    /* Notify that we're exiting this screen */
    cond_talk_ids_fq(VOICE_OK);
    return changed;
}
static bool is_setting_quickscreenable(const struct settings_list *setting);
static inline const struct settings_list *get_setting(int gs_value,
                                        const struct settings_list *defaultval)
{
    if (gs_value != -1 && gs_value < nb_settings &&
        is_setting_quickscreenable(&settings[gs_value]))
        return &settings[gs_value];
    return defaultval;
}
bool quick_screen_quick(int button_enter)
{
    struct gui_quickscreen qs;
    bool oldshuffle = global_settings.playlist_shuffle;
    int oldrepeat = global_settings.repeat_mode;

    qs.items[QUICKSCREEN_LEFT] = 
            get_setting(global_settings.qs_item_left,
                        find_setting(&global_settings.playlist_shuffle, NULL));
    qs.items[QUICKSCREEN_RIGHT] = 
            get_setting(global_settings.qs_item_right,
                    find_setting(&global_settings.repeat_mode, NULL));
    qs.items[QUICKSCREEN_BOTTOM] = 
            get_setting(global_settings.qs_item_bottom,
                    find_setting(&global_settings.dirfilter, NULL));

    qs.callback = NULL;
    if (gui_syncquickscreen_run(&qs, button_enter))
    {
        settings_save();
        settings_apply(false);
        /* make sure repeat/shuffle/any other nasty ones get updated */
        if ( oldrepeat != global_settings.repeat_mode &&
             (audio_status() & AUDIO_STATUS_PLAY) )
        {
            audio_flush_and_reload_tracks();
        }
        if (oldshuffle != global_settings.playlist_shuffle
            && audio_status() & AUDIO_STATUS_PLAY)
        {
#if CONFIG_CODEC == SWCODEC
            dsp_set_replaygain();
#endif
            if (global_settings.playlist_shuffle)
                playlist_randomise(NULL, current_tick, true);
            else
                playlist_sort(NULL, true);
        }
    }
    return(0);
}

#ifdef BUTTON_F3
bool quick_screen_f3(int button_enter)
{
    struct gui_quickscreen qs;
    qs.items[QUICKSCREEN_LEFT] =
                    find_setting(&global_settings.scrollbar, NULL);
    qs.items[QUICKSCREEN_RIGHT] =
                    find_setting(&global_settings.statusbar, NULL);
    qs.items[QUICKSCREEN_BOTTOM] =
                    find_setting(&global_settings.flip_display, NULL);
    qs.callback = NULL;
    if (gui_syncquickscreen_run(&qs, button_enter))
    {
        settings_save();
        settings_apply(false);
    }
    return(0);
}
#endif /* BUTTON_F3 */

/* stuff to make the quickscreen configurable */
static bool is_setting_quickscreenable(const struct settings_list *setting)
{
    /* to keep things simple, only settings which have a lang_id set are ok */
    if (setting->lang_id < 0 || (setting->flags&F_BANFROMQS))
        return false;
    switch (setting->flags&F_T_MASK)
    {
        case F_T_BOOL:
            return true;
        case F_T_INT:
        case F_T_UINT:
            return (setting->RESERVED != NULL);
        default:
            return false;
    }
}

static const struct settings_list *find_setting_from_index(int index)
{
    int count = -1, i;
    const struct settings_list *setting = &settings[0];
    for(i=0;i<nb_settings;i++)
    {
        setting = &settings[i];
        if (is_setting_quickscreenable(setting))
            count++;
        if (count == index)
            return setting;
    }
    return NULL;
}
static char* quickscreen_setter_getname(int selected_item, void *data,
                                        char *buffer, size_t buffer_len)
{
    (void)data;
    const struct settings_list *setting = find_setting_from_index(selected_item);
    snprintf(buffer, buffer_len, "%s  (%s)",
             str(setting->lang_id), setting->cfg_name);
    return buffer;
}
static int quickscreen_setter_speak_item(int selected_item, void * data)
{
    (void)data;
    talk_id(find_setting_from_index(selected_item)->lang_id, true);
    return 0;
}
static int quickscreen_setter_action_callback(int action, 
                                              struct gui_synclist *lists)
{
    const struct settings_list *temp = lists->data;
    switch (action)
    {
        case ACTION_STD_OK:
            /* ok, quit */
            return ACTION_STD_CANCEL;
        case ACTION_STD_CONTEXT: /* real settings use this to reset to default */
        {
            int i=0, count=0;
            reset_setting(temp, temp->setting);
            for(i=0;i<nb_settings;i++)
            {
                if (is_setting_quickscreenable(&settings[i]))
                    count++;
                if (*(int*)temp->setting == i)
                {
                    gui_synclist_select_item(lists, count-1);
                    break;
                }
            }
            return ACTION_REDRAW;
        }
    }
    return action;
}
int quickscreen_set_option(void *data)
{
    int valid_settings_count = 0;
    int i, newval = 0, oldval, *setting = NULL;
    struct simplelist_info info;
    switch ((intptr_t)data)
    {
        case QUICKSCREEN_LEFT:
            setting = &global_settings.qs_item_left;
            break;
        case QUICKSCREEN_RIGHT:
            setting = &global_settings.qs_item_right;
            break;
        case QUICKSCREEN_BOTTOM:
            setting = &global_settings.qs_item_bottom;
            break;
    }
    oldval = *setting;
    for(i=0;i<nb_settings;i++)
    {
        if (is_setting_quickscreenable(&settings[i]))
            valid_settings_count++;
        if (oldval == i)
            newval = valid_settings_count - 1;
    }
    
    simplelist_info_init(&info, str(LANG_QS_ITEMS), 
                    valid_settings_count,
                    (void*)find_setting(setting, NULL)); /* find the qs item being changed */
    info.get_name = quickscreen_setter_getname;
    if(global_settings.talk_menu)
        info.get_talk = quickscreen_setter_speak_item;
    info.action_callback = quickscreen_setter_action_callback;
    info.selection = newval;
    simplelist_show_list(&info);
    if (info.selection != oldval)
    {
        if (info.selection != -1)
        {
            const struct settings_list *temp = find_setting_from_index(info.selection);
            int i = 0;
            for(i=0;i<nb_settings;i++)
            {
                if (&settings[i] == temp)
                    break;
            }
            *setting = i;
            settings_save();
        }
        /* probably should splash LANG_CANCEL here but right now
           we cant find out the selection when the cancel button was
           pressed, (without hacks)so we cant know if the 
           selection was changed, or just viewed */
    }
    return 0;
}


