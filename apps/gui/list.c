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
#include "settings.h"
#include "kernel.h"

#include "screen_access.h"
#include "list.h"
#include "scrollbar.h"
#include "statusbar.h"
#include "textarea.h"

#ifdef HAVE_LCD_CHARCELLS
#define SCROLL_LIMIT 1
#else
#define SCROLL_LIMIT 2
#endif

#ifdef HAVE_LCD_BITMAP
static int offset_step = 16; /* pixels per screen scroll step */
/* should lines scroll out of the screen */
static bool offset_out_of_view = false;
#endif



void gui_list_init(struct gui_list * gui_list,
    list_get_name callback_get_item_name,
    void * data
    )
{
    gui_list->callback_get_item_icon = NULL;
    gui_list->callback_get_item_name = callback_get_item_name;
    gui_list->display = NULL;
    gui_list_set_nb_items(gui_list, 0);
    gui_list->selected_item = 0;
    gui_list->start_item = 0;
    gui_list->limit_scroll = false;
    gui_list->data=data;
    gui_list->cursor_flash_state=false;
#ifdef HAVE_LCD_BITMAP
    gui_list->offset_position = 0;
#endif
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

void gui_list_flash(struct gui_list * gui_list)
{
    struct screen * display=gui_list->display;
    gui_list->cursor_flash_state=!gui_list->cursor_flash_state;
    int selected_line=gui_list->selected_item-gui_list->start_item;
#ifdef HAVE_LCD_BITMAP
    int line_ypos=display->getymargin()+display->char_height*selected_line;
    if (global_settings.invert_cursor)
    {
        int line_xpos=display->getxmargin();
        display->set_drawmode(DRMODE_COMPLEMENT);
        display->fillrect(line_xpos, line_ypos, display->width,
                          display->char_height);
        display->set_drawmode(DRMODE_SOLID);
        display->invertscroll(0, selected_line);
    }
    else
    {
        int cursor_xpos=(global_settings.scrollbar &&
                         display->nb_lines < gui_list->nb_items)?1:0;
        screen_put_cursorxy(display, cursor_xpos, selected_line, gui_list->cursor_flash_state);
    }
    display->update_rect(0, line_ypos,display->width,
                         display->char_height);
#else
    screen_put_cursorxy(display, 0, selected_line, gui_list->cursor_flash_state);
    gui_textarea_update(display);
#endif
}

void gui_list_put_selection_in_screen(struct gui_list * gui_list,
                                      bool put_from_end)
{
    gui_textarea_update_nblines(gui_list->display);
    int nb_lines=gui_list->display->nb_lines;
    if(put_from_end)
    {
        int list_end = gui_list->selected_item + SCROLL_LIMIT;
        
        if(list_end-1 == gui_list->nb_items)
            list_end--;
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
    bool draw_icons = (gui_list->callback_get_item_icon != NULL ) ;
    bool draw_cursor;
    int i;

    /* Adjust the position of icon, cursor, text */
#ifdef HAVE_LCD_BITMAP
    display->setfont(FONT_UI);
    gui_textarea_update_nblines(display);
    bool draw_scrollbar = (global_settings.scrollbar &&
                           display->nb_lines < gui_list->nb_items);
    draw_cursor = !global_settings.invert_cursor;
    text_pos = 0; /* here it's in pixels */
    if(draw_scrollbar)
    {
        cursor_pos++;
        icon_pos++;
        text_pos += SCROLLBAR_WIDTH;
    }
    if(!draw_cursor)
        icon_pos--;
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

    gui_textarea_clear(display);
#ifdef HAVE_LCD_BITMAP
    screen_set_xmargin(display, text_pos);
#endif

    for(i = 0;i < display->nb_lines;i++)
    {
        char entry_buffer[MAX_PATH];
        unsigned char *entry_name;
        int current_item = gui_list->start_item + i;

        /* When there are less items to display than the
         * current available space on the screen, we stop*/
        if(current_item >= gui_list->nb_items)
            break;
        entry_name = gui_list->callback_get_item_name(current_item,
                                                      gui_list->data,
                                                      entry_buffer);
#ifdef HAVE_LCD_BITMAP
        /* position the string at the correct offset place */
        int item_offset;
        int item_width,h;
        display->getstringsize(entry_name, &item_width, &h);

        if (offset_out_of_view)
            item_offset = gui_list->offset_position;
        else
            /* if text is smaller then view */
            if (item_width <= display->width - text_pos)
                item_offset = 0;
            else
                /* if text got out of view  */
                if (gui_list->offset_position > 
                        item_width - (display->width - text_pos))
                    item_offset = item_width - (display->width - text_pos);
                else
                    item_offset = gui_list->offset_position;

#endif
        if(current_item == gui_list->selected_item) {
            /* The selected item must be displayed scrolling */
#ifdef HAVE_LCD_BITMAP
            if (global_settings.invert_cursor)/* Display inverted-line-style*/
                /* if text got out of view */
                if (item_offset > item_width - (display->width - text_pos))
                    /* don't scroll */
                    display->puts_style_offset(0, i, entry_name, STYLE_INVERT,item_offset);
                else
                    display->puts_scroll_style_offset(0, i, entry_name, STYLE_INVERT,item_offset);

            else  /*  if (!global_settings.invert_cursor) */
                if (item_offset > item_width - (display->width - text_pos))
                    display->puts_offset(0, i, entry_name,item_offset);
            else
                display->puts_scroll_offset(0, i, entry_name,item_offset);
#else
                display->puts_scroll(text_pos, i, entry_name);
#endif

            if(draw_cursor)
                screen_put_cursorxy(display, cursor_pos, i, true);
        }
        else
        {/* normal item */
#ifdef HAVE_LCD_BITMAP
            display->puts_offset(0, i, entry_name,item_offset);
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
            if(icon)
                screen_put_iconxy(display, icon_pos, i, icon);
        }
    }
#ifdef HAVE_LCD_BITMAP
    /* Draw the scrollbar if needed*/
    if(draw_scrollbar)
    {
        int y_start = gui_textarea_get_ystart(display);
        int scrollbar_y_end = display->char_height *
                              display->nb_lines + y_start;
        gui_scrollbar_draw(display, 0, y_start, SCROLLBAR_WIDTH-1,
                           scrollbar_y_end - y_start, gui_list->nb_items,
                           gui_list->start_item,
                           gui_list->start_item + display->nb_lines, VERTICAL);
    }
#endif
    gui_textarea_update(display);
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
    if( gui_list->selected_item == 0 )
    {
        int nb_lines = gui_list->display->nb_lines;
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
        int item_pos;
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
void gui_list_scroll_right(struct gui_list * gui_list)
{
    /* FIXME: This is a fake right boundry limiter. there should be some 
     * callback function to find the longest item on the list in pixels, 
     * to stop the list from scrolling past that point */ 
    gui_list->offset_position+=offset_step;
    if (gui_list->offset_position > 1000)
        gui_list->offset_position = 1000;
}

void gui_list_scroll_left(struct gui_list * gui_list)
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
 * Synchronized lists stuffs
 */
void gui_synclist_init(
    struct gui_synclist * lists,
    list_get_name callback_get_item_name,
    void * data
    )
{
    int i;
    FOR_NB_SCREENS(i)
    {
        gui_list_init(&(lists->gui_list[i]),
                      callback_get_item_name,
                      data);
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
void gui_synclist_set_icon_callback(struct gui_synclist * lists, list_get_icon icon_callback)
{
    int i;
    FOR_NB_SCREENS(i)
    {
        gui_list_set_icon_callback(&(lists->gui_list[i]), icon_callback);
    }
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

void gui_synclist_select_next(struct gui_synclist * lists)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_list_select_next(&(lists->gui_list[i]));
}

void gui_synclist_select_previous(struct gui_synclist * lists)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_list_select_previous(&(lists->gui_list[i]));
}

void gui_synclist_select_next_page(struct gui_synclist * lists,
                                   enum screen_type screen)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_list_select_next_page(&(lists->gui_list[i]),
                                  screens[screen].nb_lines);
}

void gui_synclist_select_previous_page(struct gui_synclist * lists,
                                       enum screen_type screen)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_list_select_previous_page(&(lists->gui_list[i]),
                                      screens[screen].nb_lines);
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

void gui_synclist_flash(struct gui_synclist * lists)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_list_flash(&(lists->gui_list[i]));
}

#ifdef HAVE_LCD_BITMAP
void gui_synclist_scroll_right(struct gui_synclist * lists)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_list_scroll_right(&(lists->gui_list[i]));
}

void gui_synclist_scroll_left(struct gui_synclist * lists)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_list_scroll_left(&(lists->gui_list[i]));
}
#endif /* HAVE_LCD_BITMAP */

unsigned gui_synclist_do_button(struct gui_synclist * lists, unsigned button)
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
            return LIST_PREV;

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
            return LIST_NEXT;

#ifdef LIST_PGRIGHT
        case LIST_PGRIGHT:
        case LIST_PGRIGHT | BUTTON_REPEAT:
#ifdef LIST_RC_PGRIGHT
        case LIST_RC_PGRIGHT:
        case LIST_RC_PGRIGHT | BUTTON_REPEAT:
#endif
        gui_synclist_scroll_right(lists);
        gui_synclist_draw(lists);
        return true;
#endif

#ifdef LIST_PGLEFT
        case LIST_PGLEFT:
        case LIST_PGLEFT | BUTTON_REPEAT:
#ifdef LIST_RC_PGLEFT
        case LIST_RC_PGLEFT:
        case LIST_RC_PGLEFT | BUTTON_REPEAT:
#endif
        gui_synclist_scroll_left(lists);
        gui_synclist_draw(lists);
        return true;
#endif

/* for pgup / pgdown, we are obliged to have a different behaviour depending on the screen
 * for which the user pressed the key since for example, remote and main screen doesn't
 * have the same number of lines*/
#ifdef LIST_PGUP
        case LIST_PGUP:
            gui_synclist_limit_scroll(lists, false);
        case LIST_PGUP | BUTTON_REPEAT:
            gui_synclist_select_previous_page(lists, SCREEN_MAIN);
            gui_synclist_draw(lists);
            return LIST_NEXT;
#endif

#ifdef LIST_RC_PGUP
        case LIST_RC_PGUP:
            gui_synclist_limit_scroll(lists, false);
        case LIST_RC_PGUP | BUTTON_REPEAT:
            gui_synclist_select_previous_page(lists, SCREEN_REMOTE);
            gui_synclist_draw(lists);
            return LIST_NEXT;
#endif

#ifdef LIST_PGDN
        case LIST_PGDN:
            gui_synclist_limit_scroll(lists, false);
        case LIST_PGDN | BUTTON_REPEAT:
            gui_synclist_select_next_page(lists, SCREEN_MAIN);
            gui_synclist_draw(lists);
            return LIST_PREV;
#endif

#ifdef LIST_RC_PGDN
        case LIST_RC_PGDN:
            gui_synclist_limit_scroll(lists, false);
        case LIST_RC_PGDN | BUTTON_REPEAT:
            gui_synclist_select_next_page(lists, SCREEN_REMOTE);
            gui_synclist_draw(lists);
            return LIST_PREV;
#endif
    }
    return 0;
}
