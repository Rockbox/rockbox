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
    vp_icons[screen].x = parent->x + (parent->width / 2 - CENTER_ICONAREA_WIDTH / 2);

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
    bool invert = false;
    switch(button)
    {
        case ACTION_QS_LEFT:
            item = QUICKSCREEN_LEFT;
            break;

        case ACTION_QS_DOWNINV:
            invert = true;      /* fallthrough */
        case ACTION_QS_DOWN:
            item = QUICKSCREEN_BOTTOM;
            break;

        case ACTION_QS_RIGHT:
            item = QUICKSCREEN_RIGHT;
            break;

        default:
            return false;
    }
    option_select_next_val((struct settings_list *)qs->items[item], invert, true);
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
    }
    /* Notify that we're exiting this screen */
    cond_talk_ids_fq(VOICE_OK);
    return changed;
}

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
bool is_setting_quickscreenable(const struct settings_list *setting)
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

void set_as_qs_item(const struct settings_list *setting,
                    enum QUICKSCREEN_ITEM item)
{
    int i;
    for(i=0;i<nb_settings;i++)
    {
        if (&settings[i] == setting)
            break;
    }
    switch (item)
    {
        case QUICKSCREEN_LEFT:
            global_settings.qs_item_left = i;
            break;
        case QUICKSCREEN_RIGHT:
            global_settings.qs_item_right = i;
            break;
        case QUICKSCREEN_BOTTOM:
            global_settings.qs_item_bottom = i;
            break;
        default: /* shut the copiler up */
            break;
    }
}

