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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
#include "textarea.h"
#include "lang.h"
#include "sound.h"
#include "misc.h"
#include "talk.h"

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
static struct gui_list* last_list_displayed[NB_SCREENS];

#define SHOW_LIST_TITLE ((gui_list->title != NULL) && \
                         (gui_list->display->nb_lines > 2))

static void gui_list_select_at_offset(struct gui_list * gui_list, int offset);

/*
 * Initializes a scrolling list
 *  - gui_list : the list structure to initialize
 *  - callback_get_item_name : pointer to a function that associates a label
 *    to a given item number
 *  - data : extra data passed to the list callback
 *  - scroll_all : 
 *  - selected_size : 
 */
static void gui_list_init(struct gui_list * gui_list,
    list_get_name callback_get_item_name,
    void * data,
    bool scroll_all,
    int selected_size
    )
{
    gui_list->callback_get_item_icon = NULL;
    gui_list->callback_get_item_name = callback_get_item_name;
    gui_list->callback_speak_item = NULL;
    gui_list->display = NULL;
    gui_list_set_nb_items(gui_list, 0);
    gui_list->selected_item = 0;
    gui_list->start_item = 0;
    gui_list->limit_scroll = false;
    gui_list->data=data;
#ifdef HAVE_LCD_BITMAP
    gui_list->offset_position = 0;
#endif
    gui_list->scroll_all=scroll_all;
    gui_list->selected_size=selected_size;
    gui_list->title = NULL;
    gui_list->title_width = 0;
    gui_list->title_icon = Icon_NOICON;

    gui_list->last_displayed_selected_item = -1 ;
    gui_list->last_displayed_start_item = -1 ;
    gui_list->scheduled_talk_tick = gui_list->last_talked_tick = 0;
    gui_list->show_selection_marker = true;

#ifdef HAVE_LCD_COLOR
    gui_list->title_color = -1;
    gui_list->callback_get_item_color = NULL;
#endif
}

/* this toggles the selection bar or cursor */
void gui_synclist_hide_selection_marker(struct gui_synclist * lists, bool hide)
{
    int i;
    FOR_NB_SCREENS(i)
        lists->gui_list[i].show_selection_marker = !hide;
}

/*
 * Attach the scrolling list to a screen
 * (The previous screen attachement is lost)
 *  - gui_list : the list structure
 *  - display : the screen to attach
 */
static void gui_list_set_display(struct gui_list * gui_list, struct screen * display)
{
    if(gui_list->display != 0) /* we switched from a previous display */
        gui_list->display->stop_scroll();
    gui_list->display = display;
#ifdef HAVE_LCD_CHARCELLS
    display->double_height(false);
#endif
    gui_list_select_at_offset(gui_list, 0);
}

#ifdef HAVE_LCD_BITMAP
static int gui_list_get_item_offset(struct gui_list * gui_list, int item_width,
                             int text_pos)
{
    struct screen * display=gui_list->display;
    int item_offset;

    if (offset_out_of_view)
    {
        item_offset = gui_list->offset_position;
    }
    else
    {
        /* if text is smaller then view */
        if (item_width <= display->width - text_pos)
        {
            item_offset = 0;
        }
        else
        {
            /* if text got out of view  */
            if (gui_list->offset_position >
                    item_width - (display->width - text_pos))
                item_offset = item_width - (display->width - text_pos);
            else
                item_offset = gui_list->offset_position;
        }
    }

    return item_offset;
}
#endif

/*
 * Draws the list on the attached screen
 * - gui_list : the list structure
 */
static void gui_list_draw_smart(struct gui_list *gui_list)
{
    struct screen * display=gui_list->display;
    int text_pos;
    bool draw_icons = (gui_list->callback_get_item_icon != NULL && global_settings.show_icons);
    bool draw_cursor;
    int i;
    int lines;
    static int last_lines[NB_SCREENS] = {0};
#ifdef HAVE_LCD_BITMAP
    int item_offset;
    int old_margin = display->getxmargin();
#endif
    int start, end;
    bool partial_draw = false;

#ifdef HAVE_LCD_BITMAP
    display->setfont(FONT_UI);
    gui_textarea_update_nblines(display);
#endif
    /* Speed up UI by drawing the changed contents only. */
    if (gui_list == last_list_displayed[gui_list->display->screen_type]
        && gui_list->last_displayed_start_item == gui_list->start_item
        && gui_list->selected_size == 1)
    {
        partial_draw = true;
    }

    lines = display->nb_lines - SHOW_LIST_TITLE;
    if (last_lines[display->screen_type] != lines)
    {
        gui_list_select_at_offset(gui_list, 0);
        last_lines[display->screen_type] = lines;
    }

    if (partial_draw)
    {
        end = gui_list->last_displayed_selected_item - gui_list->start_item;
        i = gui_list->selected_item - gui_list->start_item;
        if (i < end )
        {
            start = i;
            end++;
        }
        else
        {
            start = end;
            end = i + 1;
        }
    }
    else
    {
        gui_textarea_clear(display);
        start = 0;
        end = display->nb_lines;
        gui_list->last_displayed_start_item = gui_list->start_item;
        last_list_displayed[gui_list->display->screen_type] = gui_list;
    }

    gui_list->last_displayed_selected_item = gui_list->selected_item;

    /* position and draw the list title & icon */
    if (SHOW_LIST_TITLE && !partial_draw)
    {
        if (gui_list->title_icon != NOICON && draw_icons)
        {
            screen_put_icon(display, 0, 0, gui_list->title_icon);
#ifdef HAVE_LCD_BITMAP
            text_pos = get_icon_width(display->screen_type)+2; /* pixels */
#else
            text_pos = 1; /* chars */
#endif
        }
        else
        {
            text_pos = 0;
        }

#ifdef HAVE_LCD_BITMAP
        int title_style = STYLE_DEFAULT;
#ifdef HAVE_LCD_COLOR
        if (gui_list->title_color >= 0)
        {
            title_style |= STYLE_COLORED;
            title_style |= gui_list->title_color;
        }
#endif
        screen_set_xmargin(display, text_pos); /* margin for title */
        item_offset = gui_list_get_item_offset(gui_list, gui_list->title_width,
                                               text_pos);
        if (item_offset > gui_list->title_width - (display->width - text_pos))
            display->puts_style_offset(0, 0, gui_list->title,
                                       title_style, item_offset);
        else
            display->puts_scroll_style_offset(0, 0, gui_list->title,
                                              title_style, item_offset);
#else
        display->puts_scroll(text_pos, 0, gui_list->title);
#endif
    }

    /* Adjust the position of icon, cursor, text for the list */
#ifdef HAVE_LCD_BITMAP
    gui_textarea_update_nblines(display);
    bool draw_scrollbar;

    draw_scrollbar = (global_settings.scrollbar &&
                lines < gui_list->nb_items);

    draw_cursor = !global_settings.cursor_style &&
                    gui_list->show_selection_marker;
    text_pos = 0; /* here it's in pixels */
    if(draw_scrollbar || SHOW_LIST_TITLE) /* indent if there's
                                             a title */
    {
        text_pos += SCROLLBAR_WIDTH;
    }
    if(draw_cursor)
        text_pos += get_icon_width(display->screen_type) + 2;

    if(draw_icons)
        text_pos += get_icon_width(display->screen_type) + 2;
#else
    draw_cursor = true;
    if(draw_icons)
        text_pos = 2; /* here it's in chars */
    else
        text_pos = 1;
#endif

#ifdef HAVE_LCD_BITMAP
    screen_set_xmargin(display, text_pos); /* margin for list */
#endif

    if (SHOW_LIST_TITLE)
    {
        start++;
        if (end < display->nb_lines)
            end++;
    }

#ifdef HAVE_LCD_BITMAP
    unsigned char cur_line = 1;
#endif
    for (i = start; i < end; i++)
    {
        unsigned char *s;
        char entry_buffer[MAX_PATH];
        unsigned char *entry_name;
        int current_item = gui_list->start_item +
                           (SHOW_LIST_TITLE ? i-1 : i);

        /* When there are less items to display than the
         * current available space on the screen, we stop*/
        if(current_item >= gui_list->nb_items)
            break;
        s = gui_list->callback_get_item_name(current_item,
                                             gui_list->data,
                                             entry_buffer);
        entry_name = P2STR(s);

#ifdef HAVE_LCD_BITMAP
        int style = STYLE_DEFAULT;
        /* position the string at the correct offset place */
        int item_width,h;
        display->getstringsize(entry_name, &item_width, &h);
        item_offset = gui_list_get_item_offset(gui_list, item_width, text_pos);
#endif

#ifdef HAVE_LCD_COLOR
        /* if the list has a color callback */
        if (gui_list->callback_get_item_color)
        {
            int color = gui_list->callback_get_item_color(current_item,
                                                          gui_list->data);
            /* if color selected */
            if (color >= 0)
            {
                style |= STYLE_COLORED;
                style |= color;
            }
        }
#endif

        if(gui_list->show_selection_marker &&
           current_item >= gui_list->selected_item &&
           current_item <  gui_list->selected_item + gui_list->selected_size)
        {/* The selected item must be displayed scrolling */
#ifdef HAVE_LCD_BITMAP
            if (global_settings.cursor_style == 1
#ifdef HAVE_REMOTE_LCD
                || display->screen_type == SCREEN_REMOTE
#endif
               )
            {
                /* Display inverted-line-style */
                style |= STYLE_INVERT;
            }
#ifdef HAVE_LCD_COLOR
            else if (global_settings.cursor_style == 2)
            {
                /* Display colour line selector */
                style |= STYLE_COLORBAR;
            }
            else if (global_settings.cursor_style == 3)
            {
                /* Display gradient line selector */
                style = STYLE_GRADIENT;

                /* Make the lcd driver know how many lines the gradient should
                   cover and current line number */
                /* max line number*/
                style |= MAXLN_PACK(gui_list->selected_size);
                /* current line number */
                style |= CURLN_PACK(cur_line);
                cur_line++;
            }
#endif
            else  /*  if (!global_settings.cursor_style) */
            {
                if (current_item % gui_list->selected_size != 0)
                    draw_cursor = false;
            }
            /* if the text is smaller than the viewport size */
            if (item_offset > item_width - (display->width - text_pos))
            {
                /* don't scroll */
                display->puts_style_offset(0, i, entry_name,
                                           style, item_offset);
            }
            else
            {
                display->puts_scroll_style_offset(0, i, entry_name,
                                                  style, item_offset);
            }
#else
            display->puts_scroll(text_pos, i, entry_name);
#endif

            if (draw_cursor)
            {
                screen_put_icon_with_offset(display, 0, i,
                                           (draw_scrollbar || SHOW_LIST_TITLE)?
                                                   SCROLLBAR_WIDTH: 0,
                                           0, Icon_Cursor);
            }
        }
        else
        {/* normal item */
            if(gui_list->scroll_all)
            {
#ifdef HAVE_LCD_BITMAP
                display->puts_scroll_style_offset(0, i, entry_name,
                                                  style, item_offset);
#else
                display->puts_scroll(text_pos, i, entry_name);
#endif
            }
            else
            {
#ifdef HAVE_LCD_BITMAP
                display->puts_style_offset(0, i, entry_name,
                                           style, item_offset);
#else
                display->puts(text_pos, i, entry_name);
#endif
            }
        }
        /* Icons display */
        if(draw_icons)
        {
            enum themable_icons icon;
            icon = gui_list->callback_get_item_icon(current_item, gui_list->data);
            if(icon > Icon_NOICON)
            {
#ifdef HAVE_LCD_BITMAP
                int x = draw_cursor?1:0;
                int x_off = (draw_scrollbar || SHOW_LIST_TITLE) ? SCROLLBAR_WIDTH: 0;
                screen_put_icon_with_offset(display, x, i,
                                           x_off, 0, icon);
#else
                screen_put_icon(display, 1, i, icon);
#endif
            }
        }
    }

#ifdef HAVE_LCD_BITMAP
    /* Draw the scrollbar if needed*/
    if(draw_scrollbar)
    {
        int y_start = gui_textarea_get_ystart(display);
        if (SHOW_LIST_TITLE)
            y_start += display->char_height;
        int scrollbar_y_end = display->char_height *
                              lines + y_start;
        gui_scrollbar_draw(display, 0, y_start, SCROLLBAR_WIDTH-1,
                           scrollbar_y_end - y_start, gui_list->nb_items,
                           gui_list->start_item,
                           gui_list->start_item + lines, VERTICAL);
    }

    screen_set_xmargin(display, old_margin);
#endif

    gui_textarea_update(display);
}

/*
 * Force a full screen update.
 */
static void gui_list_draw(struct gui_list *gui_list)
{
    last_list_displayed[gui_list->display->screen_type] = NULL;
    return gui_list_draw_smart(gui_list);
}

/*
 * Selects an item in the list
 *  - gui_list : the list structure
 *  - item_number : the number of the item which will be selected
 */
static void gui_list_select_item(struct gui_list * gui_list, int item_number)
{
    if( item_number > gui_list->nb_items-1 || item_number < 0 )
        return;
    gui_list->selected_item = item_number;
    gui_list_select_at_offset(gui_list, 0);
}

/* select an item above the current one */
static void gui_list_select_above(struct gui_list * gui_list,
                                  int items, int nb_lines)
{
    gui_list->selected_item -= items;
    
    /* in bottom "3rd" of the screen, so dont move the start item.
       by 3rd I mean above SCROLL_LIMIT lines above the end of the screen */
    if (items && gui_list->start_item + SCROLL_LIMIT < gui_list->selected_item)
    {
        if (gui_list->show_selection_marker == false)
        {
            gui_list->start_item -= items;
            if (gui_list->start_item < 0)
                gui_list->start_item = 0;
        }
        return;
    }
    if (gui_list->selected_item < 0)
    {
        if(gui_list->limit_scroll)
        {
            gui_list->selected_item = 0;
            gui_list->start_item = 0;
        }
        else
        {
            gui_list->selected_item += gui_list->nb_items;
            if (global_settings.scroll_paginated)
            {
                gui_list->start_item = gui_list->nb_items - nb_lines;
            }
        }
    }
    if (gui_list->nb_items > nb_lines)
    {
        if (global_settings.scroll_paginated)
        {
            if (gui_list->start_item > gui_list->selected_item)
                gui_list->start_item = MAX(0, gui_list->start_item - nb_lines);
        }
        else
        {
            int top_of_screen = gui_list->selected_item - SCROLL_LIMIT;
            int temp = MIN(top_of_screen, gui_list->nb_items - nb_lines);
            gui_list->start_item = MAX(0, temp);
        }
    }
    else gui_list->start_item = 0;
    if (gui_list->selected_size > 1)
    {
        if (gui_list->start_item + nb_lines == gui_list->selected_item)
            gui_list->start_item++;
    }
}
/* select an item below the current one */
static void gui_list_select_below(struct gui_list * gui_list, 
                                  int items, int nb_lines)
{
    int bottom;
    
    gui_list->selected_item += items;
    bottom = gui_list->nb_items - nb_lines;
    
    /* always move the screen if selection isnt "visible" */
    if (items && gui_list->show_selection_marker == false)
    {
        if (bottom < 0)
            bottom = 0;
        gui_list->start_item = MIN(bottom, gui_list->start_item +
                                           items);
        return;
    }
    /* in top "3rd" of the screen, so dont move the start item */
    if (items && 
        (gui_list->start_item + nb_lines - SCROLL_LIMIT > gui_list->selected_item)
        && (gui_list->selected_item < gui_list->nb_items))
    {
        if (gui_list->show_selection_marker == false)
        {
            if (bottom < 0)
                bottom = 0;
            gui_list->start_item = MIN(bottom, 
                                       gui_list->start_item + items);
        }
        return;
    }
    
    if (gui_list->selected_item >= gui_list->nb_items)
    {
        if(gui_list->limit_scroll)
        {
            gui_list->selected_item = gui_list->nb_items-gui_list->selected_size;
            gui_list->start_item = MAX(0,gui_list->nb_items - nb_lines);
        }
        else
        {
            gui_list->selected_item = 0;
            gui_list->start_item = 0;
        }
        return;
    }
    
    if (gui_list->nb_items > nb_lines)
    {
        if (global_settings.scroll_paginated)
        {
            if (gui_list->start_item + nb_lines <= gui_list->selected_item)
                gui_list->start_item = MIN(bottom, gui_list->selected_item);
        }
        else
        {
            int top_of_screen = gui_list->selected_item + SCROLL_LIMIT - nb_lines;
            int temp = MAX(0, top_of_screen);
            gui_list->start_item = MIN(bottom, temp);
        }
    }
    else gui_list->start_item = 0;
}

static void gui_list_select_at_offset(struct gui_list * gui_list, int offset)
{
    /* do this here instead of in both select_above and select_below */
    int nb_lines = gui_list->display->nb_lines;
    if (SHOW_LIST_TITLE)
        nb_lines--;
    
    if (gui_list->selected_size > 1)
    {
        offset *= gui_list->selected_size;
        /* always select the first item of multi-line lists */
        offset -= offset%gui_list->selected_size;
    }
    if (offset == 0 && global_settings.scroll_paginated &&
        (gui_list->nb_items > nb_lines))
    {
        int bottom = gui_list->nb_items - nb_lines;
        gui_list->start_item = MIN(gui_list->selected_item, bottom);
    }
    else if (offset < 0)
        gui_list_select_above(gui_list, -offset, nb_lines);
    else
        gui_list_select_below(gui_list, offset, nb_lines);
}

/*
 * Adds an item to the list (the callback will be asked for one more item)
 * - gui_list : the list structure
 */
static void gui_list_add_item(struct gui_list * gui_list)
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
static void gui_list_del_item(struct gui_list * gui_list)
{
    if(gui_list->nb_items > 0)
    {
        gui_textarea_update_nblines(gui_list->display);
        int nb_lines = gui_list->display->nb_lines;

        int dist_selected_from_end = gui_list->nb_items
            - gui_list->selected_item - 1;
        int dist_start_from_end = gui_list->nb_items
            - gui_list->start_item - 1;
        if(dist_selected_from_end == 0)
        {
            /* Oops we are removing the selected item,
               select the previous one */
            gui_list->selected_item--;
        }
        gui_list->nb_items--;

        /* scroll the list if needed */
        if( (dist_start_from_end < nb_lines) && (gui_list->start_item != 0) )
            gui_list->start_item--;
    }
}

#ifdef HAVE_LCD_BITMAP

/*
 * Makes all the item in the list scroll by one step to the right.
 * Should stop increasing the value when reaching the widest item value 
 * in the list.
 */
static void gui_list_scroll_right(struct gui_list * gui_list)
{
    /* FIXME: This is a fake right boundry limiter. there should be some
     * callback function to find the longest item on the list in pixels,
     * to stop the list from scrolling past that point */
    gui_list->offset_position+=offset_step;
    if (gui_list->offset_position > 1000)
        gui_list->offset_position = 1000;
}

/*
 * Makes all the item in the list scroll by one step to the left.
 * stops at starting position.
 */
static void gui_list_scroll_left(struct gui_list * gui_list)
{
    gui_list->offset_position-=offset_step;
    if (gui_list->offset_position < 0)
        gui_list->offset_position = 0;
}
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
static void gui_list_set_title(struct gui_list * gui_list, 
                               char * title, enum themable_icons icon)
{
    gui_list->title = title;
    gui_list->title_icon = icon;
    if (title) {
#ifdef HAVE_LCD_BITMAP
        gui_list->display->getstringsize(title, &gui_list->title_width, NULL);
#else
        gui_list->title_width = strlen(title);
#endif
    } else {
        gui_list->title_width = 0;
    }
}

/*
 * Synchronized lists stuffs
 */
void gui_synclist_init(
    struct gui_synclist * lists,
    list_get_name callback_get_item_name,
    void * data,
    bool scroll_all,
    int selected_size
    )
{
    int i;
    FOR_NB_SCREENS(i)
    {
        gui_list_init(&(lists->gui_list[i]),
                      callback_get_item_name,
                      data, scroll_all, selected_size);
        gui_list_set_display(&(lists->gui_list[i]), &(screens[i]));
    }
}

void gui_synclist_set_nb_items(struct gui_synclist * lists, int nb_items)
{
    int i;
    FOR_NB_SCREENS(i)
    {
        gui_list_set_nb_items(&(lists->gui_list[i]), nb_items);
#ifdef HAVE_LCD_BITMAP
        lists->gui_list[i].offset_position = 0;
#endif
    }
}
int gui_synclist_get_nb_items(struct gui_synclist * lists)
{
    return gui_list_get_nb_items(&((lists)->gui_list[0]));
}
int  gui_synclist_get_sel_pos(struct gui_synclist * lists)
{
    return gui_list_get_sel_pos(&((lists)->gui_list[0]));
}
void gui_synclist_set_icon_callback(struct gui_synclist * lists,
                                    list_get_icon icon_callback)
{
    int i;
    FOR_NB_SCREENS(i)
    {
        gui_list_set_icon_callback(&(lists->gui_list[i]), icon_callback);
    }
}

void gui_synclist_set_voice_callback(struct gui_synclist * lists,
                                    list_speak_item voice_callback)
{
    gui_list_set_voice_callback(&(lists->gui_list[0]), voice_callback);
}

void gui_synclist_draw(struct gui_synclist * lists)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_list_draw(&(lists->gui_list[i]));
}

void gui_synclist_select_item(struct gui_synclist * lists, int item_number)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_list_select_item(&(lists->gui_list[i]), item_number);
}

static void gui_synclist_select_next_page(struct gui_synclist * lists,
                                   enum screen_type screen)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_list_select_at_offset(&(lists->gui_list[i]),
                                  screens[screen].nb_lines);
}

static void gui_synclist_select_previous_page(struct gui_synclist * lists,
                                       enum screen_type screen)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_list_select_at_offset(&(lists->gui_list[i]),
                                      -screens[screen].nb_lines);
}

void gui_synclist_add_item(struct gui_synclist * lists)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_list_add_item(&(lists->gui_list[i]));
}

void gui_synclist_del_item(struct gui_synclist * lists)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_list_del_item(&(lists->gui_list[i]));
}

void gui_synclist_limit_scroll(struct gui_synclist * lists, bool scroll)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_list_limit_scroll(&(lists->gui_list[i]), scroll);
}

void gui_synclist_set_title(struct gui_synclist * lists,
                            char * title, enum themable_icons icon)
{
    int i;
    FOR_NB_SCREENS(i)
            gui_list_set_title(&(lists->gui_list[i]), title, icon);
}

#ifdef HAVE_LCD_BITMAP
static void gui_synclist_scroll_right(struct gui_synclist * lists)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_list_scroll_right(&(lists->gui_list[i]));
}

static void gui_synclist_scroll_left(struct gui_synclist * lists)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_list_scroll_left(&(lists->gui_list[i]));
}
#endif /* HAVE_LCD_BITMAP */

static void _gui_synclist_speak_item(struct gui_synclist *lists, bool repeating)
{
    struct gui_list *l = &lists->gui_list[0];
    list_speak_item *cb = l->callback_speak_item;
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
           || (l->scheduled_talk_tick
               && TIME_BEFORE(current_tick, l->scheduled_talk_tick))
           || (l->last_talked_tick
               && TIME_BEFORE(current_tick, l->last_talked_tick +HZ/4)))
        {
            l->scheduled_talk_tick = current_tick +HZ/4;
            return;
        } else {
            l->scheduled_talk_tick = 0; /* work done */
            cb(sel, l->data);
            l->last_talked_tick = current_tick;
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

#if  defined(HAVE_TOUCHPAD)
unsigned gui_synclist_do_touchpad(struct gui_synclist * lists)
{
    struct gui_list *gui_list = &(lists->gui_list[SCREEN_MAIN]);
    short x,y;
    unsigned button = action_get_touchpad_press(&x, &y);
    int line;
    if (button == BUTTON_NONE)
        return ACTION_NONE;
    if (x<SCROLLBAR_WIDTH)
    {
        /* top left corner is hopefully GO_TO_ROOT */
        if (y<STATUSBAR_HEIGHT)
        {
            if (button == BUTTON_REL)
                return ACTION_STD_MENU;
            else if (button == BUTTON_REPEAT)
                return ACTION_STD_CONTEXT;
            else
                return ACTION_NONE;
        }
        /* scroll bar */
        else
        {
            int new_selection, nb_lines;
            int height, size;
            nb_lines = gui_list->display->nb_lines - SHOW_LIST_TITLE;
            if (nb_lines <  gui_list->nb_items)
            {
                height = nb_lines * gui_list->display->char_height;
                size = height*nb_lines / gui_list->nb_items;
                new_selection = (y*(gui_list->nb_items-nb_lines))/(height-size);
                gui_synclist_select_item(lists, new_selection);
                nb_lines /= 2;
                if (new_selection - gui_list->start_item > nb_lines)
                {
                    new_selection = gui_list->start_item+nb_lines;
                }
                FOR_NB_SCREENS(line)
                    lists->gui_list[line].selected_item = new_selection;
                return ACTION_REDRAW;
            }
        }
    }
    else
    {
        if (button != BUTTON_REL && button != BUTTON_REPEAT)
        {
            if (global_settings.statusbar)
                y -= STATUSBAR_HEIGHT;
            if (SHOW_LIST_TITLE)
                y -= gui_list->display->char_height;
            line = y / gui_list->display->char_height;
            if (line != gui_list->selected_item - gui_list->start_item)
                gui_synclist_select_item(lists, gui_list->start_item+line);
            return ACTION_REDRAW;
        }
        /* title or statusbar is cancel */
        if (global_settings.statusbar)
        {
            if (y < STATUSBAR_HEIGHT && !SHOW_LIST_TITLE )
                return ACTION_STD_CANCEL;
            y -= STATUSBAR_HEIGHT;
        }
        /* title goes up one level */
        if (SHOW_LIST_TITLE)
        {
            if (y < gui_list->display->char_height)
                return ACTION_STD_CANCEL;
            y -= gui_list->display->char_height;
        }
        /*  pressing an item will select it.
            pressing the selected item will "enter" it */
        line = y / gui_list->display->char_height;
        if (line != gui_list->selected_item - gui_list->start_item)
        {
            if (gui_list->start_item+line > gui_list->nb_items)
                return ACTION_NONE;
            gui_synclist_select_item(lists, gui_list->start_item+line);
        }
        
        if (button == BUTTON_REPEAT)
            return ACTION_STD_CONTEXT;
        else
            return ACTION_STD_OK;
    }
    return ACTION_NONE;
}
#endif

bool gui_synclist_do_button(struct gui_synclist * lists,
                            unsigned *actionptr, enum list_wrap wrap)
{
    int action = *actionptr;
#ifdef HAVE_LCD_BITMAP
    static bool scrolling_left = false;
#endif
    int i;

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
    
#if defined(HAVE_TOUCHPAD)
    if (action == ACTION_TOUCHPAD)
        action = *actionptr = gui_synclist_do_touchpad(lists);
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
            FOR_NB_SCREENS(i)
                gui_list_select_at_offset(&(lists->gui_list[i]), -next_item_modifier);
#ifndef HAVE_SCROLLWHEEL
            if (queue_count(&button_queue) < FRAMEDROP_TRIGGER)
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
            FOR_NB_SCREENS(i)
                gui_list_select_at_offset(&(lists->gui_list[i]), next_item_modifier);
#ifndef HAVE_SCROLLWHEEL
            if (queue_count(&button_queue) < FRAMEDROP_TRIGGER)
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
            if (lists->gui_list[0].offset_position == 0)
            {
                scrolling_left = false;
                *actionptr = ACTION_STD_CANCEL;
                return true;
            }
            *actionptr = ACTION_TREE_PGLEFT;
        case ACTION_TREE_PGLEFT:
            if(!scrolling_left && (lists->gui_list[0].offset_position == 0))
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
    if(lists->gui_list[0].scheduled_talk_tick
       && TIME_AFTER(current_tick, lists->gui_list[0].scheduled_talk_tick))
        /* scheduled postponed item announcement is due */
        _gui_synclist_speak_item(lists, false);
    return false;
}

int list_do_action_timeout(struct gui_synclist *lists, int timeout)
/* Returns the lowest of timeout or the delay until a postponed
   scheduled announcement is due (if any). */
{
    if(lists->gui_list[0].scheduled_talk_tick)
    {
        long delay = lists->gui_list[0].scheduled_talk_tick -current_tick +1;
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

/* Simple use list implementation */
static int simplelist_line_count = 0;
static char simplelist_text[SIMPLELIST_MAX_LINES][SIMPLELIST_MAX_LINELENGTH];
/* set the amount of lines shown in the list */
void simplelist_set_line_count(int lines)
{
    if (lines < 0)
        lines = 0;
    else if (lines > SIMPLELIST_MAX_LINES)
        lines = SIMPLELIST_MAX_LINES;
    simplelist_line_count = 0;
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

static char* simplelist_static_getname(int item, void * data, char *buffer)
{
    (void)data; (void)buffer;
    return simplelist_text[item];
}
bool simplelist_show_list(struct simplelist_info *info)
{
    struct gui_synclist lists;
    int action, old_line_count = simplelist_line_count;
    char* (*getname)(int item, void * data, char *buffer);
    if (info->get_name)
        getname = info->get_name;
    else
        getname = simplelist_static_getname;
    gui_synclist_init(&lists, getname,  info->callback_data, 
                      info->scroll_all, info->selection_size);
    if (info->title)
        gui_synclist_set_title(&lists, info->title, NOICON);
    if (info->get_icon)
        gui_synclist_set_icon_callback(&lists, info->get_icon);
    if (info->get_talk)
        gui_synclist_set_voice_callback(&lists, info->get_talk);
    
    gui_synclist_hide_selection_marker(&lists, info->hide_selection);
    
    if (info->action_callback)
        info->action_callback(ACTION_REDRAW, &lists);

    if (info->get_name == NULL)
        gui_synclist_set_nb_items(&lists, simplelist_line_count*info->selection_size);
    else
        gui_synclist_set_nb_items(&lists, info->count*info->selection_size);
    
    gui_synclist_select_item(&lists, info->start_selection);
    
    gui_synclist_draw(&lists);
    gui_synclist_speak_item(&lists);

    while(1)
    {
        gui_syncstatusbar_draw(&statusbars, true);
        list_do_action(CONTEXT_STD, info->timeout,
                       &lists, &action, LIST_WRAP_UNLESS_HELD);

        /* We must yield in this case or no other thread can run */
        if (info->timeout == TIMEOUT_NOBLOCK)
            yield();

        if (info->action_callback)
        {
            action = info->action_callback(action, &lists);
            if (info->get_name == NULL)
                gui_synclist_set_nb_items(&lists, simplelist_line_count*info->selection_size);
        }
        if (action == ACTION_STD_CANCEL)
            break;
        else if ((action == ACTION_NONE) ||
                 (action == ACTION_REDRAW) || 
                 (old_line_count != simplelist_line_count))
        {
            if (info->get_name == NULL)
                gui_synclist_set_nb_items(&lists, simplelist_line_count*info->selection_size);
            gui_synclist_draw(&lists);
            if (action != ACTION_NONE)
                gui_synclist_speak_item(&lists);
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
    info->start_selection = 0;
    info->action_callback = NULL;
    info->get_icon = NULL;
    info->get_name = NULL;
    info->get_talk = NULL;
    info->callback_data = data;
}







