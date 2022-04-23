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

#ifdef HAVE_TOUCHSCREEN
/* used in gui_synclist->scroll_mode */
enum {
    SCROLL_NONE = 0,        /* no scrolling */
    SCROLL_BAR,             /* scroll by using the scrollbar */
    SCROLL_SWIPE,           /* scroll by wiping over the screen */
    SCROLL_KINETIC,         /* state after releasing swipe */
};
#endif

/* these are static to make scrolling work */
static struct viewport list_text[NB_SCREENS], title_text[NB_SCREENS];

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

static void _default_listdraw_fn(struct list_putlineinfo_t *list_info)
{
    struct screen *display = list_info->display; 
    int x = list_info->x;
    int y = list_info->y;
    int item_indent = list_info->item_indent;
    int item_offset = list_info->item_offset;
    int icon = list_info->icon;
    bool is_selected = list_info->is_selected;
    bool is_title = list_info->is_title;
    bool show_cursor = list_info->show_cursor;
    bool have_icons = list_info->have_icons;
    struct line_desc *linedes = list_info->linedes;
    const char *dsp_text = list_info->dsp_text;

    if (is_title)
    {
        if (have_icons)
            display->put_line(x, y, linedes, "$"ICON_PADDING_S"I$t",
                    icon, dsp_text);
        else
            display->put_line(x, y, linedes, "$t", dsp_text);
    }
    else if (show_cursor && have_icons)
    {
    /* the list can have both, one of or neither of cursor and item icons,
     * if both don't apply icon padding twice between the icons */
        display->put_line(x, y, 
                linedes, "$*s$"ICON_PADDING_S"I$i$"ICON_PADDING_S"s$*t",
                item_indent, is_selected ? Icon_Cursor : Icon_NOICON,
                icon, item_offset, dsp_text);
    }
    else if (show_cursor || have_icons)
    {
        display->put_line(x, y, linedes, "$*s$"ICON_PADDING_S"I$*t", item_indent,
                show_cursor ? (is_selected ? Icon_Cursor:Icon_NOICON):icon,
                item_offset, dsp_text);
    }
    else
    {
        display->put_line(x, y, linedes, "$*s$*t", item_indent, item_offset, dsp_text);
    }
}

static bool draw_title(struct screen *display,
                       struct gui_synclist *list,
                       list_draw_item *callback_draw_item)
{
    const int screen = display->screen_type;
    struct viewport *title_text_vp = &title_text[screen];
    struct line_desc linedes = LINE_DESC_DEFINIT;

    if (sb_set_title_text(list->title, list->title_icon, screen))
        return false; /* the sbs is handling the title */
    display->scroll_stop_viewport(title_text_vp);
    if (!list_display_title(list, screen))
        return false;
    *title_text_vp = *(list->parent[screen]);
    linedes.height = list->line_height[screen];
    title_text_vp->height = linedes.height;

#if LCD_DEPTH > 1
    /* XXX: Do we want to support the separator on remote displays? */
    if (display->screen_type == SCREEN_MAIN && global_settings.list_separator_height != 0)
        linedes.separator_height = abs(global_settings.list_separator_height)
                                + (lcd_get_dpi() > 200 ? 2 : 1);
#endif

#ifdef HAVE_LCD_COLOR
    if (list->title_color >= 0)
        linedes.style |= (STYLE_COLORED|list->title_color);
#endif
    linedes.scroll = true;

    display->set_viewport(title_text_vp);
    int icon = list->title_icon;
    int icon_w = list_icon_width(display->screen_type);
    bool have_icons = false;
    if (icon != Icon_NOICON && list->show_icons)
    {
        have_icons = true;
    }

    struct list_putlineinfo_t list_info =
    {
        .x = 0, .y = 0, .item_indent = 0, .item_offset = 0,
         .line = -1, .icon = icon, .icon_width = icon_w,
        .display = display, .vp = title_text_vp, .linedes = &linedes, .list = list,
        .dsp_text = list->title,
        .is_selected = false, .is_title = true, .show_cursor = false,
        .have_icons = have_icons
    };
    callback_draw_item(&list_info);

    return true;
}

void list_draw(struct screen *display, struct gui_synclist *list)
{
    int start, end, item_offset, i;
    const int screen = display->screen_type;
    list_draw_item *callback_draw_item;

    const int list_start_item = list->start_item[screen];
    const bool scrollbar_in_left = (list->scrollbar == SCROLLBAR_LEFT);
    const bool scrollbar_in_right = (list->scrollbar == SCROLLBAR_RIGHT);
    const bool show_cursor = (list->cursor_style == SYNCLIST_CURSOR_NOSTYLE);
    const bool have_icons = list->callback_get_item_icon && list->show_icons;

    struct viewport *parent = (list->parent[screen]);
    struct line_desc linedes = LINE_DESC_DEFINIT;
    bool show_title;
    struct viewport *list_text_vp = &list_text[screen];
    int indent = 0;

    if (list->callback_draw_item != NULL)
        callback_draw_item = list->callback_draw_item;
    else
        callback_draw_item = _default_listdraw_fn;

    struct viewport * last_vp = display->set_viewport(parent);
    display->clear_viewport();
    if (!list->scroll_all)
        display->scroll_stop_viewport(list_text_vp);
    *list_text_vp = *parent;
    if ((show_title = draw_title(display, list, callback_draw_item)))
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
    /* y_pos needs to be clamped now since it can overflow the maximum
     * in some cases, and we have no easy way to prevent this beforehand */
    int max_y_pos = list->nb_items * linedes.height - list_text[screen].height;
    if (max_y_pos > 0 && list->y_pos > max_y_pos)
        list->y_pos = max_y_pos;

    int draw_offset = list_start_item * linedes.height - list->y_pos;
    /* draw some extra items to not have empty lines at the top and bottom */
    if (draw_offset > 0)
    {
        /* make it negative for more consistent apparence when switching
         * directions */
        draw_offset -= linedes.height;
        if (start > 0)
            start--;
    }
    else if (draw_offset < 0) {
        if(end < list->nb_items)
            end++;
    }

    /* If the viewport is not an exact multiple of the line height, then
     * there will be space for one more partial line. */
    int spare_space = list_text_vp->height - linedes.height * nb_lines;
    if(nb_lines < list->nb_items && spare_space > 0 && end < list->nb_items)
        if(end < list->nb_items)
            end++;
#else
    #define draw_offset 0
#endif

    /* draw the scrollbar if its needed */
    if (list->scrollbar != SCROLLBAR_OFF)
    {
        /* if the scrollbar is shown the text viewport needs to shrink */
        if (nb_lines < list->nb_items)
        {
            struct viewport vp = *list_text_vp;
            vp.width = SCROLLBAR_WIDTH;
#ifndef HAVE_TOUCHSCREEN
            /* touchscreens must use full viewport height
             * due to pixelwise rendering */
            vp.height = linedes.height * nb_lines;
#endif
            list_text_vp->width -= SCROLLBAR_WIDTH;
            if (scrollbar_in_right)
                vp.x += list_text_vp->width;
            else /* left */
                list_text_vp->x += SCROLLBAR_WIDTH;
            struct viewport *last = display->set_viewport(&vp);

#ifndef HAVE_TOUCHSCREEN
            /* button targets go itemwise */
            int scrollbar_items = list->nb_items;
            int scrollbar_min = list_start_item;
            int scrollbar_max = list_start_item + nb_lines;
#else
            /* touchscreens use pixelwise scrolling */
            int scrollbar_items = list->nb_items * linedes.height;
            int scrollbar_min = list->y_pos;
            int scrollbar_max = list->y_pos + list_text_vp->height;
#endif
            gui_scrollbar_draw(display,
                    (scrollbar_in_left? 0: 1), 0, SCROLLBAR_WIDTH-1, vp.height,
                    scrollbar_items, scrollbar_min, scrollbar_max, VERTICAL);
            display->set_viewport(last);
        }
        /* shift everything a bit in relation to the title */
        else if (!VP_IS_RTL(list_text_vp) && scrollbar_in_left)
            indent += SCROLLBAR_WIDTH;
        else if (VP_IS_RTL(list_text_vp) && scrollbar_in_right)
            indent += SCROLLBAR_WIDTH;
    }

    display->set_viewport(list_text_vp);
    int icon_w = list_icon_width(screen);
    int character_width = display->getcharwidth();

    struct list_putlineinfo_t list_info =
    {
        .x = 0, .y = 0, .vp = list_text_vp, .list = list,
        .icon_width = icon_w, .is_title = false, .show_cursor = show_cursor,
        .have_icons = have_icons, .linedes = &linedes, .display = display
    };

    for (i=start; i<end && i<list->nb_items; i++)
    {
        /* do the text */
        enum themable_icons icon;
        unsigned const char *s;
        extern char simplelist_buffer[SIMPLELIST_MAX_LINES * SIMPLELIST_MAX_LINELENGTH];
        /*char entry_buffer[MAX_PATH]; use the buffer from gui/list.c instead */
        unsigned char *entry_name;
        int line = i - start;
        int line_indent = 0;
        int style = STYLE_DEFAULT;
        bool is_selected = false;
        s = list->callback_get_item_name(i, list->data, simplelist_buffer,
                                         sizeof(simplelist_buffer));
        if (P2ID((unsigned char *)s) > VOICEONLY_DELIMITER)
            entry_name = "";
        else
            entry_name = P2STR(s);

        while (*entry_name == '\t')
        {
            line_indent++;
            entry_name++;
        }
        if (line_indent)
        {
            if (list->show_icons)
                line_indent *= icon_w;
            else
                line_indent *= character_width;
        }
        line_indent += indent;

        /* position the string at the correct offset place */
        int item_width,h;
        display->getstringsize(entry_name, &item_width, &h);
        item_offset = gui_list_get_item_offset(list, item_width, indent + (list->show_icons ? icon_w : 0),
                display, list_text_vp);

        /* draw the selected line */
        if(
#ifdef HAVE_TOUCHSCREEN
            /* don't draw it during scrolling */
            list->scroll_mode == SCROLL_NONE &&
#endif
                i >= list->selected_item
                && i <  list->selected_item + list->selected_size)
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
            if (list->cursor_style == SYNCLIST_CURSOR_INVERT
#ifdef HAVE_REMOTE_LCD
                    /* the global_settings.cursor_style check is here to make
                    * sure if they want the cursor instead of bar it will work
                    */
                    || (display->depth < 16 && list->cursor_style)
#endif
            )
            {
                /* Display inverted-line-style */
                style = STYLE_INVERT;
            }
#ifdef HAVE_LCD_COLOR
            else if (list->cursor_style == SYNCLIST_CURSOR_COLOR)
            {
                /* Display colour line selector */
                style = STYLE_COLORBAR;
                linedes.text_color = global_settings.lst_color;
                linedes.line_color = global_settings.lss_color;
            }
            else if (list->cursor_style == SYNCLIST_CURSOR_GRADIENT)
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


        list_info.y = line * linedes.height + draw_offset;
        list_info.is_selected = is_selected;
        list_info.item_indent = line_indent;
        list_info.line = i;
        list_info.icon = icon;
        list_info.dsp_text = entry_name;
        list_info.item_offset = item_offset;

        callback_draw_item(&list_info);
    }
    display->set_viewport(parent);
    display->update_viewport();
    display->set_viewport(last_vp);
}

#if defined(HAVE_TOUCHSCREEN)
/* This needs to be fixed if we ever get more than 1 touchscreen on a target. */

static void do_touch_scroll(struct gui_synclist *gui_list, int new_y_pos)
{
    if (new_y_pos < 0)
        new_y_pos = 0;

    int line_height = gui_list->line_height[SCREEN_MAIN];
    int new_start = new_y_pos / line_height;
    int nb_lines = list_get_nb_lines(gui_list, SCREEN_MAIN);
    if (new_start > gui_list->nb_items - nb_lines)
    {
        new_start = gui_list->nb_items - nb_lines;
        new_y_pos = new_start * line_height;
    }

    int new_item = new_start + nb_lines/2;
    if (gui_list->selected_size > 1)
        new_item -= new_item % gui_list->selected_size;

    gui_list->selected_item = new_item;
    gui_list->start_item[SCREEN_MAIN] = new_start;
    gui_list->y_pos = new_y_pos;
}

/* Handle touch scrolling using the scrollbar. Pass the screen y-coordinate
 * of the current touch position. */
static int scrollbar_scroll(struct gui_synclist *gui_list, int y)
{
    const enum screen_type screen = SCREEN_MAIN;
    const int nb_lines = list_get_nb_lines(gui_list, screen);

    if (nb_lines < gui_list->nb_items)
    {
        const int line_height = gui_list->line_height[screen];
        int bar_height = list_text[screen].height;
        int bar_y = y - list_text[screen].y;

        if (bar_y < 0)
            bar_y = 0;
        else if(bar_y >= bar_height)
            bar_y = bar_height - 1;

        int new_y_pos = (bar_y * gui_list->nb_items * line_height) / bar_height;
        new_y_pos -= (nb_lines * line_height) / 2;

        do_touch_scroll(gui_list, new_y_pos);

        return ACTION_REDRAW;
    }

    return ACTION_NONE;
}

/* Handle swipe scrolling on a list. 'delta' is the distance to scroll
 * relative to gui_list->scroll_base_y, which should be set up at the
 * beginning of scrolling. */
static int swipe_scroll(struct gui_synclist *gui_list, int delta)
{
    /* nothing to do if the list does not scroll */
    const int nb_lines = list_get_nb_lines(gui_list, SCREEN_MAIN);
    if (nb_lines >= gui_list->nb_items)
        return ACTION_NONE;

    do_touch_scroll(gui_list, gui_list->scroll_base_y - delta);

    return ACTION_REDRAW;
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

struct kinetic_cb_data {
    struct gui_synclist *list;
    int velocity;
};

struct kinetic {
    /* callback for performing the scroll animation */
    struct kinetic_cb_data cb_data;
    struct timeout tmo;
};

static struct kinetic kinetic;
static struct gesture_vel list_gvel;

static void kinetic_stop_scrolling(struct kinetic *k, struct gui_synclist *list)
{
    if (k->cb_data.list == list)
        timeout_cancel(&k->tmo);
}

/* helper for gui/list.c to cancel scrolling if a normal button event comes */
void _gui_synclist_stop_kinetic_scrolling(struct gui_synclist *list)
{
    if (list->scroll_mode == SCROLL_KINETIC)
    {
        kinetic_stop_scrolling(&kinetic, list);
        list->scroll_mode = SCROLL_NONE;
    }
}

static int kinetic_callback(struct timeout *tmo)
{
    struct kinetic_cb_data *data = (struct kinetic_cb_data*)tmo->data;
    struct gui_synclist *list = data->list;

    /* deal with cancellation */
    if (list->scroll_mode != SCROLL_KINETIC)
        return 0;

    /* ds = v*dt */
    int pixel_diff = data->velocity * RELOAD_INTERVAL / HZ;
    int action = swipe_scroll(list, pixel_diff);
    if (action == ACTION_REDRAW)
    {
        /* force the list to redraw */
        button_queue_post(BUTTON_REDRAW, 0);
    }

    /* apply deceleration */
    int old_sign = SIGN(data->velocity);
    data->velocity -= SIGN(data->velocity) * DECELERATION;
    if (SIGN(data->velocity) != old_sign)
        data->velocity = 0;

    /* stop scrolling if we didn't move, it means we hit the end */
    if (list->y_pos == list->scroll_base_y)
        data->velocity = 0;
    else
        /* update base y since our scroll distance doesn't accumulate. */
        list->scroll_base_y = list->y_pos;

    if (data->velocity == 0)
    {
        list->scroll_mode = SCROLL_NONE;
        return 0;
    }

    return RELOAD_INTERVAL;
}

/* Computes the initial velocity v0 and sets up the timer */
static bool kinetic_start_scrolling(struct kinetic *k, struct gui_synclist *list)
{
    int xvel, yvel;
    gesture_vel_get(&list_gvel, &xvel, &yvel);
    if (yvel == 0)
        return false;

    k->cb_data.list = list;
    k->cb_data.velocity = yvel;

    list->scroll_mode = SCROLL_KINETIC;
    list->scroll_base_y = list->y_pos;
    timeout_register(&k->tmo, kinetic_callback, RELOAD_INTERVAL,
                     (intptr_t)&k->cb_data);
    return true;
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
    const enum screen_type screen = SCREEN_MAIN;
    int retval = OUTSIDE;

    struct viewport *parent = list->parent[screen];
    struct viewport *text = &list_text[screen];
    struct viewport *title = &title_text[screen];

    if (viewport_point_within_vp(parent, x, y))
    {
        /* see if the title was clicked */
        if (viewport_point_within_vp(title, x, y))
            retval = TITLE_TEXT;

        /* check the title icon */
        if (list->title_icon != Icon_NOICON && list->show_icons)
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

        /* check scrollbar if shown */
        if (retval == OUTSIDE)
        {
            bool on_scrollbar_clicked;
            int adj_x = x - text->x;
            switch (list->scrollbar)
            {
                case SCROLLBAR_OFF:
                    /*fall-through*/
                default:
                    on_scrollbar_clicked = false;
                    break;
                case SCROLLBAR_LEFT:
                    on_scrollbar_clicked = adj_x <= SCROLLBAR_WIDTH;
                    break;
                case SCROLLBAR_RIGHT:
                    on_scrollbar_clicked = adj_x > (text->x + text->width - SCROLLBAR_WIDTH);
                    break;
            }
            if (on_scrollbar_clicked)
                retval = SCROLLBAR;
        }

        /* check the text area */
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

unsigned gui_synclist_do_touchscreen(struct gui_synclist *list)
{
    struct touchevent tevent;
    struct gesture_event gevent;

    action_get_touch_event(&tevent);
    if (!action_gesture_get_event(&gevent))
        return ACTION_NONE;

    const enum screen_type screen = SCREEN_MAIN;
    struct viewport *list_vp = list->parent[screen];
    int adj_x = gevent.x - list_vp->x;
    int adj_y = gevent.y - list_vp->y;
    int line_height = list->line_height[screen];
    int start_item = list->start_item[screen];
    int start_y = start_item * line_height;
    int action = ACTION_NONE;
    int click_loc;

    switch (gevent.id)
    {
    case GESTURE_NONE:
        if (!action_gesture_is_pressed())
            break;
        /* fallthrough */

    case GESTURE_TAP:
    case GESTURE_LONG_PRESS:
        _gui_synclist_stop_kinetic_scrolling(list);
        click_loc = get_click_location(list, gevent.x, gevent.y);
        if (click_loc & LIST)
        {
            int line;
            if(!skinlist_get_item(&screens[screen], list, adj_x, adj_y, &line))
            {
                line = (adj_y - (start_y - list->y_pos)) / line_height;
                if (list_display_title(list, screen))
                    line -= 1;
            }

            int new_item = start_item + line;
            if (new_item < list->nb_items)
            {
                if (list->selected_size > 1)
                    new_item -= new_item % list->selected_size;

                list->selected_item = new_item;
                gui_synclist_speak_item(list);

                if (gevent.id == GESTURE_TAP)
                    action = ACTION_STD_OK;
                else if (gevent.id == GESTURE_LONG_PRESS)
                    action = ACTION_STD_CONTEXT;
                else
                    action = ACTION_REDRAW;
            }
        }
        else if ((click_loc & TITLE) && gevent.id == GESTURE_TAP)
        {
            if (click_loc & TITLE_TEXT)
                action = ACTION_STD_CANCEL;
            else if (click_loc & TITLE_ICON)
                action = ACTION_STD_MENU;
        }
        else if (gevent.id != GESTURE_NONE && (click_loc & SCROLLBAR))
        {
            action = scrollbar_scroll(list, gevent.y);

            /* allow long press to become a motion event */
            if (gevent.id != GESTURE_TAP)
                break;
        }

        if (action != ACTION_REDRAW)
            gui_synclist_select_item(list, list->selected_item);
        if (gevent.id != GESTURE_NONE)
            action_gesture_reset();
        break;

    case GESTURE_DRAGSTART:
        _gui_synclist_stop_kinetic_scrolling(list);
        gesture_vel_reset(&list_gvel);
        list->scroll_base_y = list->y_pos;
        /* fallthrough */

    case GESTURE_DRAG:
        gesture_vel_process(&list_gvel, &tevent);

        if (list->scroll_mode == SCROLL_NONE)
        {
            click_loc = get_click_location(list, gevent.ox, gevent.oy);
            if (click_loc & SCROLLBAR)
                list->scroll_mode = SCROLL_BAR;
            else if (click_loc & LIST)
                list->scroll_mode = SCROLL_SWIPE;
        }

        if (list->scroll_mode == SCROLL_BAR)
            action = scrollbar_scroll(list, gevent.y);
        else if (list->scroll_mode == SCROLL_SWIPE)
            action = swipe_scroll(list, gevent.y - gevent.oy);

        break;

    case GESTURE_RELEASE:
        if (list->scroll_mode != SCROLL_SWIPE ||
            !kinetic_start_scrolling(&kinetic, list))
        {
            list->scroll_mode = SCROLL_NONE;
        }

        action_gesture_reset();
        action = ACTION_REDRAW;
        break;

    default:
        break;
    }

    return action;
}

#endif
