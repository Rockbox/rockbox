/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Gilles Roux
 *               2003 Garrett Derner
 *               2010 Yoshihisa Uchida
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
#include "plugin.h"
#include "tv_bookmark.h"
#include "tv_display.h"
#include "tv_preferences.h"
#include "tv_screen_pos.h"
#include "tv_text_reader.h"
#include "tv_window.h"

static int window_width;
static int window_columns;
static int display_lines;

static int cur_window;
static int cur_column;

static void tv_draw_bookmarks(const struct tv_screen_pos *top_pos)
{
    struct tv_screen_pos bookmarks[TV_MAX_BOOKMARKS];
    int disp_bookmarks[TV_MAX_BOOKMARKS];
    int count = tv_get_bookmark_positions(bookmarks);
    int disp_count = 0;
    int line;

    while (count--)
    {
        line = (bookmarks[count].page - top_pos->page) * display_lines
                                      + (bookmarks[count].line - top_pos->line);
        if (line >= 0 && line < display_lines)
            disp_bookmarks[disp_count++] = line;
    }

    tv_show_bookmarks(disp_bookmarks, disp_count);
}

void tv_draw_window(void)
{
    struct tv_screen_pos pos;
    const unsigned char *text_buf;
    int line;
    int size = 0;

    tv_copy_screen_pos(&pos);

    tv_start_display();

    tv_read_start(cur_window, (cur_column > 0));

    for (line = 0; line < display_lines; line++)
    {
        if (!tv_get_next_line(&text_buf))
            break;

        tv_draw_text(line, text_buf, cur_column);
    }

    size = tv_read_end();

    tv_draw_bookmarks(&pos);

    tv_update_extra(cur_window, cur_column, &pos, size);

    tv_end_display();
}

bool tv_traverse_lines(void)
{
    int i;
    bool res = true;

    tv_read_start(0, false);
    for (i = 0; i < display_lines; i++)
    {
        if (!tv_get_next_line(NULL))
        {
            res = false;
            break;
        }
    }
    tv_read_end();
    return res;
}

static int tv_change_preferences(const struct tv_preferences *oldp)
{
    bool need_scrollbar = false;

    (void)oldp;

    tv_set_layout(need_scrollbar);
    tv_get_drawarea_info(&window_width, &window_columns, &display_lines);

    if (tv_exist_scrollbar())
    {
        tv_seek_top();
        tv_set_read_conditions(preferences->windows, window_width);
        if (tv_traverse_lines() &&
            (preferences->vertical_scrollbar || preferences->horizontal_scrollbar))
        {
            need_scrollbar = true;
            tv_set_layout(need_scrollbar);
            tv_get_drawarea_info(&window_width, &window_columns, &display_lines);
        }
        tv_seek_top();
        tv_init_scrollbar(tv_get_total_text_size(), need_scrollbar);
    }

    if (cur_window >= preferences->windows)
        cur_window = 0;

    cur_column = 0;

    tv_set_read_conditions(preferences->windows, window_width);
    return TV_CALLBACK_OK;
}

bool tv_init_window(unsigned char **buf, size_t *size)
{
    tv_add_preferences_change_listner(tv_change_preferences);
    return tv_init_display(buf, size) && tv_init_text_reader(buf, size);
}

void tv_finalize_window(void)
{
    tv_finalize_text_reader();
    tv_finalize_display();
}

void tv_move_window(int window_delta, int column_delta)
{
    cur_window += window_delta;
    cur_column += column_delta;

    if (cur_window < 0)
    {
        cur_window = 0;
        cur_column = 0;
    }
    else if (cur_window >= preferences->windows)
    {
        cur_window = preferences->windows - 1;
        cur_column = 0;
    }

    if (cur_column < 0)
    {
        if (cur_window == 0)
            cur_column = 0;
        else
        {
            cur_window--;
            cur_column = window_columns - 1;
        }
    }
    else
    {
        if (cur_window == preferences->windows - 1)
            cur_column = 0;
        else if (cur_column >= window_columns)
        {
            cur_window++;
            cur_column = 0;
        }
    }
}
