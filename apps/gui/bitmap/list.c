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
#include "lcd.h"
#include "font.h"
#include "button.h"
#include "string.h"
#include "settings.h"
#include "kernel.h"
#include "system.h"
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

#define ICON_PADDING 1

/* these are static to make scrolling work */
static struct viewport list_text[NB_SCREENS], title_text[NB_SCREENS];

#ifdef HAVE_TOUCHSCREEN
/* difference in pixels between draws, above it means enough to start scrolling */
#define SCROLL_BEGIN_THRESHOLD 3 

static enum {
    SCROLL_NONE,            /* no scrolling */
    SCROLL_BAR,             /* scroll by using the scrollbar */
    SCROLL_SWIPE,           /* scroll by wiping over the screen */
    SCROLL_KINETIC,         /* state after releasing swipe */
} scroll_mode;

static int y_offset;
#endif

int gui_list_get_item_offset(struct gui_synclist * gui_list, int item_width,
                             int text_pos, struct screen * display,
                             struct viewport *vp);
bool list_display_title(struct gui_synclist *list, enum screen_type screen);

void gui_synclist_scroll_stop(struct gui_synclist *lists)
{
    int i;
    FOR_NB_SCREENS(i)
    {
        screens[i].scroll_stop(&list_text[i]);
        screens[i].scroll_stop(&title_text[i]);
        screens[i].scroll_stop(lists->parent[i]);
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
static bool draw_title(struct screen *display, struct gui_synclist *list)
{
    const int screen = display->screen_type;
    int style = STYLE_DEFAULT;
    struct viewport *title_text_vp = &title_text[screen];

    if (sb_set_title_text(list->title, list->title_icon, screen))
        return false; /* the sbs is handling the title */
    display->scroll_stop(title_text_vp);
    if (!list_display_title(list, screen))
        return false;
    *title_text_vp = *(list->parent[screen]);
    title_text_vp->height = font_get(title_text_vp->font)->height;

    if (list->title_icon != Icon_NOICON && global_settings.show_icons)
    {
        struct viewport title_icon = *title_text_vp;

        title_icon.width = get_icon_width(screen) + ICON_PADDING * 2;
        if (VP_IS_RTL(&title_icon))
        {
            title_icon.x += title_text_vp->width - title_icon.width;
        }
        else
        {
            title_text_vp->x += title_icon.width;
        }
        title_text_vp->width -= title_icon.width;

        display->set_viewport(&title_icon);
        screen_put_icon(display, 0, 0, list->title_icon);
    }
#ifdef HAVE_LCD_COLOR
    if (list->title_color >= 0)
    {
        style |= (STYLE_COLORED|list->title_color);
    }
#endif
    display->set_viewport(title_text_vp);
    display->puts_scroll_style(0, 0, list->title, style);
    return true;
}

void list_draw(struct screen *display, struct gui_synclist *list)
{
    struct viewport list_icons;
    int start, end, line_height, style, i;
    const int screen = display->screen_type;
    const int list_start_item = list->start_item[screen];
    const int icon_width = get_icon_width(screen) + ICON_PADDING;
    const bool scrollbar_in_left = (global_settings.scrollbar == SCROLLBAR_LEFT);
    const bool show_cursor = !global_settings.cursor_style &&
                        list->show_selection_marker;
    struct viewport *parent = (list->parent[screen]);
#ifdef HAVE_LCD_COLOR
    unsigned char cur_line = 0;
#endif
    int item_offset;
    bool show_title;
    struct viewport *list_text_vp = &list_text[screen];

    line_height = font_get(parent->font)->height;
    display->set_viewport(parent);
    display->clear_viewport();
    display->scroll_stop(list_text_vp);
    *list_text_vp = *parent;
    if ((show_title = draw_title(display, list)))
    {
        list_text_vp->y += line_height;
        list_text_vp->height -= line_height;
    }

    const int nb_lines = viewport_get_nb_lines(list_text_vp);

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
        draw_offset -= line_height;
        if (start > 0)
            start--;
    }
    else if (y_offset < 0)
        end++;
#else
    #define draw_offset 0
#endif

    /* draw the scrollbar if its needed */
    if (global_settings.scrollbar && nb_lines < list->nb_items)
    {
        struct viewport vp = *list_text_vp;
        vp.width = SCROLLBAR_WIDTH;
        vp.height = line_height * nb_lines;
        vp.x = parent->x;
        list_text_vp->width -= SCROLLBAR_WIDTH;
        if (scrollbar_in_left)
            list_text_vp->x += SCROLLBAR_WIDTH;
        else
            vp.x += list_text_vp->width;
        display->set_viewport(&vp);
        gui_scrollbar_draw(display,
                (scrollbar_in_left? 0: 1), 0, SCROLLBAR_WIDTH-1, vp.height,
                list->nb_items, list_start_item, list_start_item + nb_lines,
                VERTICAL);
    }
    else if (show_title)
    {
        /* shift everything a bit in relation to the title... */
        if (!VP_IS_RTL(list_text_vp) && scrollbar_in_left)
        {
            list_text_vp->width -= SCROLLBAR_WIDTH;
            list_text_vp->x += SCROLLBAR_WIDTH;
        }
        else if (VP_IS_RTL(list_text_vp) && !scrollbar_in_left)
        {
            list_text_vp->width -= SCROLLBAR_WIDTH;
        }
    }

    /* setup icon placement */
    list_icons = *list_text_vp;
    int icon_count = (list->callback_get_item_icon != NULL) ? 1 : 0;
    if (show_cursor)
        icon_count++;
    if (icon_count)
    {
        list_icons.width = icon_width * icon_count;
        list_text_vp->width -= list_icons.width + ICON_PADDING;
        if (VP_IS_RTL(&list_icons))
            list_icons.x += list_text_vp->width + ICON_PADDING;
        else
            list_text_vp->x += list_icons.width + ICON_PADDING;
    }

    for (i=start; i<end && i<list->nb_items; i++)
    {
        /* do the text */
        unsigned const char *s;
        char entry_buffer[MAX_PATH];
        unsigned char *entry_name;
        int text_pos = 0;
        int line = i - start;
        s = list->callback_get_item_name(i, list->data, entry_buffer,
                                         sizeof(entry_buffer));
        entry_name = P2STR(s);
        display->set_viewport(list_text_vp);
        style = STYLE_DEFAULT;
        /* position the string at the correct offset place */
        int item_width,h;
        display->getstringsize(entry_name, &item_width, &h);
        item_offset = gui_list_get_item_offset(list, item_width, text_pos,
                display, list_text_vp);

#ifdef HAVE_LCD_COLOR
        /* if the list has a color callback */
        if (list->callback_get_item_color)
        {
            int color = list->callback_get_item_color(i, list->data);
            /* if color selected */
            if (color >= 0)
            {
                style |= STYLE_COLORED|color;
            }
        }
#endif
        /* draw the selected line */
        if(
#ifdef HAVE_TOUCHSCREEN
            /* don't draw it during scrolling */
            scroll_mode == SCROLL_NONE &&
#endif
                i >= list->selected_item
                && i <  list->selected_item + list->selected_size
                && list->show_selection_marker)
        {/* The selected item must be displayed scrolling */
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
            }
            else if (global_settings.cursor_style == 3)
            {
                /* Display gradient line selector */
                style = STYLE_GRADIENT;

                /* Make the lcd driver know how many lines the gradient should
                   cover and current line number */
                /* number of selected lines */
                style |= NUMLN_PACK(list->selected_size);
                /* current line number, zero based */
                style |= CURLN_PACK(cur_line);
                cur_line++;
            }
#endif
            /* if the text is smaller than the viewport size */
            if (item_offset> item_width - (list_text_vp->width - text_pos))
            {
                /* don't scroll */
                display->puts_style_xyoffset(0, line, entry_name,
                        style, item_offset, draw_offset);
            }
            else
            {
                display->puts_scroll_style_xyoffset(0, line, entry_name,
                        style, item_offset, draw_offset);
            }
        }
        else
        {
            if (list->scroll_all)
                display->puts_scroll_style_xyoffset(0, line, entry_name,
                        style, item_offset, draw_offset);
            else
                display->puts_style_xyoffset(0, line, entry_name,
                        style, item_offset, draw_offset);
        }
        /* do the icon */
        display->set_viewport(&list_icons);
        if (list->callback_get_item_icon != NULL)
        {
            screen_put_icon_with_offset(display, show_cursor?1:0,
                                    (line),show_cursor?ICON_PADDING:0,draw_offset,
                                    list->callback_get_item_icon(i, list->data));
        }
        if (show_cursor && i >= list->selected_item &&
                i <  list->selected_item + list->selected_size)
        {
            screen_put_icon_with_offset(display, 0, line, 0, draw_offset, Icon_Cursor);
        }
    }
    display->set_viewport(parent);
    display->update_viewport();
    display->set_viewport(NULL);
}

#if defined(HAVE_TOUCHSCREEN)
/* This needs to be fixed if we ever get more than 1 touchscreen on a target. */

static bool released = false;

/* Used for kinetic scrolling as we need to know the last position to
 * recognize the scroll direction.
 * This gets reset to 0 at the end of scrolling
 */
static int last_position=0;

static int scrollbar_scroll(struct gui_synclist * gui_list,
                                              int y)
{
    const int screen = screens[SCREEN_MAIN].screen_type;
    const int nb_lines = viewport_get_nb_lines(&list_text[screen]);

    if (nb_lines <  gui_list->nb_items)
    {
        /* scrollbar scrolling is still line based */
        y_offset = 0;
        int scrollbar_size = nb_lines*
            font_get(gui_list->parent[screen]->font)->height;
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
}
/*
 * returns false if scrolling should be stopped entirely
 *
 * otherwise it returns true even if it didn't actually scroll,
 * but scrolling mode shouldn't be changed
 **/

 
static int scroll_begin_threshold;
static int threshold_accumulation;
static bool swipe_scroll(struct gui_synclist * gui_list, int line_height, int difference)
{
    /* fixme */
    const enum screen_type screen = screens[SCREEN_MAIN].screen_type;
    const int nb_lines = viewport_get_nb_lines(&list_text[screen]);

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
    int line_height = font_get(data->list->parent[0]->font)->height;
    /* ds = v*dt */
    int pixel_diff = data->velocity * RELOAD_INTERVAL / HZ;
    /* remember signedness to detect stopping */
    int old_sign = SIGN(data->velocity);
    /* advance the list */
    if (!swipe_scroll(data->list, line_height, pixel_diff))
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

    queue_post(&button_queue, BUTTON_TOUCHSCREEN, 0);
    /* stop if the velocity hit or crossed zero */
    if (!data->velocity)
    {
        kinetic_stats_reset();
        return 0;
    }
    /* let get_action() timeout, which loads to a
     * gui_synclist_draw() call from the main thread */
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

unsigned gui_synclist_do_touchscreen(struct gui_synclist * gui_list)
{
    short x, y;
    const enum screen_type screen = SCREEN_MAIN;
    struct viewport *info_vp = sb_skin_get_info_vp(screen);
    const int button = action_get_touchscreen_press_in_vp(&x, &y, info_vp);
    const int list_start_item = gui_list->start_item[screen];
    const int line_height = font_get(gui_list->parent[screen]->font)->height;
    const struct viewport *list_text_vp = &list_text[screen];
    const bool old_released = released;
    const bool show_title = list_display_title(gui_list, screen);
    const bool show_cursor = !global_settings.cursor_style &&
                        gui_list->show_selection_marker;
    const bool on_title_clicked = show_title && y < line_height && (button&BUTTON_REL);
    const bool cancelled_kinetic = (scroll_mode == SCROLL_KINETIC
                        && button != ACTION_NONE && button != ACTION_UNKNOWN
                        && !is_kinetic_over());
    int icon_width = 0;
    int line, list_width = list_text_vp->width;
    static bool wait_for_release = false;

    released = (button&BUTTON_REL) != 0;

    if (released && wait_for_release)
    {   /* was waiting on a release, reset everything so the next call
         * can start from new */
        wait_for_release = false;
        last_position = 0;
        if (scroll_mode == SCROLL_KINETIC)
            kinetic_force_stop();
        scroll_mode = SCROLL_NONE;
        return ACTION_NONE;
    }

    if (button == ACTION_NONE || button == ACTION_UNKNOWN)
    {
        /* this happens when we hit edges of the list while kinetic scrolling,
         * but not when manually cancelling */
        if (scroll_mode == SCROLL_KINETIC)
            return ACTION_REDRAW;
        return ACTION_NONE;
    }

    /* x and y are relative to info_vp */
    if (gui_list->callback_get_item_icon != NULL)
        icon_width += get_icon_width(screen);
    if (show_cursor)
        icon_width += get_icon_width(screen);

    if (on_title_clicked)
    {
        if (scroll_mode == SCROLL_NONE || is_kinetic_over())
        {
            if (x < icon_width)
            {
                /* Top left corner is GO_TO_ROOT */
                if (button == BUTTON_REL)
                    return ACTION_STD_MENU;
                else if (button == (BUTTON_REPEAT|BUTTON_REL))
                    return ACTION_STD_CONTEXT;
                return ACTION_NONE;
            }
            else /* click on title text is cancel */
                if (button == BUTTON_REL)
                    return ACTION_STD_CANCEL;
        }
        /* do this after the above so the scrolling stops without
         * going back in the list with the same touch */
        if (scroll_mode == SCROLL_KINETIC)
        {
            kinetic_force_stop();
            scroll_mode = SCROLL_NONE;
        }
    }
    else /* list area clicked (or not released) */
    {
        const int actual_y = y - (show_title ? line_height : 0);
        bool on_scrollbar_clicked;
        switch (global_settings.scrollbar)
        {
            case SCROLLBAR_LEFT:
                on_scrollbar_clicked = x <= SCROLLBAR_WIDTH; break;
            case SCROLLBAR_RIGHT:
                on_scrollbar_clicked = x > (icon_width + list_width); break;
            default:
                on_scrollbar_clicked = false; break;
        }
        /* conditions for scrollbar scrolling:
         *    * pen is on the scrollbar
         *      AND scrollbar is on the right (left case is handled above)
         * OR * pen is in the somewhere else but we did scrollbar scrolling before
         *
         * scrollbar scrolling must end if the pen is released
         * scrollbar scrolling must not happen if we're currently scrolling
         * via swiping the screen
         **/

        if (!released && scroll_mode != SCROLL_SWIPE &&
            (on_scrollbar_clicked || scroll_mode == SCROLL_BAR))
        {
            if (scroll_mode == SCROLL_KINETIC)
                kinetic_force_stop();
            scroll_mode = SCROLL_BAR;
            return scrollbar_scroll(gui_list, y);
        }

        /* |--------------------------------------------------------|
         * | Description of the touchscreen list interface:         |
         * |--------------------------------------------------------|
         * | Pressing an item will select it and "enter" it.        |
         * |                                                        |
         * | Pressing and holding your pen down will scroll through |
         * | the list of items.                                     |
         * |                                                        |
         * | Pressing and holding your pen down on a single item    |
         * | will bring up the context menu of it.                  |
         * |--------------------------------------------------------|
         */
        if (actual_y > 0 || button & BUTTON_REPEAT)
        {
            /* selection needs to be corrected if an items are only
             * partially visible */
            line = (actual_y - y_offset) / line_height;

            if (cancelled_kinetic)
            {
                kinetic_force_stop();
                scroll_mode = SCROLL_SWIPE;
            }

            /* Pressed below the list*/
            if (list_start_item + line >= gui_list->nb_items)
            {
                /* don't collect last_position outside of the list area
                 * it'd break selecting after such a situation */
                last_position = 0;
                return ACTION_NONE;
            }

            if (button & BUTTON_REPEAT && scroll_mode == SCROLL_NONE
                && !wait_for_release)
            {
                /* held a single line for a while, bring up the context menu */
                gui_synclist_select_item(gui_list, list_start_item + line);
                /* don't sent context repeatedly */
                wait_for_release = true;
                return ACTION_STD_CONTEXT;
            }
            if (released && !cancelled_kinetic)
            {
                /* Pen was released anywhere on the screen */
                last_position = 0;
                if (scroll_mode == SCROLL_NONE)
                {
                    /* select current line */
                    gui_synclist_select_item(gui_list, list_start_item + line);
                    return ACTION_STD_OK;
                }
                else
                {
                    /* we were scrolling
                     *  -> reset scrolling but do nothing else */
                    if (scroll_mode == SCROLL_SWIPE)
                    {
                        if (kinetic_setup_scroll(gui_list))
                            scroll_mode = SCROLL_KINETIC;
                    }
                    if (scroll_mode != SCROLL_KINETIC)
                        scroll_mode = SCROLL_NONE;
                    return ACTION_NONE;
                }
            }
            else
            {   /* pen is on the screen */
                bool redraw = false, result = false;
                /* beginning of list interaction denoted by release in
                 * the previous call */
                if (old_released || is_kinetic_over())
                {
                    scroll_mode = SCROLL_NONE;
                    redraw = true;
                }
                
                /* select current item; gui_synclist_select_item()
                 * is not called because it has side effects that
                 * disturb kinetic scrolling */
                gui_list->selected_item = list_start_item+line;
                gui_synclist_speak_item(gui_list);
                if (last_position == 0)
                    last_position = actual_y;
                else
                {
                    /* record speed data in case we do kinetic scrolling */
                    int diff = actual_y - last_position;
                    kinetic_stats_collect(diff);
                    result = swipe_scroll(gui_list, line_height, diff);
                }

                /* Start scrolling once the pen is moved without
                 * releasing it inbetween */
                if (result)
                {
                    redraw = true;
                    scroll_mode = SCROLL_SWIPE;
                }
                last_position = actual_y;

                return redraw ? ACTION_REDRAW:ACTION_NONE;
            }
        }
    }
    return ACTION_REDRAW;
}
#endif
