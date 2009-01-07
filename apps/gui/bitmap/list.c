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

#define SCROLLBAR_WIDTH 6
#define ICON_PADDING 1

/* these are static to make scrolling work */
static struct viewport list_text[NB_SCREENS], title_text[NB_SCREENS];

int gui_list_get_item_offset(struct gui_synclist * gui_list, int item_width,
                             int text_pos, struct screen * display,
                             struct viewport *vp);
bool list_display_title(struct gui_synclist *list, enum screen_type screen);

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
*/
static bool draw_title(struct screen *display, struct gui_synclist *list)
{
    const int screen = display->screen_type;
    if (!list_display_title(list, screen))
        return false;
    title_text[screen] = *(list->parent[screen]);
    title_text[screen].height
            = font_get(title_text[screen].font)->height;
    if (list->title_icon != Icon_NOICON && global_settings.show_icons)
    {
        struct viewport title_icon = *(list->parent[screen]);
        title_icon = title_text[screen];
        title_icon.width = get_icon_width(screen)
                          + ICON_PADDING*2;
        title_icon.x += ICON_PADDING;

        title_text[screen].width -= title_icon.width + title_icon.x;
        title_text[screen].x += title_icon.width + title_icon.x;

        display->set_viewport(&title_icon);
        screen_put_icon(display, 0, 0, list->title_icon);
    }
    title_text[screen].drawmode = STYLE_DEFAULT;
#ifdef HAVE_LCD_COLOR
    if (list->title_color >= 0)
    {
        title_text[screen].drawmode
                |= (STYLE_COLORED|list->title_color);
    }
#endif
    display->set_viewport(&title_text[screen]);
    display->puts_scroll_style(0, 0, list->title,
            title_text[screen].drawmode);
    return true;
}
    
void list_draw(struct screen *display, struct gui_synclist *list)
{
    struct viewport list_icons;
    int start, end, line_height, i;
    const int screen = display->screen_type;
    const int icon_width = get_icon_width(screen) + ICON_PADDING;
    const bool show_cursor = !global_settings.cursor_style &&
                        list->show_selection_marker;
    struct viewport *parent = (list->parent[screen]);
#ifdef HAVE_LCD_COLOR
    unsigned char cur_line = 0;
#endif
    int item_offset;
    bool show_title;
    line_height = font_get(parent->font)->height;
    display->set_viewport(parent);
    display->clear_viewport();
    display->stop_scroll();
    list_text[screen] = *parent;
    if ((show_title = draw_title(display, list)))
    {
        list_text[screen].y += line_height;
        list_text[screen].height -= line_height;
    }

    start = list->start_item[screen];
    end = start + viewport_get_nb_lines(&list_text[screen]);
    
    /* draw the scrollbar if its needed */
    if (global_settings.scrollbar &&
        viewport_get_nb_lines(&list_text[screen]) < list->nb_items)
    {
        struct viewport vp;
        vp = list_text[screen];
        vp.width = SCROLLBAR_WIDTH;
        list_text[screen].width -= SCROLLBAR_WIDTH;
        list_text[screen].x += SCROLLBAR_WIDTH;
        vp.height = line_height *
                    viewport_get_nb_lines(&list_text[screen]);
        vp.x = parent->x;
        display->set_viewport(&vp);
        gui_scrollbar_draw(display, 0, 0, SCROLLBAR_WIDTH-1,
                           vp.height, list->nb_items,
                           list->start_item[screen],
                           list->start_item[screen] + end-start,
                           VERTICAL);
    }
    else if (show_title)
    {
        /* shift everything right a bit... */
        list_text[screen].width -= SCROLLBAR_WIDTH;
        list_text[screen].x += SCROLLBAR_WIDTH;
    }
    
    /* setup icon placement */
    list_icons = list_text[screen];
    int icon_count = global_settings.show_icons && 
            (list->callback_get_item_icon != NULL) ? 1 : 0;
    if (show_cursor)
        icon_count++;
    if (icon_count)
    {
        list_icons.width = icon_width * icon_count;
        list_text[screen].width -= 
                list_icons.width + ICON_PADDING;
        list_text[screen].x += 
                list_icons.width + ICON_PADDING;
    }
    
    for (i=start; i<end && i<list->nb_items; i++)
    {
        /* do the text */
        unsigned char *s;
        char entry_buffer[MAX_PATH];
        unsigned char *entry_name;
        int text_pos = 0;
        s = list->callback_get_item_name(i, list->data, entry_buffer,
                                         sizeof(entry_buffer));
        entry_name = P2STR(s);
        display->set_viewport(&list_text[screen]);
        list_text[screen].drawmode = STYLE_DEFAULT;
        /* position the string at the correct offset place */
        int item_width,h;
        display->getstringsize(entry_name, &item_width, &h);
        item_offset = gui_list_get_item_offset(list, item_width,
                                               text_pos, display, 
                                               &list_text[screen]);

#ifdef HAVE_LCD_COLOR
        /* if the list has a color callback */
        if (list->callback_get_item_color)
        {
            int color = list->callback_get_item_color(i, list->data);
            /* if color selected */
            if (color >= 0)
            {
                list_text[screen].drawmode |= STYLE_COLORED|color;
            }
        }
#endif
        if(i >= list->selected_item && i <  list->selected_item
                + list->selected_size && list->show_selection_marker)
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
                list_text[screen].drawmode = STYLE_INVERT;
            }
#ifdef HAVE_LCD_COLOR
            else if (global_settings.cursor_style == 2)
            {
                /* Display colour line selector */
                list_text[screen].drawmode = STYLE_COLORBAR;
            }
            else if (global_settings.cursor_style == 3)
            {
                /* Display gradient line selector */
                list_text[screen].drawmode = STYLE_GRADIENT;

                /* Make the lcd driver know how many lines the gradient should
                   cover and current line number */
                /* number of selected lines */
                list_text[screen].drawmode |= NUMLN_PACK(list->selected_size);
                /* current line number, zero based */
                list_text[screen].drawmode |= CURLN_PACK(cur_line);
                cur_line++;
            }
#endif
            /* if the text is smaller than the viewport size */
            if (item_offset> item_width
                            - (list_text[screen].width - text_pos))
            {
                /* don't scroll */
                display->puts_style_offset(0, i-start, entry_name,
                        list_text[screen].drawmode, item_offset);
            }
            else
            {
                display->puts_scroll_style_offset(0, i-start, entry_name,
                        list_text[screen].drawmode, item_offset);
            }
        }
        else
        {
            if (list->scroll_all)
                display->puts_scroll_style_offset(0, i-start, entry_name,
                        list_text[screen].drawmode, item_offset);
            else
                display->puts_style_offset(0, i-start, entry_name,
                        list_text[screen].drawmode, item_offset);
        }
        /* do the icon */
        if (list->callback_get_item_icon && global_settings.show_icons)
        {
            display->set_viewport(&list_icons);
            screen_put_icon_with_offset(display, show_cursor?1:0,
                                    (i-start),show_cursor?ICON_PADDING:0,0,
                                    list->callback_get_item_icon(i, list->data));
            if (show_cursor && i >= list->selected_item &&
                i <  list->selected_item + list->selected_size)
            {
                screen_put_icon(display, 0, i-start, Icon_Cursor);
            }
        }
        else if (show_cursor && i >= list->selected_item &&
                i <  list->selected_item + list->selected_size)
        {
            display->set_viewport(&list_icons);
            screen_put_icon(display, 0, (i-start), Icon_Cursor);
        }
    }
    display->set_viewport(parent);
    display->update_viewport();
    display->set_viewport(NULL);
}


#if defined(HAVE_TOUCHSCREEN)
/* This needs to be fixed if we ever get more than 1 touchscreen on a target.
 * This also assumes the whole screen is used, which is a bad assumption but
 * fine until customizable lists comes in...
 */
static bool scrolling=false;

unsigned gui_synclist_do_touchscreen(struct gui_synclist * gui_list)
{
    short x, y;
    int button = action_get_touchscreen_press(&x, &y);
    int line;
    struct screen *display = &screens[SCREEN_MAIN];
    int screen = display->screen_type;
    if (button == BUTTON_NONE)
        return ACTION_NONE;
    if (x<list_text[screen].x)
    {
        /* Top left corner is hopefully GO_TO_ROOT */
        if (y<list_text[SCREEN_MAIN].y)
        {
            if (button == BUTTON_REL)
                return ACTION_STD_MENU;
            else if (button == (BUTTON_REPEAT|BUTTON_REL))
                return ACTION_STD_CONTEXT;
            else
                return ACTION_NONE;
        }
        /* Scroll bar */
        else
        {
            int nb_lines = viewport_get_nb_lines(&list_text[screen]);
            if (nb_lines <  gui_list->nb_items)
            {
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
                gui_synclist_select_item(gui_list, new_selection);
                
                return ACTION_REDRAW;
            }
        }
    }
    else
    {
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
        if (y > list_text[screen].y)
        {
            int line_height, actual_y;
            static int last_y = 0;
            
            actual_y =  y - list_text[screen].y;
            line_height = font_get(gui_list->parent[screen]->font)->height;
            line = actual_y / line_height;
            
            if(actual_y%line_height == 0) /* Pressed a border */
                return ACTION_NONE;

            if (gui_list->start_item[screen]+line > gui_list->nb_items)
            {
                 /* Pressed below the list*/
                return ACTION_NONE;
            }
            last_y = actual_y;
            if (line != gui_list->selected_item
                    - gui_list->start_item[screen] && button ^ BUTTON_REL)
            {
                if(button & BUTTON_REPEAT)
                    scrolling = true;
                gui_synclist_select_item(gui_list, gui_list->start_item[screen]
                        + line);
                return ACTION_REDRAW;
            }
            
            if (button == (BUTTON_REPEAT|BUTTON_REL))
            {
                if(!scrolling)
                {
                    /* Pen was hold on the same line as the 
                     * previously selected one
                     * => simulate long button press
                     */
                    return ACTION_STD_CONTEXT;
                }
                else
                {
                    /* Pen was moved across several lines and then released on 
                     * this one
                     * => do nothing
                     */
                    scrolling = false;
                    return ACTION_NONE;
                }
            }
            else if(button == BUTTON_REL)
            {
                /* Pen was released on either the same line as the previously
                 *  selected one or an other one
                 *  => simulate short press
                 */
                return ACTION_STD_OK;
            }
            else
                return ACTION_NONE;
        }
        /* Title goes up one level (only on BUTTON_REL&~BUTTON_REPEAT) */
        else if (y > title_text[screen].y && draw_title(display, gui_list)
                && button == BUTTON_REL)
        {
            return ACTION_STD_CANCEL;
        }
        /* Title or statusbar is cancel (only on BUTTON_REL&~BUTTON_REPEAT) */
        else if (global_settings.statusbar && button == BUTTON_REL)
        {
            return ACTION_STD_CANCEL;
        }
    }
    return ACTION_NONE;
}
#endif
