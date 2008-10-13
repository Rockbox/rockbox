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

#include "config.h"
#include "lcd.h"
#include "font.h"
#include "button.h"
#include "sprintf.h"
#include "string.h"
#include "settings.h"
#include "kernel.h"
#include "system.h"

#include "action.h"
#include "screen_access.h"
#include "list.h"
#include "scrollbar.h"
#include "statusbar.h"
#include "lang.h"
#include "sound.h"
#include "misc.h"
#include "talk.h"
#include "viewport.h"
#include "list.h"

#ifdef HAVE_LCD_CHARCELLS
#define SCROLL_LIMIT 1
#else
#define SCROLL_LIMIT (nb_lines<3?1:2)
#endif

/* The minimum number of pending button events in queue before starting
 * to limit list drawing interval.
 */
#define FRAMEDROP_TRIGGER 6

#ifdef HAVE_LCD_BITMAP
static int offset_step = 16; /* pixels per screen scroll step */
/* should lines scroll out of the screen */
static bool offset_out_of_view = false;
#endif
static int force_list_reinit = false;

static void gui_list_select_at_offset(struct gui_synclist * gui_list,
                                      int offset);
void list_draw(struct screen *display, struct viewport *parent, struct gui_synclist *list);

#ifdef HAVE_LCD_BITMAP
static struct viewport parent[NB_SCREENS];
void list_init_viewports(struct gui_synclist *list)
{
    int i;
    struct viewport *vp;
    FOR_NB_SCREENS(i)
    {
        vp = &parent[i];
        if (!list)
            viewport_set_defaults(vp, i);
        else if (list->parent[i] == vp)
        {
            viewport_set_defaults(vp, i);
            list->parent[i]->y = gui_statusbar_height();
            list->parent[i]->height = screens[i].lcdheight - list->parent[i]->y;
        }
    }
#ifdef HAVE_BUTTONBAR
    if (list && (list->parent[0] == &parent[0]) && global_settings.buttonbar)
        list->parent[0]->height -= BUTTONBAR_HEIGHT;
#endif
    force_list_reinit = false;
}
#else
static struct viewport parent[NB_SCREENS] =
{
    [SCREEN_MAIN] = 
    {
        .x        = 0,
        .y        = 0,
        .width    = LCD_WIDTH,
        .height   = LCD_HEIGHT
    },
};
void list_init_viewports(struct gui_synclist *list)
{
    (void)list;
    force_list_reinit = false;
}
#endif

#ifdef HAVE_LCD_BITMAP
bool list_display_title(struct gui_synclist *list, struct viewport *vp)
{
    return list->title != NULL && viewport_get_nb_lines(vp)>2;
}
#else
#define list_display_title(l,v) false
#endif

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
    int i;
    gui_list->callback_get_item_icon = NULL;
    gui_list->callback_get_item_name = callback_get_item_name;
    gui_list->callback_speak_item = NULL;
    gui_list->nb_items = 0;
    gui_list->selected_item = 0;
    FOR_NB_SCREENS(i)
    {
        gui_list->start_item[i] = 0;
        gui_list->last_displayed_start_item[i] = -1 ;
#ifdef HAVE_LCD_BITMAP
        gui_list->offset_position[i] = 0;
#endif
        if (list_parent)
            gui_list->parent[i] = &list_parent[i];
        else
        {
            gui_list->parent[i] = &parent[i];
        }
    }
    list_init_viewports(gui_list);
    gui_list->limit_scroll = false;
    gui_list->data=data;
    gui_list->scroll_all=scroll_all;
    gui_list->selected_size=selected_size;
    gui_list->title = NULL;
    gui_list->title_width = 0;
    gui_list->title_icon = Icon_NOICON;

    gui_list->scheduled_talk_tick = gui_list->last_talked_tick = 0;
    gui_list->show_selection_marker = true;
    gui_list->last_displayed_selected_item = -1 ;

#ifdef HAVE_LCD_COLOR
    gui_list->title_color = -1;
    gui_list->callback_get_item_color = NULL;
#endif
    force_list_reinit = true;
}

/* this toggles the selection bar or cursor */
void gui_synclist_hide_selection_marker(struct gui_synclist * lists, bool hide)
{
    lists->show_selection_marker = !hide;
}


#ifdef HAVE_LCD_BITMAP
int list_title_height(struct gui_synclist *list, struct viewport *vp);

int gui_list_get_item_offset(struct gui_synclist * gui_list, int item_width,
                             int text_pos, struct screen * display, struct viewport *vp)
{
    int item_offset;

    if (offset_out_of_view)
    {
        item_offset = gui_list->offset_position[display->screen_type];
    }
    else
    {
        /* if text is smaller then view */
        if (item_width <= vp->width - text_pos)
        {
            item_offset = 0;
        }
        else
        {
            /* if text got out of view  */
            if (gui_list->offset_position[display->screen_type] >
                    item_width - (vp->width - text_pos))
                item_offset = item_width - (vp->width - text_pos);
            else
                item_offset = gui_list->offset_position[display->screen_type];
        }
    }

    return item_offset;
}
#endif
/*
 * Force a full screen update.
 */
 
void gui_synclist_draw(struct gui_synclist *gui_list)
{
    int i;
    static struct gui_synclist *last_list = NULL;
    static int last_count = -1;
#ifdef HAVE_BUTTONBAR
    static bool last_buttonbar = false;
#endif
    if (force_list_reinit ||
#ifdef HAVE_BUTTONBAR
        last_buttonbar != screens[SCREEN_MAIN].has_buttonbar ||
#endif
        last_list != gui_list ||
        gui_list->nb_items != last_count)
    {
        list_init_viewports(gui_list);
        gui_synclist_select_item(gui_list, gui_list->selected_item);
    }
#ifdef HAVE_BUTTONBAR
    last_buttonbar = screens[SCREEN_MAIN].has_buttonbar;
#endif
    last_count = gui_list->nb_items;
    last_list = gui_list;
    FOR_NB_SCREENS(i)
    {
        list_draw(&screens[i], gui_list->parent[i], gui_list);
    }
}

/* sets up the list so the selection is shown correctly on the screen */
static void gui_list_put_selection_on_screen(struct gui_synclist * gui_list,
                                             enum screen_type screen)
{
    int nb_lines;
    int difference = gui_list->selected_item - gui_list->start_item[screen];
    struct viewport vp = *gui_list->parent[screen];
#ifdef HAVE_LCD_BITMAP
    if (list_display_title(gui_list, gui_list->parent[screen]))
        vp.height -= list_title_height(gui_list,gui_list->parent[screen]);
#endif
    nb_lines = viewport_get_nb_lines(&vp);
    
    /* edge case,, selected last item */
    if (gui_list->selected_item == gui_list->nb_items -1)
    {
        gui_list->start_item[screen] = MAX(0, gui_list->nb_items - nb_lines);
    }
    /* selected first item */
    else if (gui_list->selected_item == 0)
    {
        gui_list->start_item[screen] = 0;
    }
    else if (difference < SCROLL_LIMIT) /* list moved up */
    {
        if (global_settings.scroll_paginated)
        {
            if (gui_list->start_item[screen] > gui_list->selected_item)
                gui_list->start_item[screen] = (gui_list->selected_item/nb_lines)*nb_lines;
            if (gui_list->nb_items <= nb_lines)
                gui_list->start_item[screen] = 0;
        }
        else
        {
            int top_of_screen = gui_list->selected_item - SCROLL_LIMIT + 1;
            int temp = MIN(top_of_screen, gui_list->nb_items - nb_lines);
            gui_list->start_item[screen] = MAX(0, temp);
        }
    }
    else if (difference > nb_lines - SCROLL_LIMIT) /* list moved down */
    {
        int bottom = gui_list->nb_items - nb_lines;
        /* always move the screen if selection isnt "visible" */
        if (gui_list->show_selection_marker == false)
        {
            if (bottom < 0)
                bottom = 0;
            gui_list->start_item[screen] = MIN(bottom, gui_list->start_item[screen] + 
                                                       2*gui_list->selected_size);
        }
        else if (global_settings.scroll_paginated)
        {
            if (gui_list->start_item[screen] + nb_lines <= gui_list->selected_item)
                gui_list->start_item[screen] = MIN(bottom, gui_list->selected_item);
        }
        else
        {
            int top_of_screen = gui_list->selected_item + SCROLL_LIMIT - nb_lines;
            int temp = MAX(0, top_of_screen);
            gui_list->start_item[screen] = MIN(bottom, temp);
        }
    }
}
/*
 * Selects an item in the list
 *  - gui_list : the list structure
 *  - item_number : the number of the item which will be selected
 */
void gui_synclist_select_item(struct gui_synclist * gui_list, int item_number)
{
    int i;
    if( item_number > gui_list->nb_items-1 || item_number < 0 )
        return;
    gui_list->selected_item = item_number;
    FOR_NB_SCREENS(i)
        gui_list_put_selection_on_screen(gui_list, i);
}

static void gui_list_select_at_offset(struct gui_synclist * gui_list,
                                      int offset)
{
    int new_selection;
    if (gui_list->selected_size > 1)
    {
        offset *= gui_list->selected_size;
        /* always select the first item of multi-line lists */
        offset -= offset%gui_list->selected_size;
    }
    new_selection = gui_list->selected_item + offset;
    if (new_selection >= gui_list->nb_items)
    {
        gui_list->selected_item = gui_list->limit_scroll ?
          gui_list->nb_items - gui_list->selected_size : 0;
    }
    else if (new_selection < 0)
    {
        gui_list->selected_item = gui_list->limit_scroll ?
          0 : gui_list->nb_items - gui_list->selected_size;
    }
    else if (gui_list->show_selection_marker == false)
    {
        int i, nb_lines, screen_top;
        FOR_NB_SCREENS(i)
        {
            struct viewport vp = *gui_list->parent[i];
#ifdef HAVE_LCD_BITMAP
            if (list_display_title(gui_list, gui_list->parent[i]))
                vp.height -= list_title_height(gui_list,gui_list->parent[i]);
#endif
            nb_lines = viewport_get_nb_lines(&vp);
            if (offset > 0)
            {
                screen_top = gui_list->nb_items-nb_lines;
                if (screen_top < 0)
                    screen_top = 0;
                gui_list->start_item[i] = MIN(screen_top, gui_list->start_item[i] + 
                                                gui_list->selected_size);
                gui_list->selected_item = gui_list->start_item[i];
            }
            else
            {
                gui_list->start_item[i] = MAX(0, gui_list->start_item[i] - 
                                                    gui_list->selected_size);
                gui_list->selected_item = gui_list->start_item[i] + nb_lines;
            }
        }
        return;
    }
    else gui_list->selected_item += offset;
    gui_synclist_select_item(gui_list, gui_list->selected_item);
}

/*
 * Adds an item to the list (the callback will be asked for one more item)
 * - gui_list : the list structure
 */
void gui_synclist_add_item(struct gui_synclist * gui_list)
{
    gui_list->nb_items++;
    /* if only one item in the list, select it */
    if(gui_list->nb_items == 1)
        gui_list->selected_item = 0;
}

/*
 * Removes an item to the list (the callback will be asked for one less item)
 * - gui_list : the list structure
 */
void gui_synclist_del_item(struct gui_synclist * gui_list)
{
    if(gui_list->nb_items > 0)
    {
        if (gui_list->selected_item == gui_list->nb_items-1)
            gui_list->selected_item--;
        gui_list->nb_items--;
        gui_synclist_select_item(gui_list, gui_list->selected_item);
    }
}

#ifdef HAVE_LCD_BITMAP
void gui_list_screen_scroll_step(int ofs)
{
     offset_step = ofs;
}

void gui_list_screen_scroll_out_of_view(bool enable)
{
    if (enable)
        offset_out_of_view = true;
    else
        offset_out_of_view = false;
}
#endif /* HAVE_LCD_BITMAP */

/* 
 * Set the title and title icon of the list. Setting title to NULL disables
 * both the title and icon. Use NOICON if there is no icon.
 */
void gui_synclist_set_title(struct gui_synclist * gui_list, 
                               char * title, enum themable_icons icon)
{
    gui_list->title = title;
    gui_list->title_icon = icon;
    if (title) {
#ifdef HAVE_LCD_BITMAP
        int i;
        FOR_NB_SCREENS(i)
            screens[i].getstringsize(title, &gui_list->title_width, NULL);
#else
        gui_list->title_width = strlen(title);
#endif
    } else {
        gui_list->title_width = 0;
    }
    force_list_reinit = true;
}


void gui_synclist_set_nb_items(struct gui_synclist * lists, int nb_items)
{
#ifdef HAVE_LCD_BITMAP
    int i;
#endif
    lists->nb_items = nb_items;
#ifdef HAVE_LCD_BITMAP
    FOR_NB_SCREENS(i)
    {
        lists->offset_position[i] = 0;
    }
#endif
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

#ifdef HAVE_LCD_COLOR
void gui_synclist_set_color_callback(struct gui_synclist * lists,
                                    list_get_color color_callback)
{
    lists->callback_get_item_color = color_callback;
}
#endif

static void gui_synclist_select_next_page(struct gui_synclist * lists,
                                   enum screen_type screen)
{
    int nb_lines = viewport_get_nb_lines(lists->parent[screen]);
    gui_list_select_at_offset(lists, nb_lines);
}

static void gui_synclist_select_previous_page(struct gui_synclist * lists,
                                       enum screen_type screen)
{
    int nb_lines = viewport_get_nb_lines(lists->parent[screen]);
    gui_list_select_at_offset(lists, -nb_lines);
}

void gui_synclist_limit_scroll(struct gui_synclist * lists, bool scroll)
{
    lists->limit_scroll = scroll;
}

#ifdef HAVE_LCD_BITMAP
/*
 * Makes all the item in the list scroll by one step to the right.
 * Should stop increasing the value when reaching the widest item value 
 * in the list.
 */
static void gui_synclist_scroll_right(struct gui_synclist * lists)
{
    int i;
    FOR_NB_SCREENS(i)
    {
        /* FIXME: This is a fake right boundry limiter. there should be some
        * callback function to find the longest item on the list in pixels,
        * to stop the list from scrolling past that point */
        lists->offset_position[i]+=offset_step;
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
    int i;
    FOR_NB_SCREENS(i)
    {
        lists->offset_position[i]-=offset_step;
        if (lists->offset_position[i] < 0)
            lists->offset_position[i] = 0;
    }
}
#endif /* HAVE_LCD_BITMAP */

static void _gui_synclist_speak_item(struct gui_synclist *lists, bool repeating)
{
    list_speak_item *cb = lists->callback_speak_item;
    if(cb && gui_synclist_get_nb_items(lists) != 0)
    {
        int sel = gui_synclist_get_sel_pos(lists);
        talk_shutup();
        /* If we got a repeating key action, or we have just very
           recently started talking, then we want to stay silent for a
           while until things settle. Likewise if we already had a
           pending scheduled announcement not yet due: we need to
           reschedule it. */
        if(repeating
           || (lists->scheduled_talk_tick
               && TIME_BEFORE(current_tick, lists->scheduled_talk_tick))
           || (lists->last_talked_tick
               && TIME_BEFORE(current_tick, lists->last_talked_tick +HZ/4)))
        {
            lists->scheduled_talk_tick = current_tick +HZ/4;
            return;
        } else {
            lists->scheduled_talk_tick = 0; /* work done */
            cb(sel, lists->data);
            lists->last_talked_tick = current_tick;
        }
    }
}
void gui_synclist_speak_item(struct gui_synclist * lists)
/* The list user should call this to speak the first item on entering
   the list, and whenever the list is updated. */
{
    if(gui_synclist_get_nb_items(lists) == 0 && global_settings.talk_menu)
        talk_id(VOICE_EMPTY_LIST, true);
    else _gui_synclist_speak_item(lists, false);
}

extern intptr_t get_action_data(void);
#if  defined(HAVE_TOUCHSCREEN)
/* this needs to be fixed if we ever get more than 1 touchscreen on a target */
unsigned gui_synclist_do_touchscreen(struct gui_synclist * gui_list, struct viewport *parent);
#endif

bool gui_synclist_do_button(struct gui_synclist * lists,
                            unsigned *actionptr, enum list_wrap wrap)
{
    int action = *actionptr;
#ifdef HAVE_LCD_BITMAP
    static bool scrolling_left = false;
#endif

#ifdef HAVE_SCROLLWHEEL
    int next_item_modifier = button_apply_acceleration(get_action_data());
#else
    static int next_item_modifier = 1;
    static int last_accel_tick = 0;

    if (global_settings.list_accel_start_delay)
    {
        int start_delay = global_settings.list_accel_start_delay * (HZ/2);
        int accel_wait = global_settings.list_accel_wait * HZ/2;

        if (get_action_statuscode(NULL)&ACTION_REPEAT)
        {
            if (!last_accel_tick)
                last_accel_tick = current_tick + start_delay;
            else if (current_tick >= 
                        last_accel_tick + accel_wait)
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
#endif
    
#if defined(HAVE_TOUCHSCREEN)
    if (action == ACTION_TOUCHSCREEN)
        action = *actionptr = gui_synclist_do_touchscreen(lists, &parent[SCREEN_MAIN]);
#endif

    switch (wrap)
    {
        case LIST_WRAP_ON:
            gui_synclist_limit_scroll(lists, false);
        break;
        case LIST_WRAP_OFF:
            gui_synclist_limit_scroll(lists, true);
        break;
        case LIST_WRAP_UNLESS_HELD:
            if (action == ACTION_STD_PREVREPEAT ||
                action == ACTION_STD_NEXTREPEAT ||
                action == ACTION_LISTTREE_PGUP  ||
                action == ACTION_LISTTREE_PGDOWN)
                gui_synclist_limit_scroll(lists, true);
            else gui_synclist_limit_scroll(lists, false);
        break;
    };

    switch (action)
    {
        case ACTION_REDRAW:
            gui_synclist_draw(lists);
            return true;
            
#ifdef HAVE_VOLUME_IN_LIST
        case ACTION_LIST_VOLUP:
            global_settings.volume += 2; 
            /* up two because the falthrough brings it down one */
        case ACTION_LIST_VOLDOWN:
            global_settings.volume--;
            setvol();
            return true;
#endif
        case ACTION_STD_PREV:
        case ACTION_STD_PREVREPEAT:
                gui_list_select_at_offset(lists, -next_item_modifier);
#ifndef HAVE_SCROLLWHEEL
            if (button_queue_count() < FRAMEDROP_TRIGGER)
#endif
                gui_synclist_draw(lists);
            _gui_synclist_speak_item(lists,
                                     action == ACTION_STD_PREVREPEAT
                                     || next_item_modifier >1);
            yield();
            *actionptr = ACTION_STD_PREV;
            return true;

        case ACTION_STD_NEXT:
        case ACTION_STD_NEXTREPEAT:
                gui_list_select_at_offset(lists, next_item_modifier);
#ifndef HAVE_SCROLLWHEEL
            if (button_queue_count() < FRAMEDROP_TRIGGER)
#endif
                gui_synclist_draw(lists);
            _gui_synclist_speak_item(lists,
                                     action == ACTION_STD_NEXTREPEAT
                                     || next_item_modifier >1);
            yield();
            *actionptr = ACTION_STD_NEXT;
            return true;

#ifdef HAVE_LCD_BITMAP
        case ACTION_TREE_PGRIGHT:
            gui_synclist_scroll_right(lists);
            gui_synclist_draw(lists);
            return true;
        case ACTION_TREE_ROOT_INIT:
         /* After this button press ACTION_TREE_PGLEFT is allowed
            to skip to root. ACTION_TREE_ROOT_INIT must be defined in the
            keymaps as a repeated button press (the same as the repeated
            ACTION_TREE_PGLEFT) with the pre condition being the non-repeated
            button press */
            if (lists->offset_position[0] == 0)
            {
                scrolling_left = false;
                *actionptr = ACTION_STD_CANCEL;
                return true;
            }
            *actionptr = ACTION_TREE_PGLEFT;
        case ACTION_TREE_PGLEFT:
            if(!scrolling_left && (lists->offset_position[0] == 0))
            {
                *actionptr = ACTION_STD_CANCEL;
                return false;
            }
            gui_synclist_scroll_left(lists);
            gui_synclist_draw(lists);
            scrolling_left = true; /* stop ACTION_TREE_PAGE_LEFT
                                      skipping to root */
            return true;
#endif
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
            gui_synclist_select_previous_page(lists, screen);
            gui_synclist_draw(lists);
            _gui_synclist_speak_item(lists, false);
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
            gui_synclist_select_next_page(lists, screen);
            gui_synclist_draw(lists);
            _gui_synclist_speak_item(lists, false);
            yield();
            *actionptr = ACTION_STD_PREV;
        }
        return true;
    }
    if(lists->scheduled_talk_tick
       && TIME_AFTER(current_tick, lists->scheduled_talk_tick))
        /* scheduled postponed item announcement is due */
        _gui_synclist_speak_item(lists, false);
    return false;
}

int list_do_action_timeout(struct gui_synclist *lists, int timeout)
/* Returns the lowest of timeout or the delay until a postponed
   scheduled announcement is due (if any). */
{
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
                    struct gui_synclist *lists, int *action,
                    enum list_wrap wrap)
/* Combines the get_action() (with possibly overridden timeout) and
   gui_synclist_do_button() calls. Returns the list action from
   do_button, and places the action from get_action in *action. */
{
    timeout = list_do_action_timeout(lists, timeout);
    *action = get_action(context, timeout);
    return gui_synclist_do_button(lists, action, wrap);
}

bool gui_synclist_item_is_onscreen(struct gui_synclist *lists,
                                   enum screen_type screen, int item)
{
    struct viewport vp = *lists->parent[screen];
#ifdef HAVE_LCD_BITMAP
    if (list_display_title(lists, lists->parent[screen]))
        vp.height -= list_title_height(lists, lists->parent[screen]);
#endif
    return item <= (lists->start_item[screen] + viewport_get_nb_lines(&vp));
}

/* Simple use list implementation */
static int simplelist_line_count = 0;
static char simplelist_text[SIMPLELIST_MAX_LINES][SIMPLELIST_MAX_LINELENGTH];
/* set the amount of lines shown in the list */
void simplelist_set_line_count(int lines)
{
    if (lines < 0)
        simplelist_line_count = 0;
    else if (lines >= SIMPLELIST_MAX_LINES)
        simplelist_line_count = SIMPLELIST_MAX_LINES;
    else
        simplelist_line_count = lines;
}
/* get the current amount of lines shown */
int simplelist_get_line_count(void)
{
    return simplelist_line_count;
}
/* add/edit a line in the list.
   if line_number > number of lines shown it adds the line, else it edits the line */
void simplelist_addline(int line_number, const char *fmt, ...)
{
    va_list ap;

    if (line_number > simplelist_line_count)
    {
        if (simplelist_line_count < SIMPLELIST_MAX_LINES)
            line_number = simplelist_line_count++;
        else 
             return;
    }
    va_start(ap, fmt);
    vsnprintf(simplelist_text[line_number], SIMPLELIST_MAX_LINELENGTH, fmt, ap);
    va_end(ap);
}

static char* simplelist_static_getname(int item,
                                       void * data,
                                       char *buffer,
                                       size_t buffer_len)
{
    (void)data; (void)buffer; (void)buffer_len;
    return simplelist_text[item];
}

bool simplelist_show_list(struct simplelist_info *info)
{
    struct gui_synclist lists;
    struct viewport vp[NB_SCREENS];
    int action, old_line_count = simplelist_line_count,i;
    char* (*getname)(int item, void * data, char *buffer, size_t buffer_len);
    int wrap = LIST_WRAP_UNLESS_HELD;
    if (info->get_name)
        getname = info->get_name;
    else
        getname = simplelist_static_getname;
    FOR_NB_SCREENS(i)
    {
        viewport_set_defaults(&vp[i], i);
    }
    gui_synclist_init(&lists, getname,  info->callback_data, 
                      info->scroll_all, info->selection_size, vp);
    
    if (info->title)
        gui_synclist_set_title(&lists, info->title, NOICON);
    if (info->get_icon)
        gui_synclist_set_icon_callback(&lists, info->get_icon);
    if (info->get_talk)
        gui_synclist_set_voice_callback(&lists, info->get_talk);
    
    if (info->hide_selection)
    {
        gui_synclist_hide_selection_marker(&lists, true);
        wrap = LIST_WRAP_OFF;
    }
    
    if (info->action_callback)
        info->action_callback(ACTION_REDRAW, &lists);

    if (info->get_name == NULL)
        gui_synclist_set_nb_items(&lists, simplelist_line_count*info->selection_size);
    else
        gui_synclist_set_nb_items(&lists, info->count*info->selection_size);
    
    gui_synclist_select_item(&lists, info->selection);
    
    gui_synclist_draw(&lists);
    gui_synclist_speak_item(&lists);

    while(1)
    {
        gui_syncstatusbar_draw(&statusbars, true);
        list_do_action(CONTEXT_STD, info->timeout,
                       &lists, &action, wrap);

        /* We must yield in this case or no other thread can run */
        if (info->timeout == TIMEOUT_NOBLOCK)
            yield();

        if (info->action_callback)
        {
            bool stdok = action==ACTION_STD_OK;
            action = info->action_callback(action, &lists);
            if (stdok && action == ACTION_STD_CANCEL) /* callback asked us to exit */
            {
                info->selection = gui_synclist_get_sel_pos(&lists);
                break;
            }
                
            if (info->get_name == NULL)
                gui_synclist_set_nb_items(&lists, simplelist_line_count*info->selection_size);
        }
        if (action == ACTION_STD_CANCEL)
        {
            info->selection = -1;
            break;
        }
        else if ((action == ACTION_REDRAW) ||
                 (old_line_count != simplelist_line_count))
        {
            if (info->get_name == NULL)
                gui_synclist_set_nb_items(&lists, simplelist_line_count*info->selection_size);
            gui_synclist_draw(&lists);
            old_line_count = simplelist_line_count;
        }
        else if(default_event_handler(action) == SYS_USB_CONNECTED)
            return true;
    }
    talk_shutup();
    return false;
}

void simplelist_info_init(struct simplelist_info *info, char* title,
                          int count, void* data)
{
    info->title = title;
    info->count = count;
    info->selection_size = 1;
    info->hide_selection = false;
    info->scroll_all = false;
    info->timeout = HZ/10;
    info->selection = 0;
    info->action_callback = NULL;
    info->get_icon = NULL;
    info->get_name = NULL;
    info->get_talk = NULL;
    info->callback_data = data;
}







