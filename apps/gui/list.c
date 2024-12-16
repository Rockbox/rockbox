/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Kevin Ferrare
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
#include <stdarg.h>
#include <stdio.h>
#include "config.h"
#include "lcd.h"
#include "font.h"
#include "button.h"
#include "string.h"
#include "settings.h"
#include "kernel.h"
#include "system.h"

#include "action.h"
#include "screen_access.h"
#include "list.h"
#include "scrollbar.h"
#include "lang.h"
#include "sound.h"
#include "misc.h"
#include "talk.h"
#include "viewport.h"
#include "appevents.h"
#include "statusbar-skinned.h"

/* The minimum number of pending button events in queue before starting
 * to limit list drawing interval.
 */
#define FRAMEDROP_TRIGGER 6

void list_draw(struct screen *display, struct gui_synclist *list);

static long last_dirty_tick;
static struct viewport parent[NB_SCREENS];
static struct gui_synclist *current_lists;

static bool list_is_dirty(struct gui_synclist *list)
{
    return TIME_BEFORE(list->dirty_tick, last_dirty_tick);
}

static void list_force_reinit(unsigned short id, void *param, void *last_dirty_tick)
{
    (void)id;
    (void)param;
    *(int *)last_dirty_tick = current_tick;
}

void list_init(void)
{
    last_dirty_tick = current_tick;
    add_event_ex(GUI_EVENT_THEME_CHANGED, false, list_force_reinit, &last_dirty_tick);
}

static void list_init_viewports(struct gui_synclist *list)
{
    bool parent_used = (*list->parent == &parent[SCREEN_MAIN]);

    if (parent_used)
    {
        FOR_NB_SCREENS(i)
        {
            gui_synclist_set_viewport_defaults(list->parent[i], i);
        }
    }
    list->dirty_tick = current_tick;
}

static int list_nb_lines(struct gui_synclist *list, enum screen_type screen)
{
    struct viewport *vp = list->parent[screen];
    return vp->height / list->line_height[screen];
}

bool list_display_title(struct gui_synclist *list, enum screen_type screen)
{
    return list->title != NULL &&
        !sb_set_title_text(list->title, list->title_icon, screen) &&
        list_nb_lines(list, screen) > 2;
}

int list_get_nb_lines(struct gui_synclist *list, enum screen_type screen)
{
    int lines = skinlist_get_line_count(screen, list);
    if (lines < 0)
    {
        lines = list_nb_lines(list, screen);
        if (list_display_title(list, screen))
            lines -= 1;
    }
    return lines;
}

void list_init_item_height(struct gui_synclist *list, enum screen_type screen)
{
    struct viewport *vp = list->parent[screen];
    int line_height = font_get(vp->font)->height;
#ifdef HAVE_TOUCHSCREEN
    /* the 4/12 factor is designed for reasonable item size on a 160dpi screen */
    if (global_settings.list_line_padding == -1)
        list->line_height[screen] = MAX(lcd_get_dpi()*4/12, line_height);
    else
        list->line_height[screen] = line_height + global_settings.list_line_padding;
#else
    list->line_height[screen] = line_height;
#endif
}

static void gui_synclist_init_display_settings(struct gui_synclist * list)
{
    struct user_settings *gs = &global_settings;
    list->scrollbar = gs->scrollbar;
    list->show_icons = gs->show_icons;
    list->scroll_paginated = gs->scroll_paginated;
    list->keyclick = gs->keyclick;
    list->talk_menu = gs->talk_menu;
    list->wraparound = gs->list_wraparound;
    list->cursor_style = gs->cursor_style;
}

/*
 * Initializes a scrolling list
 *  - gui_list : the list structure to initialize
 *  - callback_get_item_name : pointer to a function that associates a label
 *    to a given item number
 *  - data : extra data passed to the list callback
 *  - scroll_all :
 *  - selected_size :
 *  - parent : the parent viewports to use. NULL means the full screen minus
 *             statusbar if enabled. NOTE: new screens should NOT set this to NULL.
 */
void gui_synclist_init(struct gui_synclist * gui_list,
    list_get_name callback_get_item_name,
    void * data,
    bool scroll_all,
    int selected_size, struct viewport list_parent[NB_SCREENS]
    )
{
    gui_list->callback_get_item_icon = NULL;
    gui_list->callback_get_item_name = callback_get_item_name;
    gui_list->callback_speak_item = NULL;
    gui_list->callback_draw_item = NULL;
    gui_list->nb_items = 0;
    gui_list->selected_item = 0;
    gui_synclist_init_display_settings(gui_list);
#ifdef HAVE_TOUCHSCREEN
    gui_list->y_pos = 0;
#endif
    FOR_NB_SCREENS(i)
    {
        gui_list->start_item[i] = 0;
        gui_list->offset_position[i] = 0;
        if (list_parent)
            gui_list->parent[i] = &list_parent[i];
        else
            gui_list->parent[i] = &parent[i];
    }
    list_init_viewports(gui_list);
    FOR_NB_SCREENS(i)
        list_init_item_height(gui_list, i);
    gui_list->data = data;
    gui_list->scroll_all = scroll_all;
    gui_list->selected_size = selected_size;
    gui_list->title = NULL;
    gui_list->title_icon = Icon_NOICON;

    gui_list->scheduled_talk_tick = gui_list->last_talked_tick = 0;
    gui_list->dirty_tick = current_tick;

#ifdef HAVE_LCD_COLOR
    gui_list->title_color = -1;
    gui_list->callback_get_item_color = NULL;
    gui_list->selection_color = NULL;
#endif
}

int gui_list_get_item_offset(struct gui_synclist * gui_list,
                            int item_width,
                            int text_pos,
                            struct screen * display,
                            struct viewport *vp)
{
    int item_offset = gui_list->offset_position[display->screen_type];

    if (!global_settings.offset_out_of_view)
    {
        int view_width = (vp->width - text_pos);
        /* if text is smaller than view */
        if (item_width <= view_width)
        {
            item_offset = 0;
        }
        /* if text got out of view  */
        else if (item_offset > item_width - view_width)
        {
            item_offset = item_width - view_width;
        }
    }

    return item_offset;
}

/*
 * Force a full screen update.
 */
void gui_synclist_draw(struct gui_synclist *gui_list)
{
    if (list_is_dirty(gui_list))
    {
        list_init_viewports(gui_list);
        FOR_NB_SCREENS(i)
            list_init_item_height(gui_list, i);
        gui_synclist_select_item(gui_list, gui_list->selected_item);
    }
    FOR_NB_SCREENS(i)
    {
        if (!skinlist_draw(&screens[i], gui_list))
            list_draw(&screens[i], gui_list);
    }
}

/* sets up the list so the selection is shown correctly on the screen */
static void gui_list_put_selection_on_screen(struct gui_synclist * gui_list,
                                             enum screen_type screen)
{
    int nb_lines = list_get_nb_lines(gui_list, screen);
    int bottom = MAX(0, gui_list->nb_items - nb_lines);
    int new_start_item = gui_list->start_item[screen];
    int difference = gui_list->selected_item - gui_list->start_item[screen];
    const int scroll_limit_up   = (nb_lines < gui_list->selected_size+2 ? 0:1);
    const int scroll_limit_down = (scroll_limit_up+gui_list->selected_size);

    if (gui_list->selected_size >= nb_lines)
    {
        new_start_item = gui_list->selected_item;
    }
    else if (gui_list->scroll_paginated)
    {
        nb_lines -= nb_lines%gui_list->selected_size;
        if (difference < 0 || difference >= nb_lines)
        {
            new_start_item = gui_list->selected_item -
                                (gui_list->selected_item%nb_lines);
        }
    }
    else if (difference <= scroll_limit_up) /* list moved up */
    {
        new_start_item = gui_list->selected_item - scroll_limit_up;
    }
    else if (difference > nb_lines - scroll_limit_down) /* list moved down */
    {
        new_start_item = gui_list->selected_item + scroll_limit_down - nb_lines;
    }
    if (new_start_item < 0)
        gui_list->start_item[screen] = 0;
    else if (new_start_item > bottom)
        gui_list->start_item[screen] = bottom;
    else
        gui_list->start_item[screen] = new_start_item;
#ifdef HAVE_TOUCHSCREEN
    gui_list->y_pos = gui_list->start_item[SCREEN_MAIN] * gui_list->line_height[SCREEN_MAIN];
#endif
}

static void edge_beep(struct gui_synclist * gui_list, bool wrap)
{
    if (gui_list->keyclick)
    {
        enum system_sound sound = SOUND_LIST_EDGE_BEEP_WRAP;
        if (!wrap) /* a bounce */
        {
            static long last_bounce_tick = 0;
            if(TIME_BEFORE(current_tick, last_bounce_tick+HZ/4))
                return;
            last_bounce_tick = current_tick;
            sound = SOUND_LIST_EDGE_BEEP_NOWRAP;
        }
        /* Next thing the list code will do is go speak the item, doing
           a talk_shutup() first. Shutup now so the beep is clearer, and
           make sure the subsequent shutup is skipped because otherwise
           it'd kill the pcm buffer. */
        if (gui_list->callback_speak_item) {
            talk_shutup();
            talk_force_enqueue_next();
            system_sound_play(sound);

            /* On at least x5: if, instead of the above shutup, I insert a
               sleep just after the beep_play() call, to delay the subsequent
               shutup and talk, then in some cases the beep is not played: if
               the end of a previous utterance is still playing from the pcm buf,
               the beep fails, even if there would seem to remain enough time
               to the utterance to mix in the beep. */

            /* Somehow, the following voice utterance is suppressed on e200,
               but not on x5. Work around... */
            sleep((40*HZ +999)/1000); // FIXME:  Is this really needed?
            talk_force_shutup();
        }
        else
            system_sound_play(sound);
    }
}

static void _gui_synclist_speak_item(struct gui_synclist *lists)
{
    list_speak_item *cb = lists->callback_speak_item;
    if (cb && lists->nb_items != 0)
    {
        talk_shutup();
        /* If we have just very recently started talking, then we want
           to stay silent for a while until things settle. Likewise if
           we already had a pending scheduled announcement not yet due
           we need to reschedule it. */
        if ((lists->scheduled_talk_tick &&
                TIME_BEFORE(current_tick, lists->scheduled_talk_tick)) ||
            (lists->last_talked_tick &&
                TIME_BEFORE(current_tick, lists->last_talked_tick + HZ/5)))
        {
            lists->scheduled_talk_tick = current_tick + HZ/5;
        }
        else
        {
            lists->scheduled_talk_tick = 0; /* work done */
            cb(lists->selected_item, lists->data);
            lists->last_talked_tick = current_tick;
        }
    }
}

void gui_synclist_speak_item(struct gui_synclist *lists)
{
    if (lists->talk_menu)
    {
        if (lists->nb_items == 0)
            talk_id(VOICE_EMPTY_LIST, true);
        else
            _gui_synclist_speak_item(lists);
    }
}

/*
 * Selects an item in the list
 *  - gui_list : the list structure
 *  - item_number : the number of the item which will be selected
 */
void gui_synclist_select_item(struct gui_synclist * gui_list, int item_number)
{
    if (item_number >= gui_list->nb_items || item_number < 0)
        return;
    if (item_number != gui_list->selected_item)
    {
        gui_list->selected_item = item_number;
        _gui_synclist_speak_item(gui_list);
    }
    FOR_NB_SCREENS(i)
        gui_list_put_selection_on_screen(gui_list, i);
}

static void gui_list_select_at_offset(struct gui_synclist * gui_list,
                                      int offset, bool allow_wrap)
{
    if (gui_list->selected_size > 1)
    {
        offset *= gui_list->selected_size;
    }

    int new_selection = gui_list->selected_item + offset;
    int remain = (gui_list->nb_items - gui_list->selected_size);

    if (new_selection >= gui_list->nb_items)
    {
        new_selection = allow_wrap ? 0 : remain;
        edge_beep(gui_list, allow_wrap);
    }
    else if (new_selection < 0)
    {
        new_selection = allow_wrap ? remain : 0;
        edge_beep(gui_list, allow_wrap);
    }

    gui_synclist_select_item(gui_list, new_selection);
}

/*
 * Adds an item to the list (the callback will be asked for one more item)
 * - gui_list : the list structure
 */
void gui_synclist_add_item(struct gui_synclist * gui_list)
{
    gui_list->nb_items++;
    /* if only one item in the list, select it */
    if (gui_list->nb_items == 1)
        gui_list->selected_item = 0;
}

/*
 * Removes an item to the list (the callback will be asked for one less item)
 * - gui_list : the list structure
 */
void gui_synclist_del_item(struct gui_synclist * gui_list)
{
    if (gui_list->nb_items > 0)
    {
        if (gui_list->selected_item == gui_list->nb_items-1)
            gui_list->selected_item--;
        gui_list->nb_items--;
        gui_synclist_select_item(gui_list, gui_list->selected_item);
    }
}

/*
 * Set the title and title icon of the list. Setting title to NULL disables
 * both the title and icon. Use NOICON if there is no icon.
 */
void gui_synclist_set_title(struct gui_synclist * gui_list,
                            const char * title, enum themable_icons icon)
{
    gui_list->title = title;
    gui_list->title_icon = icon;
    FOR_NB_SCREENS(i)
        sb_set_title_text(title, icon, i);
    send_event(GUI_EVENT_ACTIONUPDATE, (void*)1);
}

void gui_synclist_set_nb_items(struct gui_synclist * lists, int nb_items)
{
    lists->nb_items = nb_items;
    FOR_NB_SCREENS(i)
    {
        lists->offset_position[i] = 0;
    }
}
int gui_synclist_get_nb_items(struct gui_synclist * lists)
{
    return lists->nb_items;
}
int  gui_synclist_get_sel_pos(struct gui_synclist * lists)
{
    return lists->selected_item;
}
void gui_synclist_set_icon_callback(struct gui_synclist * lists,
                                    list_get_icon icon_callback)
{
    lists->callback_get_item_icon = icon_callback;
}

void gui_synclist_set_voice_callback(struct gui_synclist * lists,
                                     list_speak_item voice_callback)
{
    lists->callback_speak_item = voice_callback;
}

void gui_synclist_set_viewport_defaults(struct viewport *vp,
                                        enum screen_type screen)
{
    viewport_set_defaults(vp, screen);
}

#ifdef HAVE_LCD_COLOR
void gui_synclist_set_color_callback(struct gui_synclist * lists,
                                     list_get_color color_callback)
{
    lists->callback_get_item_color = color_callback;
}

void gui_synclist_set_sel_color(struct gui_synclist * lists,
                                struct list_selection_color *list_sel_color)
{
    lists->selection_color = list_sel_color;
    if(list_sel_color)
    {
        FOR_NB_SCREENS(i) /* might need to be only SCREEN_MAIN */
        {
            lists->parent[i]->fg_pattern = list_sel_color->fg_color;
            lists->parent[i]->bg_pattern = list_sel_color->bg_color;
        }
    }
    else
        list_init_viewports(lists);
}
#endif

static void gui_synclist_select_next_page(struct gui_synclist * lists,
                                          enum screen_type screen,
                                          bool allow_wrap)
{
    int nb_lines = list_get_nb_lines(lists, screen);
    if (lists->selected_size > 1)
        nb_lines = MAX(1, nb_lines/lists->selected_size);

    gui_list_select_at_offset(lists, nb_lines, allow_wrap);
}

static void gui_synclist_select_previous_page(struct gui_synclist * lists,
                                              enum screen_type screen,
                                              bool allow_wrap)
{
    int nb_lines = list_get_nb_lines(lists, screen);
    if (lists->selected_size > 1)
        nb_lines = MAX(1, nb_lines/lists->selected_size);

    gui_list_select_at_offset(lists, -nb_lines, allow_wrap);
}

/*
 * Makes all the item in the list scroll by one step to the right.
 * Should stop increasing the value when reaching the widest item value
 * in the list.
 */
static void gui_synclist_scroll_right(struct gui_synclist * lists)
{
    FOR_NB_SCREENS(i)
    {
        /* FIXME: This is a fake right boundry limiter. there should be some
        * callback function to find the longest item on the list in pixels,
        * to stop the list from scrolling past that point */
        lists->offset_position[i] += global_settings.screen_scroll_step;
        if (lists->offset_position[i] > 1000)
            lists->offset_position[i] = 1000;
    }
}

/*
 * Makes all the item in the list scroll by one step to the left.
 * stops at starting position.
 */
static void gui_synclist_scroll_left(struct gui_synclist * lists)
{
    FOR_NB_SCREENS(i)
    {
        lists->offset_position[i] -= global_settings.screen_scroll_step;
        if (lists->offset_position[i] < 0)
            lists->offset_position[i] = 0;
    }
}

bool gui_synclist_keyclick_callback(int action, void* data)
{
    struct gui_synclist *lists = (struct gui_synclist *)data;

    /* Block the beep if we're at the end of the list and we're not wrapping. */
    if (lists->selected_item == 0)
    {
        if (action == ACTION_STD_PREVREPEAT)
            return false;
        if (action == ACTION_STD_PREV && !lists->wraparound)
            return false;
    }

    if (lists->selected_item == lists->nb_items - lists->selected_size)
    {
        if (action == ACTION_STD_NEXTREPEAT)
            return false;
        if (action == ACTION_STD_NEXT && !lists->wraparound)
            return false;
    }

    return action != ACTION_NONE && !IS_SYSEVENT(action);
}

/*
 * Magic to make sure the list gets updated correctly if the skin does
 * something naughty like a full screen update when we are in a button
 * loop.
 *
 * The GUI_EVENT_NEED_UI_UPDATE event is registered for in list_do_action_timeout()
 * as a oneshot and current_lists updated. later current_lists is set to NULL
 * in gui_synclist_do_button() effectively disabling the callback. 
*  This is done because if something is using the list UI they *must* be calling those
 * two functions in the correct order or the list wont work.
 */

static void _lists_uiviewport_update_callback(unsigned short id,
                                              void *data, void *userdata)
{
    (void)id;
    (void)data;
    (void)userdata;

    if (current_lists)
        gui_synclist_draw(current_lists);
}

bool gui_synclist_do_button(struct gui_synclist * lists, int *actionptr)
{
    int action = *actionptr;
    static bool pgleft_allow_cancel = false;

#ifdef HAVE_WHEEL_ACCELERATION
    int next_item_modifier = button_apply_acceleration(get_action_data());
#else
    static int next_item_modifier = 1;
    static int last_accel_tick = 0;

    if (IS_SYSEVENT(action))
        return false;

    if (action != ACTION_TOUCHSCREEN)
    {
        if (global_settings.list_accel_start_delay)
        {
            int start_delay = global_settings.list_accel_start_delay * HZ;
            int accel_wait = global_settings.list_accel_wait * HZ;

            if (get_action_statuscode(NULL)&ACTION_REPEAT)
            {
                if (!last_accel_tick)
                    last_accel_tick = current_tick + start_delay;
                else if (TIME_AFTER(current_tick, last_accel_tick + accel_wait))
                {
                    last_accel_tick = current_tick;
                    next_item_modifier++;
                }
            }
            else if (last_accel_tick)
            {
                next_item_modifier = 1;
                last_accel_tick = 0;
            }
        }
    }
#endif
#if defined(HAVE_TOUCHSCREEN)
    if (action == ACTION_TOUCHSCREEN)
        action = *actionptr = gui_synclist_do_touchscreen(lists);
    else if (action > ACTION_TOUCHSCREEN_MODE)
        /* cancel kinetic if we got a normal button event */
        _gui_synclist_stop_kinetic_scrolling(lists);
#endif

    /* Disable the skin redraw callback */
    current_lists = NULL;

    /* repeat actions block list wraparound */
    bool allow_wrap = lists->wraparound;

    switch (action)
    {
        case ACTION_REDRAW:
            gui_synclist_draw(lists);
            return true;

#ifdef SIMULATOR /* BUGFIX sim doesn't scroll lists from other threads */
        case ACTION_NONE:
        {
            extern struct scroll_screen_info lcd_scroll_info;
            struct scroll_screen_info *si = &lcd_scroll_info;

            for (int index = 0; index < si->lines; index++)
            {
                struct scrollinfo *s = &si->scroll[index];
                if (s->vp && (s->vp->flags & VP_FLAG_VP_DIRTY))
                {
                    s->vp->flags &= ~VP_FLAG_VP_SET_CLEAN;
                    lcd_update_viewport_rect(s->x, s->y, s->width, s->height);
                }
            }

            break;
        }
#endif

#ifdef HAVE_VOLUME_IN_LIST
        case ACTION_LIST_VOLUP:
            adjust_volume(1);
            return true;
        case ACTION_LIST_VOLDOWN:
            adjust_volume(-1);
            return true;
#endif
        case ACTION_STD_PREVREPEAT:
            allow_wrap = false; /* Prevent list wraparound on repeating actions */
            /*Fallthrough*/
        case ACTION_STD_PREV:
        
            gui_list_select_at_offset(lists, -next_item_modifier, allow_wrap);
#ifndef HAVE_WHEEL_ACCELERATION
            if (button_queue_count() < FRAMEDROP_TRIGGER)
#endif
                gui_synclist_draw(lists);
            yield();
            *actionptr = ACTION_STD_PREV;
            return true;

        case ACTION_STD_NEXTREPEAT:
            allow_wrap = false; /* Prevent list wraparound on repeating actions */
            /*Fallthrough*/
        case ACTION_STD_NEXT:
            gui_list_select_at_offset(lists, next_item_modifier, allow_wrap);
#ifndef HAVE_WHEEL_ACCELERATION
            if (button_queue_count() < FRAMEDROP_TRIGGER)
#endif
                gui_synclist_draw(lists);
            yield();
            *actionptr = ACTION_STD_NEXT;
            return true;

        case ACTION_TREE_PGRIGHT:
            gui_synclist_scroll_right(lists);
            gui_synclist_draw(lists);
            yield();
            return true;
        case ACTION_TREE_ROOT_INIT:
         /* After this button press ACTION_TREE_PGLEFT is allowed
            to skip to root. ACTION_TREE_ROOT_INIT must be defined in the
            keymaps as a repeated button press (the same as the repeated
            ACTION_TREE_PGLEFT) with the pre condition being the non-repeated
            button press. Leave out ACTION_TREE_ROOT_INIT in your keymaps to
            disable cancel action by PGLEFT key (e.g. if PGLEFT and CANCEL
            are mapped to different keys) */
            if (lists->offset_position[0] == 0)
            {
                pgleft_allow_cancel = true;
                *actionptr = ACTION_STD_MENU;
                return true;
            }
            *actionptr = ACTION_TREE_PGLEFT;
            /* fallthrough */
        case ACTION_TREE_PGLEFT:
            if(pgleft_allow_cancel && (lists->offset_position[0] == 0))
            {
                *actionptr = ACTION_STD_CANCEL;
                return false;
            }
            gui_synclist_scroll_left(lists);
            gui_synclist_draw(lists);
            pgleft_allow_cancel = false; /* stop ACTION_TREE_PAGE_LEFT
                                            skipping to root */
            yield();
            return true;
/* for pgup / pgdown, we are obliged to have a different behaviour depending
 * on the screen for which the user pressed the key since for example, remote
 * and main screen doesn't have the same number of lines */
        case ACTION_LISTTREE_PGUP:
        {
            int screen =
#ifdef HAVE_REMOTE_LCD
                get_action_statuscode(NULL)&ACTION_REMOTE ?
                         SCREEN_REMOTE :
#endif
                                          SCREEN_MAIN;
            gui_synclist_select_previous_page(lists, screen, false);
            gui_synclist_draw(lists);
            yield();
            *actionptr = ACTION_STD_NEXT;
        }
        return true;

        case ACTION_LISTTREE_PGDOWN:
        {
            int screen =
#ifdef HAVE_REMOTE_LCD
                get_action_statuscode(NULL)&ACTION_REMOTE ?
                         SCREEN_REMOTE :
#endif
                                          SCREEN_MAIN;
            gui_synclist_select_next_page(lists, screen, false);
            gui_synclist_draw(lists);
            yield();
            *actionptr = ACTION_STD_PREV;
        }
        return true;
    }
    if(lists->scheduled_talk_tick
       && TIME_AFTER(current_tick, lists->scheduled_talk_tick))
        /* scheduled postponed item announcement is due */
        _gui_synclist_speak_item(lists);
    return false;
}

int list_do_action_timeout(struct gui_synclist *lists, int timeout)
/* Returns the lowest of timeout or the delay until a postponed
   scheduled announcement is due (if any). */
{
    add_event_ex(GUI_EVENT_NEED_UI_UPDATE, true, _lists_uiviewport_update_callback, NULL);
    current_lists = lists;
    if(lists->scheduled_talk_tick)
    {
        long delay = lists->scheduled_talk_tick -current_tick +1;
        /* +1 because the trigger condition uses TIME_AFTER(), which
           is implemented as strictly greater than. */
        if(delay < 0)
            delay = 0;
        if(timeout > delay || timeout == TIMEOUT_BLOCK)
            timeout = delay;
    }
    return timeout;
}

bool list_do_action(int context, int timeout,
                    struct gui_synclist *lists, int *action)
/* Combines the get_action() (with possibly overridden timeout) and
   gui_synclist_do_button() calls. Returns the list action from
   do_button, and places the action from get_action in *action. */
{
    timeout = list_do_action_timeout(lists, timeout);
    keyclick_set_callback(gui_synclist_keyclick_callback, lists);
    *action = get_action(context, timeout);
    return gui_synclist_do_button(lists, action);
}

/* Simple use list implementation */
static int simplelist_line_count = 0, simplelist_line_remaining;
static int simplelist_line_pos;
/* buffer shared with bitmap/list code */
char simplelist_buffer[SIMPLELIST_MAX_LINES * SIMPLELIST_MAX_LINELENGTH];
static const char *simplelist_text[SIMPLELIST_MAX_LINES];
/* reset the amount of lines shown in the list to 0 */
void simplelist_reset_lines(void)
{
    simplelist_line_pos = 0;
    simplelist_line_remaining = sizeof(simplelist_buffer);
    simplelist_line_count = 0;
}
/* get the current amount of lines shown */
int simplelist_get_line_count(void)
{
    return simplelist_line_count;
}

/* set/edit a line pointer in the list. */
void simplelist_setline(const char *text)
{
    int line_number = simplelist_line_count++;
    if (simplelist_line_count >= SIMPLELIST_MAX_LINES)
        simplelist_line_count = 0;
    simplelist_text[line_number] = text;
}

/* add/edit a line in the list.
   if line_number > number of lines shown it adds the line,
   else it edits the line */
void simplelist_addline(const char *fmt, ...)
{
    va_list ap;
    size_t len = simplelist_line_remaining;

    char *bufpos = &simplelist_buffer[simplelist_line_pos];
    simplelist_setline(bufpos);

    va_start(ap, fmt);
    len = vsnprintf(bufpos, simplelist_line_remaining, fmt, ap);
    va_end(ap);
    len++;
    simplelist_line_remaining -= len;
    simplelist_line_pos += len;
}

static const char* simplelist_static_getname(int item,
                                             void * data,
                                             char *buffer,
                                             size_t buffer_len)
{
    (void)data; (void)buffer; (void)buffer_len;
    /* Note: buffer shouldn't be used..
     *  simplelist_buffer[] is already referenced by simplelist_text[] */
    return simplelist_text[item];
}

bool simplelist_show_list(struct simplelist_info *info)
{
    struct gui_synclist lists;
    int action, old_line_count = simplelist_line_count;
    list_get_name *getname;
    int line_count;

    if (info->get_name)
    {
        getname = info->get_name;
        line_count = info->count;
    }
    else
    {
        getname = simplelist_static_getname;
        line_count = simplelist_line_count;
    }

    FOR_NB_SCREENS(i)
        viewportmanager_theme_enable(i, !info->hide_theme, NULL);

    gui_synclist_init(&lists, getname,  info->callback_data,
                      info->scroll_all, info->selection_size, NULL);

    if (info->title)
        gui_synclist_set_title(&lists, info->title, info->title_icon);

    gui_synclist_set_icon_callback(&lists, info->get_icon);
    gui_synclist_set_voice_callback(&lists, info->get_talk);
#ifdef HAVE_LCD_COLOR
    gui_synclist_set_color_callback(&lists, info->get_color);
    if (info->selection_color)
        gui_synclist_set_sel_color(&lists, info->selection_color);
#endif

    if (info->action_callback)
        info->action_callback(ACTION_REDRAW, &lists);

    gui_synclist_set_nb_items(&lists, line_count*info->selection_size);

    gui_synclist_select_item(&lists, info->selection);

    gui_synclist_draw(&lists);

    if (info->speak_onshow)
        gui_synclist_speak_item(&lists);

    while(1)
    {
        list_do_action(CONTEXT_LIST, info->timeout, &lists, &action);

        /* We must yield in this case or no other thread can run */
        if (info->timeout == TIMEOUT_NOBLOCK)
            yield();

        if (info->action_callback)
        {
            bool stdok = action==ACTION_STD_OK;
            action = info->action_callback(action, &lists);
            if (stdok && action == ACTION_STD_CANCEL)
            {
                /* callback asked us to exit */
                info->selection = gui_synclist_get_sel_pos(&lists);
                break;
            }

            if (info->get_name == NULL)
                gui_synclist_set_nb_items(&lists,
                        simplelist_line_count*info->selection_size);
        }
        if (action == ACTION_STD_CANCEL)
        {
            info->selection = -1;
            break;
        }
        else if (action == ACTION_STD_OK)
        {
            info->selection = gui_synclist_get_sel_pos(&lists);
            break;
        }
        else if ((action == ACTION_REDRAW) ||
                 (list_is_dirty(&lists)) ||
                 (old_line_count != simplelist_line_count))
        {
            if (info->get_name == NULL)
            {
                gui_synclist_set_nb_items(&lists,
                        simplelist_line_count*info->selection_size);
            }
            gui_synclist_draw(&lists);
            old_line_count = simplelist_line_count;
        }
        else if(default_event_handler(action) == SYS_USB_CONNECTED)
        {
            return true;
        }
    }
    talk_shutup();

#ifdef HAVE_LCD_COLOR
    if (info->selection_color)
        gui_synclist_set_sel_color(&lists, NULL);
#endif

    FOR_NB_SCREENS(i)
        viewportmanager_theme_undo(i, false);
    return false;
}

void simplelist_info_init(struct simplelist_info *info, char* title,
                          int count, void* data)
{
    info->title = title;
    info->count = count;
    info->selection_size = 1;
    info->scroll_all = false;
    info->hide_theme = false;
    info->speak_onshow = true;
    info->timeout = HZ/10;
    info->selection = 0;
    info->action_callback = NULL;
    info->title_icon = Icon_NOICON;
    info->get_icon = NULL;
    info->get_name = NULL;
    info->get_talk = NULL;
#ifdef HAVE_LCD_COLOR
    info->get_color = NULL;
    info->selection_color = NULL;
#endif
    info->callback_data = data;
    simplelist_line_count = 0;
}
