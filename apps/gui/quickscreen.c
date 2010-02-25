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
#include "option_select.h"
#include "debug.h"

 /* 1 top, 1 bottom, 2 on either side, 1 for the icons
  * if enough space, top and bottom have 2 lines */
#define MIN_LINES 5
#define MAX_NEEDED_LINES 10
 /* pixels between the 2 center items minimum or between text and icons,
  * and between text and parent boundaries */
#define MARGIN 10
#define CENTER_ICONAREA_SIZE (MARGIN+8*2)

static void quickscreen_fix_viewports(struct gui_quickscreen *qs,
                                        struct screen *display,
                                        struct viewport *parent,
                                        struct viewport
                                                  vps[QUICKSCREEN_ITEM_COUNT],
                                        struct viewport *vp_icons)
{
    int char_height, width, pad = 0;
    int left_width = 0, right_width = 0, vert_lines;
    unsigned char *s;
    int nb_lines = viewport_get_nb_lines(parent);

    /* nb_lines only returns the number of fully visible lines, small screens
        or really large fonts could cause problems with the calculation below.
     */
    if (nb_lines == 0)
        nb_lines++;

    char_height = parent->height/nb_lines;

    /* center the icons VP first */
    *vp_icons = *parent;
    vp_icons->width = CENTER_ICONAREA_SIZE; /* abosulte smallest allowed */
    vp_icons->x = parent->x;
    vp_icons->x += (parent->width-CENTER_ICONAREA_SIZE)/2;

    vps[QUICKSCREEN_BOTTOM] = *parent;
    vps[QUICKSCREEN_TOP] = *parent;
    /* depending on the space the top/buttom items use 1 or 2 lines */
    if (nb_lines < MIN_LINES)
        vert_lines = 1;
    else
        vert_lines = 2;
    vps[QUICKSCREEN_TOP].y = parent->y;
    vps[QUICKSCREEN_TOP].height = vps[QUICKSCREEN_BOTTOM].height
            = vert_lines*char_height;
    vps[QUICKSCREEN_BOTTOM].y
            = parent->y + parent->height - vps[QUICKSCREEN_BOTTOM].height;

    /* enough space vertically, so put a nice margin */
    if (nb_lines >= MAX_NEEDED_LINES)
    {
        vps[QUICKSCREEN_TOP].y += MARGIN;
        vps[QUICKSCREEN_BOTTOM].y -= MARGIN;
    }

    vp_icons->y = vps[QUICKSCREEN_TOP].y
            + vps[QUICKSCREEN_TOP].height;
    vp_icons->height = vps[QUICKSCREEN_BOTTOM].y - vp_icons->y;

    /* adjust the left/right items widths to fit the screen nicely */
    if (qs->items[QUICKSCREEN_LEFT])
    {
        s = P2STR(ID2P(qs->items[QUICKSCREEN_LEFT]->lang_id));
        left_width = display->getstringsize(s, NULL, NULL);
    }
    if (qs->items[QUICKSCREEN_RIGHT])
    {
        s = P2STR(ID2P(qs->items[QUICKSCREEN_RIGHT]->lang_id));
        right_width = display->getstringsize(s, NULL, NULL);
    }

    width = MAX(left_width, right_width);
    if (width*2 + vp_icons->width > parent->width)
    {   /* crop text viewports */
        width = (parent->width - vp_icons->width)/2;
    }
    else
    {   /* add more gap in icons vp */
        int excess = parent->width - vp_icons->width - width*2;
        if (excess > MARGIN*4)
        {
            pad = MARGIN;
            excess -= MARGIN*2;
        }
        vp_icons->x -= excess/2;
        vp_icons->width += excess;
    }

    vps[QUICKSCREEN_LEFT] = *parent;
    vps[QUICKSCREEN_LEFT].x = parent->x + pad;
    vps[QUICKSCREEN_LEFT].width = width;

    vps[QUICKSCREEN_RIGHT] = *parent;
    vps[QUICKSCREEN_RIGHT].x = parent->x + parent->width - width - pad;
    vps[QUICKSCREEN_RIGHT].width = width;

    vps[QUICKSCREEN_LEFT].height = vps[QUICKSCREEN_RIGHT].height
            = 2*char_height;

    vps[QUICKSCREEN_LEFT].y = vps[QUICKSCREEN_RIGHT].y
            = parent->y + (parent->height/2) - char_height;

    /* shrink the icons vp by a few pixels if there is room so the arrows
       aren't drawn right next to the text */
    if (vp_icons->width > CENTER_ICONAREA_SIZE*2)
    {
        vp_icons->width -= CENTER_ICONAREA_SIZE*2/3;
        vp_icons->x += CENTER_ICONAREA_SIZE*2/6;
    }
    if (vp_icons->height > CENTER_ICONAREA_SIZE*2)
    {
        vp_icons->height -= CENTER_ICONAREA_SIZE*2/3;
        vp_icons->y += CENTER_ICONAREA_SIZE*2/6;
    }

    /* text alignment */
    vps[QUICKSCREEN_LEFT].flags &= ~VP_FLAG_ALIGNMENT_MASK; /* left-aligned */
    vps[QUICKSCREEN_TOP].flags    |= VP_FLAG_ALIGN_CENTER;  /* centered */
    vps[QUICKSCREEN_BOTTOM].flags |= VP_FLAG_ALIGN_CENTER;  /* centered */
    vps[QUICKSCREEN_RIGHT].flags  &= ~VP_FLAG_ALIGNMENT_MASK;/* right aligned*/
    vps[QUICKSCREEN_RIGHT].flags  |= VP_FLAG_ALIGN_RIGHT;
}

static void gui_quickscreen_draw(const struct gui_quickscreen *qs,
                                 struct screen *display,
                                 struct viewport *parent,
                                 struct viewport vps[QUICKSCREEN_ITEM_COUNT],
                                 struct viewport *vp_icons)
{
    int i;
    char buf[MAX_PATH];
    unsigned const char *title, *value;
    void *setting;
    int temp;
    display->set_viewport(parent);
    display->clear_viewport();
    for (i = 0; i < QUICKSCREEN_ITEM_COUNT; i++)
    {
        struct viewport *vp = &vps[i];
        if (!qs->items[i])
            continue;
        display->set_viewport(vp);
        display->scroll_stop(vp);

        title = P2STR(ID2P(qs->items[i]->lang_id));
        setting = qs->items[i]->setting;
        temp = option_value_as_int(qs->items[i]);
        value = option_get_valuestring(qs->items[i],
                                       buf, MAX_PATH, temp);

        if (viewport_get_nb_lines(vp) < 2)
        {
            char text[MAX_PATH];
            snprintf(text, MAX_PATH, "%s: %s", title, value);
            display->puts_scroll(0, 0, text);
        }
        else
        {
            display->puts_scroll(0, 0, title);
            display->puts_scroll(0, 1, value);
        }
        display->update_viewport();
    }
    /* draw the icons */
    display->set_viewport(vp_icons);

    if (qs->items[QUICKSCREEN_TOP] != NULL)
    {
        display->mono_bitmap(bitmap_icons_7x8[Icon_UpArrow],
            (vp_icons->width/2) - 4, 0, 7, 8);
    }
    if (qs->items[QUICKSCREEN_RIGHT] != NULL)
    {
        display->mono_bitmap(bitmap_icons_7x8[Icon_FastForward],
            vp_icons->width - 8, (vp_icons->height/2) - 4, 7, 8);
    }
    if (qs->items[QUICKSCREEN_LEFT] != NULL)
    {
        display->mono_bitmap(bitmap_icons_7x8[Icon_FastBackward],
            0, (vp_icons->height/2) - 4, 7, 8);
    }
    if (qs->items[QUICKSCREEN_BOTTOM] != NULL)
    {
        display->mono_bitmap(bitmap_icons_7x8[Icon_DownArrow],
            (vp_icons->width/2) - 4, vp_icons->height - 8, 7, 8);
    }

    display->set_viewport(parent);
    display->update_viewport();
    display->set_viewport(NULL);
}

static void talk_qs_option(const struct settings_list *opt, bool enqueue)
{
    if (!global_settings.talk_menu || !opt)
        return;

    if (!enqueue)
        talk_shutup();
    talk_id(opt->lang_id, true);
    option_talk_value(opt, option_value_as_int(opt), true);
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
        case ACTION_QS_TOP:
            invert = true;
            item = QUICKSCREEN_TOP;
            break;
        case ACTION_QS_LEFT:
            invert = true;
            item = QUICKSCREEN_LEFT;
            break;

        case ACTION_QS_DOWN:
            item = QUICKSCREEN_BOTTOM;
            break;

        case ACTION_QS_RIGHT:
            item = QUICKSCREEN_RIGHT;
            break;

        default:
            return false;
    }
    if (qs->items[item] == NULL)
        return false;

    option_select_next_val(qs->items[item], invert, true);
    talk_qs_option(qs->items[item], false);
    return true;
}

#ifdef HAVE_TOUCHSCREEN
static int quickscreen_touchscreen_button(const struct viewport
                                                    vps[QUICKSCREEN_ITEM_COUNT])
{
    short x,y;
    /* only hitting the text counts, everything else is exit */
    if (action_get_touchscreen_press(&x, &y) != BUTTON_REL)
        return ACTION_NONE;
    else if (viewport_point_within_vp(&vps[QUICKSCREEN_TOP], x, y))
        return ACTION_QS_TOP;
    else if (viewport_point_within_vp(&vps[QUICKSCREEN_BOTTOM], x, y))
        return ACTION_QS_DOWN;
    else if (viewport_point_within_vp(&vps[QUICKSCREEN_LEFT], x, y))
        return ACTION_QS_LEFT;
    else if (viewport_point_within_vp(&vps[QUICKSCREEN_RIGHT], x, y))
        return ACTION_QS_RIGHT;
    return ACTION_STD_CANCEL;
}
#endif

static bool gui_syncquickscreen_run(struct gui_quickscreen * qs, int button_enter)
{
    int button, i, j;
    struct viewport parent[NB_SCREENS];
    struct viewport vps[NB_SCREENS][QUICKSCREEN_ITEM_COUNT];
    struct viewport vp_icons[NB_SCREENS];
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
        viewport_set_defaults(&parent[i], i);
        quickscreen_fix_viewports(qs, &screens[i], &parent[i], vps[i], &vp_icons[i]);
        gui_quickscreen_draw(qs, &screens[i], &parent[i], vps[i], &vp_icons[i]);
    }
    /* Announce current selection on entering this screen. This is all
       queued up, but can be interrupted as soon as a setting is
       changed. */
    cond_talk_ids(VOICE_QUICKSCREEN);
    talk_qs_option(qs->items[QUICKSCREEN_TOP], true);
    talk_qs_option(qs->items[QUICKSCREEN_LEFT], true);
    talk_qs_option(qs->items[QUICKSCREEN_BOTTOM], true);
    talk_qs_option(qs->items[QUICKSCREEN_RIGHT], true);
    while (true) {
        button = get_action(CONTEXT_QUICKSCREEN, HZ/5);
#ifdef HAVE_TOUCHSCREEN
        if (button == ACTION_TOUCHSCREEN)
            button = quickscreen_touchscreen_button(vps[SCREEN_MAIN]);
#endif
        if (default_event_handler(button) == SYS_USB_CONNECTED)
            return(true);
        if (gui_quickscreen_do_button(qs, button))
        {
            changed = true;
            can_quit = true;
            FOR_NB_SCREENS(i)
                gui_quickscreen_draw(qs, &screens[i], &parent[i],
                                     vps[i], &vp_icons[i]);
            if (qs->callback)
                qs->callback(qs);
        }
        else if (button == button_enter)
            can_quit = true;

        if ((button == button_enter) && can_quit)
            break;

        if (button == ACTION_STD_CANCEL)
            break;
    }
    /* Notify that we're exiting this screen */
    cond_talk_ids_fq(VOICE_OK);
    FOR_NB_SCREENS(i)
    {   /* stop scrolling before exiting */
        for (j = 0; j < QUICKSCREEN_ITEM_COUNT; j++)
            screens[i].scroll_stop(&vps[i][j]);
    }

    return changed;
}

static const struct settings_list *get_setting(int gs_value,
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

    qs.items[QUICKSCREEN_TOP] =
            get_setting(global_settings.qs_items[QUICKSCREEN_TOP],
                        find_setting(&global_settings.party_mode, NULL));
    qs.items[QUICKSCREEN_LEFT] =
            get_setting(global_settings.qs_items[QUICKSCREEN_LEFT],
                        find_setting(&global_settings.playlist_shuffle, NULL));
    qs.items[QUICKSCREEN_RIGHT] =
            get_setting(global_settings.qs_items[QUICKSCREEN_RIGHT],
                        find_setting(&global_settings.repeat_mode, NULL));
    qs.items[QUICKSCREEN_BOTTOM] =
            get_setting(global_settings.qs_items[QUICKSCREEN_BOTTOM],
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
    qs.items[QUICKSCREEN_TOP] = NULL;
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
                    enum quickscreen_item item)
{
    int i;
    for (i = 0; i < nb_settings; i++)
    {
        if (&settings[i] == setting)
            break;
    }

    global_settings.qs_items[item] = i;
}
