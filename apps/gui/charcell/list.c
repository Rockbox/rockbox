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

#include "list.h"
#include "screen_access.h"
#include "scrollbar.h"
#include "statusbar.h"
#include "lang.h"
#include "sound.h"
#include "misc.h"
#include "talk.h"

void list_draw(struct screen *display, struct viewport *parent,
               struct gui_synclist *gui_list)
{
    (void)parent;
    int text_pos;
    bool draw_icons = (gui_list->callback_get_item_icon != NULL &&
                       global_settings.show_icons);
    bool draw_cursor;
    int i;
    int lines;
    int start, end;
    
    display->set_viewport(NULL);
    lines = display->nb_lines;
    
    display->clear_display();
    start = 0;
    end = display->nb_lines;
    gui_list->last_displayed_start_item[display->screen_type] = 
                                gui_list->start_item[display->screen_type];

    gui_list->last_displayed_selected_item = gui_list->selected_item;

    /* Adjust the position of icon, cursor, text for the list */
    draw_cursor = true;
    if(draw_icons)
        text_pos = 2; /* here it's in chars */
    else
        text_pos = 1;

    for (i = start; i < end; i++)
    {
        unsigned char *s;
        char entry_buffer[MAX_PATH];
        unsigned char *entry_name;
        int current_item = gui_list->start_item[display->screen_type] + i;

        /* When there are less items to display than the
         * current available space on the screen, we stop*/
        if(current_item >= gui_list->nb_items)
            break;
        s = gui_list->callback_get_item_name(current_item,
                                             gui_list->data,
                                             entry_buffer,
                                             sizeof(entry_buffer));
        entry_name = P2STR(s);


        if(gui_list->show_selection_marker &&
           current_item >= gui_list->selected_item &&
           current_item <  gui_list->selected_item + gui_list->selected_size)
        {/* The selected item must be displayed scrolling */
            display->puts_scroll(text_pos, i, entry_name);

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
                display->puts_scroll(text_pos, i, entry_name);
            }
            else
            {
                display->puts(text_pos, i, entry_name);
            }
        }
        /* Icons display */
        if(draw_icons)
        {
            enum themable_icons icon;
            icon = gui_list->callback_get_item_icon(current_item,
                                                    gui_list->data);
            if(icon > Icon_NOICON)
            {
                screen_put_icon(display, 1, i, icon);
            }
        }
    }

    display->update_viewport();
    display->update();
}
