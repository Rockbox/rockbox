/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Jonathan Gordon
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

/* This file contains the code to draw the list widget on BITMAP LCDs. */

#include "config.h"
#include "system.h"
#include "lcd.h"
#include "font.h"
#include "button.h"
#include "string.h"
#include "settings.h"
#include "kernel.h"
#include "file.h"

#include "action.h"
#include "screen_access.h"
#include "list.h"
#include "scrollbar.h"
#include "lang.h"
#include "sound.h"
#include "misc.h"
#include "viewport.h"
#include "statusbar-skinned.h"
#include "debug.h"
#include "line.h"

#define ICON_PADDING 1
#define ICON_PADDING_S "1"

/* these are static to make scrolling work */
static struct viewport list_text[NB_SCREENS], title_text[NB_SCREENS];

#ifdef HAVE_TOUCHSCREEN
static int y_offset;
static bool hide_selection;
#endif

/* list-private helpers from the generic list.c (move to header?) */
int gui_list_get_item_offset(struct gui_synclist * gui_list, int item_width,
                             int text_pos, struct screen * display,
                             struct viewport *vp);
bool list_display_title(struct gui_synclist *list, enum screen_type screen);
int list_get_nb_lines(struct gui_synclist *list, enum screen_type screen);

void gui_synclist_scroll_stop(struct gui_synclist *lists)
{
    FOR_NB_SCREENS(i)
    {
        screens[i].scroll_stop_viewport(&list_text[i]);
        screens[i].scroll_stop_viewport(&title_text[i]);
        screens[i].scroll_stop_viewport(lists->parent[i]);
    }
}

/* Draw the list...
    internal screen layout:
        -----------------
        |TI|  title     |   TI is title icon
        -----------------
        | | |            |
        |S|I|            |   S - scrollbar
        | | | items      |   I - icons
        | | |            |
        ------------------

        Note: This image is flipped horizontally when the language is a
        right-to-left one (Hebrew, Arabic)
*/

static int list_icon_width(enum screen_type screen)
{
    return get_icon_width(screen) + ICON_PADDING * 2;
}

static bool draw_title(struct screen *display, struct gui_synclist *list)
{
    const int screen = display->screen_type;
    struct viewport *title_text_vp = &title_text[screen];
    struct line_desc line = LINE_DESC_DEFINIT;

    if (sb_set_title_text(list->title, list->title_icon, screen))
        return false; /* the sbs is handling the title */
    display->scroll_stop_viewport(title_text_vp);
    if (!list_display_title(list, screen))
        return false;
    *title_text_vp = *(list->parent[screen]);
    line.height = list->line_height[screen];
    title_text_vp->height = line.height;

#if LCD_DEPTH > 1
    /* XXX: Do we want to support the separator on remote displays? */
    if (display->screen_type == SCREEN_MAIN && global_settings.list_separator_height != 0)
        line.separator_height = abs(global_settings.list_separator_height)
                                + (lcd_get_dpi() > 200 ? 2 : 1);
#endif

#ifdef HAVE_LCD_COLOR
    if (list->title_color >= 0)
        line.style |= (STYLE_COLORED|list->title_color);
#endif
    line.scroll = true;

    display->set_viewport(title_text_vp);

    if (list->title_icon != Icon_NOICON && global_settings.show_icons)
        put_line(display, 0, 0, &line, "$"ICON_PADDING_S"I$t",
                 list->title_icon, list->title);
    else
        put_line(display, 0, 0, &line, "$t", list->title);

    return true;
}

void list_draw(struct screen *display, struct gui_synclist *list)
{
    int start, end, item_offset, i;
    const int screen = display->screen_type;
    const int list_start_item = list->start_item[screen];
    const bool scrollbar_in_left = (global_settings.scrollbar == SCROLLBAR_LEFT);
    const bool scrollbar_in_right = (global_settings.scrollbar == SCROLLBAR_RIGHT);
    const bool show_cursor = !global_settings.cursor_style &&
                        list->show_selection_marker;
    const bool have_icons = global_settings.show_icons && list->callback_get_item_icon;
    struct viewport *parent = (list->parent[screen]);
    struct line_desc linedes = LINE_DESC_DEFINIT;
    bool show_title;
    struct viewport *list_text_vp = &list_text[screen];
    int indent = 0;

    struct viewport * last_vp = display->set_viewport(parent);
    display->clear_viewport();
    display->scroll_stop_viewport(list_text_vp);
    *list_text_vp = *parent;
    if ((show_title = draw_title(display, list)))
    {
        int title_height = title_text[screen].height;
        list_text_vp->y += title_height;
        list_text_vp->height -= title_height;
    }

    const int nb_lines = list_get_nb_lines(list, screen);

    linedes.height = list->line_height[screen];
    linedes.nlines = list->selected_size;
#if LCD_DEPTH > 1
    /* XXX: Do we want to support the separator on remote displays? */
    if (display->screen_type == SCREEN_MAIN)
        linedes.separator_height = abs(global_settings.list_separator_height);
#endif
    start = list_start_item;
    end = start + nb_lines;

#ifdef HAVE_TOUCHSCREEN
    if (list->selected_item == 0 || (list->nb_items < nb_lines))
        y_offset = 0; /* reset in case it's a new list */

    int draw_offset = y_offset;
    /* draw some extra items to not have empty lines at the top and bottom */
    if (y_offset > 0)
    {
        /* make it negative for more consistent apparence when switching
         * directions */
        draw_offset -= linedes.height;
        if (start > 0)
            start--;
    }
    else if (y_offset < 0)
        end++;
#else
    #define draw_offset 0
#endif

    /* draw the scrollbar if its needed */
    if (global_settings.scrollbar != SCROLLBAR_OFF)
    {
        /* if the scrollbar is shown the text viewport needs to shrink */
        if (nb_lines < list->nb_items)
        {
            struct viewport vp = *list_text_vp;
            vp.width = SCROLLBAR_WIDTH;
            vp.height = linedes.height * nb_lines;
            list_text_vp->width -= SCROLLBAR_WIDTH;
            if (scrollbar_in_right)
                vp.x += list_text_vp->width;
            else /* left */
                list_text_vp->x += SCROLLBAR_WIDTH;
            display->set_viewport(&vp);
            gui_scrollbar_draw(display,
                    (scrollbar_in_left? 0: 1), 0, SCROLLBAR_WIDTH-1, vp.height,
                    list->nb_items, list_start_item, list_start_item + nb_lines,
                    VERTICAL);
        }
        /* shift everything a bit in relation to the title */
        else if (!VP_IS_RTL(list_text_vp) && scrollbar_in_left)
            indent += SCROLLBAR_WIDTH;
        else if (VP_IS_RTL(list_text_vp) && scrollbar_in_right)
            indent += SCROLLBAR_WIDTH;
    }

    for (i=start; i<end && i<list->nb_items; i++)
    {
        /* do the text */
        enum themable_icons icon;
        unsigned const char *s;
        char entry_buffer[MAX_PATH];
        unsigned char *entry_name;
        int text_pos = 0;
        int line = i - start;
        int line_indent = 0;
        int style = STYLE_DEFAULT;
        bool is_selected = false;
        s = list->callback_get_item_name(i, list->data, entry_buffer,
                                         sizeof(entry_buffer));
        entry_name = P2STR(s);

        while (*entry_name == '\t')
        {
            line_indent++;
            entry_name++;
        }
        if (line_indent)
        {
            if (global_settings.show_icons)
                line_indent *= list_icon_width(screen);
            else
                line_indent *= display->getcharwidth();
        }
        line_indent += indent;

        display->set_viewport(list_text_vp);
        /* position the string at the correct offset place */
        int item_width,h;
        display->getstringsize(entry_name, &item_width, &h);
        item_offset = gui_list_get_item_offset(list, item_width, text_pos,
                display, list_text_vp);

        /* draw the selected line */
        if(
#ifdef HAVE_TOUCHSCREEN
            /* don't draw it during scrolling */
            !hide_selection &&
#endif
                i >= list->selected_item
                && i <  list->selected_item + list->selected_size
                && list->show_selection_marker)
        {/* The selected item must be displayed scrolling */
#ifdef HAVE_LCD_COLOR
            if (list->selection_color)
            {
                /* Display gradient line selector */
                style = STYLE_GRADIENT;
                linedes.text_color = list->selection_color->text_color;
                linedes.line_color = list->selection_color->line_color;
                linedes.line_end_color = list->selection_color->line_end_color;
            }
            else
#endif
            if (global_settings.cursor_style == 1
#ifdef HAVE_REMOTE_LCD
                    /* the global_settings.cursor_style check is here to make
                    * sure if they want the cursor instead of bar it will work
                    */
                    || (display->depth < 16 && global_settings.cursor_style)
#endif
            )
            {
                /* Display inverted-line-style */
                style = STYLE_INVERT;
            }
#ifdef HAVE_LCD_COLOR
            else if (global_settings.cursor_style == 2)
            {
                /* Display colour line selector */
                style = STYLE_COLORBAR;
                linedes.text_color = global_settings.lst_color;
                linedes.line_color = global_settings.lss_color;
            }
            else if (global_settings.cursor_style == 3)
            {
                /* Display gradient line selector */
                style = STYLE_GRADIENT;
                linedes.text_color = global_settings.lst_color;
                linedes.line_color = global_settings.lss_color;
                linedes.line_end_color = global_settings.lse_color;
            }
#endif
            is_selected = true;
        }
        
#ifdef HAVE_LCD_COLOR
        /* if the list has a color callback */
        if (list->callback_get_item_color)
        {
            int c = list->callback_get_item_color(i, list->data);
            if (c >= 0)
            {   /* if color selected */
                linedes.text_color = c;
                style |= STYLE_COLORED;
            }
        }
#endif

        linedes.style = style;
        linedes.scroll = is_selected ? true : list->scroll_all;
        linedes.line = i % list->selected_size;
        icon = list->callback_get_item_icon ?
                    list->callback_get_item_icon(i, list->data) : Icon_NOICON;
        /* the list can have both, one of or neither of cursor and item icons,
         * if both don't apply icon padding twice between the icons */
        if (show_cursor && have_icons)
            put_line(display, 0, line * linedes.height + draw_offset,
                    &linedes, "$*s$"ICON_PADDING_S"I$i$"ICON_PADDING_S"s$*t",
                    line_indent, is_selected ? Icon_Cursor : Icon_NOICON,
                    icon, item_offset, entry_name);
        else if (show_cursor || have_icons)
            put_line(display, 0, line * linedes.height + draw_offset,
                    &linedes, "$*s$"ICON_PADDING_S"I$*t", line_indent,
                    show_cursor ? (is_selected ? Icon_Cursor:Icon_NOICON):icon,
                    item_offset, entry_name);
        else
            put_line(display, 0, line * linedes.height + draw_offset,
                    &linedes, "$*s$*t", line_indent, item_offset, entry_name);
    }
    display->set_viewport(parent);
    display->update_viewport();
    display->set_viewport(last_vp);
}

#if defined(HAVE_TOUCHSCREEN)
/* This needs to be fixed if we ever get more than 1 touchscreen on a target. */

/* difference in pixels between draws, above it means enough to start scrolling */
#define SCROLL_BEGIN_THRESHOLD 3 

static enum {
    SCROLL_NONE,            /* no scrolling */
    SCROLL_BAR,             /* scroll by using the scrollbar */
    SCROLL_SWIPE,           /* scroll by wiping over the screen */
    SCROLL_KINETIC,         /* state after releasing swipe */
} scroll_mode;

static int scrollbar_scroll(struct gui_synclist * gui_list,
                                              int y)
{
    const int screen = screens[SCREEN_MAIN].screen_type;
    const int nb_lines = list_get_nb_lines(gui_list, screen);

    if (nb_lines <  gui_list->nb_items)
    {
        /* scrollbar scrolling is still line based */
        y_offset = 0;
        int scrollbar_size = nb_lines*gui_list->line_height[screen];
        int actual_y = y - list_text[screen].y;

        int new_selection = (actual_y * gui_list->nb_items)
                / scrollbar_size;

        int start_item = new_selection - nb_lines/2;
        if(start_item < 0)
            start_item = 0;
        else if(start_item > gui_list->nb_items - nb_lines)
            start_item = gui_list->nb_items - nb_lines;

        gui_list->start_item[screen] = start_item;

        return ACTION_REDRAW;
    }

    return ACTION_NONE;
}

/* kinetic scrolling, based on
 *
 * v = a*t + v0 and ds = v*dt
 *
 * In each (fixed interval) timeout, the list is advanced by ds, then
 * the v is reduced by a.
 * This way we get a linear and smooth deceleration of the scrolling
 *
 * As v is the difference of distance per time unit, v is passed (as
 * pixels moved since the last call) to the scrolling function which takes
 * care of the pixel accurate drawing
 *
 * v0 is dertermined by averaging the last 4 movements of the list
 * (the pixel and time difference is used to compute each v)
 *
 * influenced by http://stechz.com/tag/kinetic/
 * We take the easy and smooth first approach (until section "Drawbacks"),
 * since its drawbacks don't apply for us since our timers seem to be
 * relatively accurate
 */


#define SIGN(a) ((a) < 0 ? -1 : 1)
/* these could possibly be configurable */
/* the lower the smoother */
#define RELOAD_INTERVAL (HZ/25)
/* the higher the earler the list stops */
#define DECELERATION (1000*RELOAD_INTERVAL/HZ)

/* this array holds data to compute the initial velocity v0 */
static struct kinetic_info {
    int difference;
    long ticks;
} kinetic_data[4];
static size_t cur_idx;

static struct cb_data {
    struct gui_synclist *list;  /* current list */
    int velocity;               /* in pixel/s */
} cb_data;

/* data member points to the above struct */
static struct timeout kinetic_tmo;

static bool is_kinetic_over(void)
{
    return !cb_data.velocity && (scroll_mode == SCROLL_KINETIC);
}

/*
 * collect data about how fast the list is moved in order to compute
 * the initial velocity from it later */
static void kinetic_stats_collect(const int difference)
{
    static long last_tick;
    /* collect velocity statistics */
    kinetic_data[cur_idx].difference = difference;
    kinetic_data[cur_idx].ticks = current_tick - last_tick;

    last_tick = current_tick;
    cur_idx += 1;
    if (cur_idx >= ARRAYLEN(kinetic_data))
        cur_idx = 0; /* rewind the index */
}

/*
 * resets the statistic */
static void kinetic_stats_reset(void)
{
    memset(kinetic_data, 0, sizeof(kinetic_data));
    cur_idx = 0;
}

/* cancels all currently active kinetic scrolling */
static void kinetic_force_stop(void)
{
    timeout_cancel(&kinetic_tmo);
    kinetic_stats_reset();
}

/* helper for gui/list.c to cancel scrolling if a normal button event comes
 * through dpad or keyboard or whatever */
void _gui_synclist_stop_kinetic_scrolling(void)
{
    y_offset = 0;
    if (scroll_mode == SCROLL_KINETIC)
        kinetic_force_stop();
    scroll_mode = SCROLL_NONE;
    hide_selection = false;
}
/*
 * returns false if scrolling should be stopped entirely
 *
 * otherwise it returns true even if it didn't actually scroll,
 * but scrolling mode shouldn't be changed
 **/

 
static int scroll_begin_threshold;
static int threshold_accumulation;
static bool swipe_scroll(struct gui_synclist * gui_list, int difference)
{
    /* fixme */
    const enum screen_type screen = screens[SCREEN_MAIN].screen_type;
    const int nb_lines = list_get_nb_lines(gui_list, screen);
    const int line_height = gui_list->line_height[screen];

    if (UNLIKELY(scroll_begin_threshold == 0))
        scroll_begin_threshold = touchscreen_get_scroll_threshold();

    /* make selecting items easier */
    threshold_accumulation += abs(difference);
    if (threshold_accumulation < scroll_begin_threshold && scroll_mode == SCROLL_NONE)
        return false;

    threshold_accumulation = 0;

    /* does the list even scroll? if no, return but still show
     * the caller that we would scroll */
    if (nb_lines >= gui_list->nb_items)
        return true;

    const int old_start = gui_list->start_item[screen];
    int new_start_item = -1;
    int line_diff = 0;

    /* don't scroll at the edges of the list */
    if ((old_start == 0 && difference > 0)
     || (old_start == (gui_list->nb_items - nb_lines) && difference < 0))
    {
        y_offset = 0;
        gui_list->start_item[screen] = old_start;
        return scroll_mode != SCROLL_KINETIC; /* stop kinetic at the edges */
    }

    /* add up y_offset over time and translate to lines
     * if scrolled enough */
    y_offset += difference;
    if (abs(y_offset) > line_height)
    {
        line_diff = y_offset/line_height;
        y_offset -= line_diff * line_height;
    }

    if(line_diff != 0)
    {
        int selection_offset = gui_list->selected_item - old_start;
        new_start_item = old_start - line_diff;
        /* check if new_start_item is bigger than list item count */
        if(new_start_item > gui_list->nb_items - nb_lines)
            new_start_item = gui_list->nb_items - nb_lines;
        /* set new_start_item to 0 if it's negative */
        if(new_start_item < 0)
            new_start_item = 0;

        gui_list->start_item[screen] = new_start_item;
        /* keep selected item in sync */
        gui_list->selected_item = new_start_item + selection_offset;
    }

    return true;
}

static int kinetic_callback(struct timeout *tmo)
{
    /* cancel if screen was pressed */
    if (scroll_mode != SCROLL_KINETIC)
        return 0;

    struct cb_data *data = (struct cb_data*)tmo->data;
    /* ds = v*dt */
    int pixel_diff = data->velocity * RELOAD_INTERVAL / HZ;
    /* remember signedness to detect stopping */
    int old_sign = SIGN(data->velocity);
    /* advance the list */
    if (!swipe_scroll(data->list, pixel_diff))
    {
        /* nothing to scroll? */
        data->velocity = 0;
    }
    else
    {
        /* decelerate by a fixed amount
         * decrementing v0 over time by the deceleration is
         * equivalent to computing v = a*t + v0 */
        data->velocity -= SIGN(data->velocity)*DECELERATION;
        if (SIGN(data->velocity) != old_sign)
            data->velocity = 0;
    }

    /* let get_action() timeout, which loads to a
     * gui_synclist_draw() call from the main thread */
    queue_post(&button_queue, BUTTON_REDRAW, 0);
    /* stop if the velocity hit or crossed zero */
    if (!data->velocity)
    {
        kinetic_stats_reset();
        return 0;
    }
    return RELOAD_INTERVAL; /* cancel or reload */
}

/*
 * computes the initial velocity v0 and sets up the timer */
static bool kinetic_setup_scroll(struct gui_synclist *list)
{
    /* compute initial velocity */
    int i, _i, v0, len = ARRAYLEN(kinetic_data);
    for(i = 0, _i = 0, v0 = 0; i < len; i++)
    {   /* in pixel/s */
        if (kinetic_data[i].ticks > 0)
        {
            v0 += kinetic_data[i].difference*HZ/kinetic_data[i].ticks;
            _i++;
        }
    }
    if (_i > 0)
        v0 /= _i;
    else
        v0 = 0;

    if (v0 != 0)
    {
        cb_data.list = list;
        cb_data.velocity = v0;
        timeout_register(&kinetic_tmo, kinetic_callback, RELOAD_INTERVAL, (intptr_t)&cb_data);
        return true;
    }
    return false;
}

#define OUTSIDE    0
#define TITLE_TEXT (1<<0)
#define TITLE_ICON (1<<1)
#define SCROLLBAR  (1<<2)
#define LIST_TEXT  (1<<3)
#define LIST_ICON  (1<<4)

#define TITLE      (TITLE_TEXT|TITLE_ICON)
#define LIST       (LIST_TEXT|LIST_ICON)

static int get_click_location(struct gui_synclist *list, int x, int y)
{
    int screen = SCREEN_MAIN;
    struct viewport *parent, *title, *text;
    int retval = OUTSIDE;

    parent = list->parent[screen];
    if (viewport_point_within_vp(parent, x, y))
    {
        /* see if the title was clicked */
        title = &title_text[screen];
        if (viewport_point_within_vp(title, x, y))
            retval = TITLE_TEXT;
        /* check the icon too */
        if (list->title_icon != Icon_NOICON && global_settings.show_icons)
        {
            int width = list_icon_width(screen);
            struct viewport vp = *title;
            if (VP_IS_RTL(&vp))
                vp.x += vp.width;
            else
                vp.x -= width;
            vp.width = width;
            if (viewport_point_within_vp(&vp, x, y))
                retval = TITLE_ICON;
        }
        /* check scrollbar. assume it's shown, if it isn't it will be handled
         * later */
        if (retval == OUTSIDE)
        {
            bool on_scrollbar_clicked;
            int adj_x = x - parent->x;
            switch (global_settings.scrollbar)
            {
                case SCROLLBAR_LEFT:
                    on_scrollbar_clicked = adj_x <= SCROLLBAR_WIDTH; break;
                case SCROLLBAR_RIGHT:
                    on_scrollbar_clicked = adj_x > (title->x + title->width - SCROLLBAR_WIDTH); break;
                default:
                    on_scrollbar_clicked = false; break;
            }
            if (on_scrollbar_clicked)
                retval = SCROLLBAR;
        }
        if (retval == OUTSIDE)
        {
            text = &list_text[screen];
            if (viewport_point_within_vp(text, x, y))
                retval = LIST_TEXT;
            else /* if all fails, it must be on the list icons */
                retval = LIST_ICON;
        }
    }
    return retval;
}

unsigned gui_synclist_do_touchscreen(struct gui_synclist * list)
{
    enum screen_type screen;
    struct viewport *parent;
    short x, y;
    int action, adj_x, adj_y, line, line_height, list_start_item;
    bool recurse;
    static bool initial_touch = true;
    static int last_y;
    
    screen = SCREEN_MAIN;
    parent = list->parent[screen];
    line_height = list->line_height[screen];
    list_start_item = list->start_item[screen];
    /* start with getting the action code and finding the click location */
    action = action_get_touchscreen_press(&x, &y);
    adj_x = x - parent->x;
    adj_y = y - parent->y;


    /* some defaults before running the state machine */
    recurse = false;
    hide_selection = false;

    switch (scroll_mode)
    {
        case SCROLL_NONE:
        {
            int click_loc;
            if (initial_touch)
            {
                /* on the first touch last_y has to be reset to avoid
                 * glitches with touches from long ago */
                last_y = adj_y;
                initial_touch = false;
            }

            line = 0; /* silence gcc 'used uninitialized' warning */
            click_loc = get_click_location(list, x, y);
            if (click_loc & LIST)
            {
                if(!skinlist_get_item(&screens[screen], list, adj_x, adj_y, &line))
                {
                    /* selection needs to be corrected if items are only partially visible */
                    line = (adj_y - y_offset) / line_height;
                    if (list_display_title(list, screen))
                        line -= 1; /* adjust for the list title */
                }
                if (line >= list->nb_items)
                    return ACTION_NONE;
                list->selected_item = list_start_item+line;

                gui_synclist_speak_item(list);
            }
            if (action == BUTTON_TOUCHSCREEN)
            {
                /* if not scrolling, the user is trying to select */
                int diff = adj_y - last_y;
                if ((click_loc & LIST) && swipe_scroll(list, diff))
                    scroll_mode = SCROLL_SWIPE;
                else if (click_loc & SCROLLBAR)
                    scroll_mode = SCROLL_BAR;
            }
            else if (action == BUTTON_REPEAT)
            {
                if (click_loc & LIST)
                {
                    /* held a single line for a while, bring up the context menu */
                    gui_synclist_select_item(list, list_start_item + line);
                    /* don't sent context repeatedly */
                    action_wait_for_release();
                    initial_touch = true;
                    return ACTION_STD_CONTEXT;
                }
            }
            else if (action & BUTTON_REL)
            {
                initial_touch = true;
                if (click_loc & LIST)
                {   /* release on list item enters it */
                    gui_synclist_select_item(list, list_start_item + line);
                    return ACTION_STD_OK;
                }
                else if (click_loc & TITLE_TEXT)
                {   /* clicking the title goes one level up (cancel) */
                    return ACTION_STD_CANCEL;
                }
                else if (click_loc & TITLE_ICON)
                {   /* clicking the title icon goes back to the root */
                    return ACTION_STD_MENU;
                }
            }
            break;
        }
        case SCROLL_SWIPE:
        {
            /* when swipe scrolling, we accept outside presses as well and
             * grab the entire screen (i.e. click_loc does not matter) */
            int diff = adj_y - last_y;
            hide_selection = true;
            kinetic_stats_collect(diff);
            if (swipe_scroll(list, diff))
            {
                /* letting the pen go enters kinetic scrolling */
                if ((action & BUTTON_REL))
                {
                    if (kinetic_setup_scroll(list))
                    {
                        hide_selection = true;
                        scroll_mode = SCROLL_KINETIC;
                    }
                    else
                        scroll_mode = SCROLL_NONE;
                }
            }
            else if (action & BUTTON_REL)
                scroll_mode = SCROLL_NONE;

            if (scroll_mode == SCROLL_NONE)
                initial_touch = true;
            break;
        }
        case SCROLL_KINETIC:
        {
            /* during kinetic scrolling we need to handle cancellation.
             * This state is actually only entered upon end of it as this
             * function is not called during the animation. */
            if (!is_kinetic_over())
            {   /* a) the user touched the screen (manual cancellation) */
                kinetic_force_stop();
                if (get_click_location(list, x, y) & SCROLLBAR)
                    scroll_mode = SCROLL_BAR;
                else
                    scroll_mode = SCROLL_SWIPE;
            }
            else
            {   /* b) kinetic scrolling stopped on its own */
                /* need to re-run this with SCROLL_NONE since otherwise
                 * the next touch is not detected correctly */
                scroll_mode = SCROLL_NONE;
                recurse = true;
            }
            break;
        }
        case SCROLL_BAR:
        {
            hide_selection = true;
            /* similarly to swipe scroll, using the scrollbar grabs
             * focus so the click location is irrelevant */
            scrollbar_scroll(list, adj_y);
            if (action & BUTTON_REL)
                scroll_mode = SCROLL_NONE;
            break;
        }
    }

    /* register y position unless forcefully reset */
    if (!initial_touch)
        last_y = adj_y;

    return recurse ? gui_synclist_do_touchscreen(list) : ACTION_REDRAW;
}

#endif
