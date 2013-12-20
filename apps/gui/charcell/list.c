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

#include "list.h"
#include "screen_access.h"
#include "scrollbar.h"
#include "lang.h"
#include "sound.h"
#include "misc.h"

void gui_synclist_scroll_stop(struct gui_synclist *lists)
{
    (void)lists;
    FOR_NB_SCREENS(i)
    {
        screens[i].scroll_stop();
    }
}

void list_draw(struct screen *display, struct gui_synclist *gui_list)
{
    bool draw_icons = (gui_list->callback_get_item_icon != NULL);
    bool selected;
    int i;
    int start, end;

    display->set_viewport(NULL);

    display->clear_display();
    start = 0;
    end = display->getnblines();

    struct line_desc desc = {
        .height = -1,
        .text_color = 1,
        .line_color = 1,
        .line_end_color = 1,
        .style = STYLE_DEFAULT
    };

    for (i = start; i < end; i++)
    {
        unsigned const char *s;
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

        if (gui_list->show_selection_marker &&
                current_item >= gui_list->selected_item &&
                current_item <  gui_list->selected_item + gui_list->selected_size)
            selected = true; /* The selected item must be displayed scrolling */
        else
            selected = false;

        desc.nlines = gui_list->selected_size,
        desc.line = gui_list->selected_size > 1 ? i : 0,
        desc.scroll = selected ? true : gui_list->scroll_all;

        if (draw_icons)
            put_line(display, 0, i, &desc, "$i$i$t",
                selected ? Icon_Cursor : Icon_NOICON,
                gui_list->callback_get_item_icon(current_item, gui_list->data),
                entry_name);
        else
            put_line(display, 0, i, &desc, "$i$t",
                selected ? Icon_Cursor : Icon_NOICON, 
                entry_name);
    }

    display->update_viewport();
    display->update();
}
