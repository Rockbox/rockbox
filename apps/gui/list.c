/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Kevin FERRARE
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
#include "settings.h"
#include "kernel.h"

#include "screen_access.h"
#include "list.h"
#include "scrollbar.h"
#include "statusbar.h"

#ifdef HAVE_LCD_CHARCELLS
#define SCROLL_LIMIT 1
#else
#define SCROLL_LIMIT 2
#endif



void gui_list_init(struct gui_list * gui_list,
    void (*callback_get_item_icon)
        (int selected_item, void * data, ICON * icon),
    char * (*callback_get_item_name)
        (int selected_item, void * data, char *buffer),
    void * data
    )
{
    gui_list->callback_get_item_icon = callback_get_item_icon;
    gui_list->callback_get_item_name = callback_get_item_name;
    gui_list->display = NULL;
    gui_list_set_nb_items(gui_list, 0);
    gui_list->selected_item = 0;
    gui_list->start_item = 0;
    gui_list->limit_scroll = false;
    gui_list->data=data;
}

void gui_list_set_display(struct gui_list * gui_list, struct screen * display)
{
    if(gui_list->display != 0) /* we switched from a previous display */
        gui_list->display->stop_scroll();
    gui_list->display = display;
#ifdef HAVE_LCD_CHARCELLS
    display->double_height(false);
#endif
    gui_list_put_selection_in_screen(gui_list, false);
}

void gui_list_put_selection_in_screen(struct gui_list * gui_list,
                                      bool put_from_end)
{
    int nb_lines=gui_list->display->nb_lines;
    if(put_from_end)
    {
        int list_end = gui_list->selected_item + SCROLL_LIMIT - 1;
        if(list_end > gui_list->nb_items)
            list_end = nb_lines;
        gui_list->start_item = list_end - nb_lines;
    }
    else
    {
        int list_start = gui_list->selected_item - SCROLL_LIMIT + 1;
        if(list_start + nb_lines > gui_list->nb_items)
            list_start = gui_list->nb_items - nb_lines;
        gui_list->start_item = list_start;
    }
    if(gui_list->start_item < 0)
        gui_list->start_item = 0;
}

void gui_list_draw(struct gui_list * gui_list)
{
    struct screen * display=gui_list->display;
    int cursor_pos = 0;
    int icon_pos = 1;
    int text_pos;
    bool draw_icons = (gui_list->callback_get_item_icon != NULL &&
                       global_settings.show_icons) ;
    bool draw_cursor;
    int i;

    /* Adjust the position of icon, cursor, text */
#ifdef HAVE_LCD_BITMAP
    bool draw_scrollbar = (global_settings.scrollbar &&
                           display->nb_lines < gui_list->nb_items);

    int list_y_start = screen_get_text_y_start(gui_list->display);
    int list_y_end = screen_get_text_y_end(gui_list->display);

    draw_cursor = !global_settings.invert_cursor;
    text_pos = 0; /* here it's in pixels */
    if(draw_scrollbar)
    {
        cursor_pos++;
        icon_pos++;
        text_pos += SCROLLBAR_WIDTH;
    }
    if(!draw_cursor)
    {
        icon_pos--;
    }
    else
        text_pos += CURSOR_WIDTH;

    if(draw_icons)
        text_pos += 8;
#else
    draw_cursor = true;
    if(draw_icons)
        text_pos = 2; /* here it's in chars */
    else
        text_pos = 1;
#endif
    /* The drawing part */
#ifdef HAVE_LCD_BITMAP
    /* clear the drawing area */
    display->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    display->fillrect(0, list_y_start,
                      display->width, list_y_end - list_y_start);
    display->set_drawmode(DRMODE_SOLID);

    display->setfont(FONT_UI);
    screen_update_nblines(display);

    display->stop_scroll();
    display->setmargins(text_pos, list_y_start);
#else
    display->clear_display();
#endif

    for(i = 0;i < display->nb_lines;i++)
    {
        char entry_buffer[MAX_PATH];
        char * entry_name;
        int current_item = gui_list->start_item + i;

        /* When there are less items to display than the
         * current available space on the screen, we stop*/
        if(current_item >= gui_list->nb_items)
            break;
        entry_name = gui_list->callback_get_item_name(current_item,
                                                      gui_list->data,
                                                      entry_buffer);
        if(current_item == gui_list->selected_item)
        {
            /* The selected item must be displayed scrolling */
#ifdef HAVE_LCD_BITMAP
            if (global_settings.invert_cursor)/* Display inverted-line-style*/
                display->puts_scroll_style(0, i, entry_name, STYLE_INVERT);
            else
                display->puts_scroll(0, i, entry_name);
#else
                display->puts_scroll(text_pos, i, entry_name);
#endif

            if(draw_cursor)
                screen_put_cursorxy(display, cursor_pos, i);
        }
        else
        {/* normal item */
#ifdef HAVE_LCD_BITMAP
            display->puts(0, i, entry_name);
#else
            display->puts(text_pos, i, entry_name);
#endif
        }
        /* Icons display */
        if(draw_icons)
        {
            ICON icon;
            gui_list->callback_get_item_icon(current_item,
                                             gui_list->data,
                                             &icon);
            screen_put_iconxy(display, icon_pos, i, icon);
        }
    }
#ifdef HAVE_LCD_BITMAP
    /* Draw the scrollbar if needed*/
    if(draw_scrollbar)
    {
        int scrollbar_y_end = display->char_height *
                              display->nb_lines + list_y_start;
        gui_scrollbar_draw(display, 0, list_y_start, SCROLLBAR_WIDTH-1,
                           scrollbar_y_end - list_y_start, gui_list->nb_items,
                           gui_list->start_item,
                           gui_list->start_item + display->nb_lines, VERTICAL);
    }
    display->update_rect(0, list_y_start, display->width,
                         list_y_end - list_y_start);
#else
#ifdef SIMULATOR
    display->update();
#endif
#endif
}

void gui_list_select_item(struct gui_list * gui_list, int item_number)
{
    if( item_number > gui_list->nb_items-1 || item_number < 0 )
        return;
    gui_list->selected_item = item_number;
    gui_list_put_selection_in_screen(gui_list, false);
}

void gui_list_select_next(struct gui_list * gui_list)
{
    int item_pos;
    int end_item;

    if( gui_list->selected_item == gui_list->nb_items-1 )
    {
        if(gui_list->limit_scroll)
            return;
        gui_list->selected_item++;
        /* we have already reached the bottom of the list */
        gui_list->selected_item = 0;
        gui_list->start_item = 0;
    }
    else
    {
        int nb_lines = gui_list->display->nb_lines;
        gui_list->selected_item++;
        item_pos = gui_list->selected_item - gui_list->start_item;
        end_item = gui_list->start_item + nb_lines;
        /* we start scrolling vertically when reaching the line
         * (nb_lines-SCROLL_LIMIT)
         * and when we are not in the last part of the list*/
        if( item_pos > nb_lines-SCROLL_LIMIT && end_item < gui_list->nb_items )
            gui_list->start_item++;
    }
}

void gui_list_select_previous(struct gui_list * gui_list)
{
    int item_pos;
    int nb_lines = gui_list->display->nb_lines;

    if( gui_list->selected_item == 0 )
    {
        if(gui_list->limit_scroll)
            return;
        gui_list->selected_item--;
        /* we have aleady reached the top of the list */
        int start;
        gui_list->selected_item = gui_list->nb_items-1;
        start = gui_list->nb_items-nb_lines;
        if( start < 0 )
            gui_list->start_item = 0;
        else
            gui_list->start_item = start;
    }
    else
    {
        gui_list->selected_item--;
        item_pos = gui_list->selected_item - gui_list->start_item;
        if( item_pos < SCROLL_LIMIT-1 && gui_list->start_item > 0 )
            gui_list->start_item--;
    }
}

void gui_list_select_next_page(struct gui_list * gui_list, int nb_lines)
{
    if(gui_list->selected_item == gui_list->nb_items-1)
    {
        if(gui_list->limit_scroll)
            return;
        gui_list->selected_item = 0;
    }
    else
    {
        gui_list->selected_item += nb_lines;
        if(gui_list->selected_item > gui_list->nb_items-1)
            gui_list->selected_item = gui_list->nb_items-1;
    }
    gui_list_put_selection_in_screen(gui_list, true);
}

void gui_list_select_previous_page(struct gui_list * gui_list, int nb_lines)
{
    if(gui_list->selected_item == 0)
    {
        if(gui_list->limit_scroll)
            return;
        gui_list->selected_item = gui_list->nb_items - 1;
    }
    else
    {
        gui_list->selected_item -= nb_lines;
        if(gui_list->selected_item < 0)
            gui_list->selected_item = 0;
    }
    gui_list_put_selection_in_screen(gui_list, false);
}

void gui_list_add_item(struct gui_list * gui_list)
{
    gui_list->nb_items++;
    /* if only one item in the list, select it */
    if(gui_list->nb_items == 1)
        gui_list->selected_item = 0;
}

void gui_list_del_item(struct gui_list * gui_list)
{
    int nb_lines = gui_list->display->nb_lines;

    if(gui_list->nb_items > 0)
    {
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

/*
 * Synchronized lists stuffs
 */
void gui_synclist_init(
    struct gui_synclist * lists,
    void (*callback_get_item_icon)
        (int selected_item, void * data, ICON * icon),
    char * (*callback_get_item_name)
        (int selected_item, void * data, char *buffer),
    void * data
    )
{
    int i;
    for(i = 0;i < NB_SCREENS;i++)
    {
        gui_list_init(&(lists->gui_list[i]),
                      callback_get_item_icon,
                      callback_get_item_name,
                      data);
        gui_list_set_display(&(lists->gui_list[i]), &(screens[i]));
    }
}

void gui_synclist_set_nb_items(struct gui_synclist * lists, int nb_items)
{
    int i;
    for(i = 0;i < NB_SCREENS;i++)
    {
        gui_list_set_nb_items(&(lists->gui_list[i]), nb_items);
    }
}

void gui_synclist_draw(struct gui_synclist * lists)
{
    int i;
    for(i = 0;i < NB_SCREENS;i++)
        gui_list_draw(&(lists->gui_list[i]));
}

void gui_synclist_select_item(struct gui_synclist * lists, int item_number)
{
    int i;
    for(i = 0;i < NB_SCREENS;i++)
        gui_list_select_item(&(lists->gui_list[i]), item_number);
}

void gui_synclist_select_next(struct gui_synclist * lists)
{
    int i;
    for(i = 0;i < NB_SCREENS;i++)
        gui_list_select_next(&(lists->gui_list[i]));
}

void gui_synclist_select_previous(struct gui_synclist * lists)
{
    int i;
    for(i = 0;i < NB_SCREENS;i++)
        gui_list_select_previous(&(lists->gui_list[i]));
}

void gui_synclist_select_next_page(struct gui_synclist * lists,
                                   enum screen_type screen)
{
    int i;
    for(i = 0;i < NB_SCREENS;i++)
        gui_list_select_next_page(&(lists->gui_list[i]),
                                  screens[screen].nb_lines);
}

void gui_synclist_select_previous_page(struct gui_synclist * lists,
                                       enum screen_type screen)
{
    int i;
    for(i = 0;i < NB_SCREENS;i++)
        gui_list_select_previous_page(&(lists->gui_list[i]),
                                      screens[screen].nb_lines);
}

void gui_synclist_add_item(struct gui_synclist * lists)
{
    int i;
    for(i = 0;i < NB_SCREENS;i++)
        gui_list_add_item(&(lists->gui_list[i]));
}

void gui_synclist_del_item(struct gui_synclist * lists)
{
    int i;
    for(i = 0;i < NB_SCREENS;i++)
        gui_list_del_item(&(lists->gui_list[i]));
}

void gui_synclist_limit_scroll(struct gui_synclist * lists, bool scroll)
{
    int i;
    for(i = 0;i < NB_SCREENS;i++)
        gui_list_limit_scroll(&(lists->gui_list[i]), scroll);
}

bool gui_synclist_do_button(struct gui_synclist * lists, unsigned button)
{
    gui_synclist_limit_scroll(lists, true);
    switch(button)
    {
        case LIST_PREV:
#ifdef LIST_RC_PREV
        case LIST_RC_PREV:
#endif
            gui_synclist_limit_scroll(lists, false);

        case LIST_PREV | BUTTON_REPEAT:
#ifdef LIST_RC_PREV
        case LIST_RC_PREV | BUTTON_REPEAT:
#endif
            gui_synclist_select_previous(lists);
            gui_synclist_draw(lists);
            return true;

        case LIST_NEXT:
#ifdef LIST_RC_NEXT
        case LIST_RC_NEXT:
#endif
            gui_synclist_limit_scroll(lists, false);

        case LIST_NEXT | BUTTON_REPEAT:
#ifdef LIST_RC_NEXT

        case LIST_RC_NEXT | BUTTON_REPEAT:
#endif
            gui_synclist_select_next(lists);
            gui_synclist_draw(lists);
            return true;
/* for pgup / pgdown, we are obliged to have a different behaviour depending on the screen
 * for which the user pressed the key since for example, remote and main screen doesn't
 * have the same number of lines*/
#ifdef LIST_PGUP
        case LIST_PGUP:
            gui_synclist_limit_scroll(lists, false);
        case LIST_PGUP | BUTTON_REPEAT:
            gui_synclist_select_previous_page(lists, SCREEN_MAIN);
            gui_synclist_draw(lists);
            return true;
#endif

#ifdef LIST_RC_PGUP
        case LIST_RC_PGUP:
            gui_synclist_limit_scroll(lists, false);
        case LIST_RC_PGUP | BUTTON_REPEAT:
            gui_synclist_select_previous_page(lists, SCREEN_REMOTE);
            gui_synclist_draw(lists);
            return true;
#endif

#ifdef LIST_PGDN
        case LIST_PGDN:
            gui_synclist_limit_scroll(lists, false);
        case LIST_PGDN | BUTTON_REPEAT:
            gui_synclist_select_next_page(lists, SCREEN_MAIN);
            gui_synclist_draw(lists);
            return true;
#endif

#ifdef LIST_RC_PGDN
        case LIST_RC_PGDN:
            gui_synclist_limit_scroll(lists, false);
        case LIST_RC_PGDN | BUTTON_REPEAT:
            gui_synclist_select_next_page(lists, SCREEN_REMOTE);
            gui_synclist_draw(lists);
            return true;
#endif
    }
    return false;
}
