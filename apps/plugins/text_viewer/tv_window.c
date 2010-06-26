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
static int col_width;

static int cur_window;
static int cur_column;

#ifdef HAVE_LCD_BITMAP
static bool tv_set_font(const unsigned char *font)
{
    unsigned char path[MAX_PATH];

    if (font != NULL && *font != '\0')
    {
        rb->snprintf(path, MAX_PATH, "%s/%s.fnt", FONT_DIR, font);
        if (rb->font_load(NULL, path) < 0)
        {
            rb->splash(HZ/2, "font load failed");
            return false;
        }
    }
    return true;
}

static bool tv_check_header_and_footer(struct tv_preferences *new_prefs)
{
    bool change_prefs = false;

    if (rb->global_settings->statusbar != STATUSBAR_TOP)
    {
        if (new_prefs->header_mode == HD_SBAR)
        {
            new_prefs->header_mode = HD_NONE;
            change_prefs = true;
        }
        else if (new_prefs->header_mode == HD_BOTH)
        {
            new_prefs->header_mode = HD_PATH;
            change_prefs = true;
        }
    }
    if (rb->global_settings->statusbar != STATUSBAR_BOTTOM)
    {
        if (new_prefs->footer_mode == FT_SBAR)
        {
            new_prefs->footer_mode = FT_NONE;
            change_prefs = true;
        }
        else if (new_prefs->footer_mode == FT_BOTH)
        {
            new_prefs->footer_mode = FT_PAGE;
            change_prefs = true;
        }
    }

    return change_prefs;
}
#endif

static void tv_show_bookmarks(const struct tv_screen_pos *top_pos)
{
    struct tv_screen_pos bookmarks[TV_MAX_BOOKMARKS];
    int count = tv_get_bookmark_positions(bookmarks);
    int line;

#ifdef HAVE_LCD_BITMAP
    tv_set_drawmode(DRMODE_COMPLEMENT);
#endif

    while (count--)
    {
        line = (bookmarks[count].page - top_pos->page) * display_lines
                                      + (bookmarks[count].line - top_pos->line);
        if (line >= 0 && line < display_lines)
        {
#ifdef HAVE_LCD_BITMAP
            tv_fillrect(0, line, 1);
#else
            tv_put_bookmark_icon(line);
#endif
        }
    }
}

void tv_draw_window(void)
{
    struct tv_screen_pos pos;
    const unsigned char *text_buf;
    int line;
    int size = 0;

    tv_copy_screen_pos(&pos);

    tv_start_display();

#ifdef HAVE_LCD_BITMAP
    tv_set_drawmode(DRMODE_SOLID);
#endif
    tv_clear_display();

    tv_read_start(cur_window, (cur_column > 0));

    for (line = 0; line < display_lines; line++)
    {
        if (!tv_get_next_line(&text_buf))
            break;

        tv_draw_text(line, text_buf, cur_column);
    }

    size = tv_read_end();

#ifdef HAVE_LCD_BITMAP
    tv_show_scrollbar(cur_window, cur_column, pos.file_pos, size);
    tv_show_header();
    tv_show_footer(&pos);
#endif
    tv_show_bookmarks(&pos);

    tv_update_display();
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

static void tv_change_preferences(const struct tv_preferences *oldp)
{
#ifndef HAVE_LCD_BITMAP
    (void)oldp;
#else
    static bool font_changing = false;
    const unsigned char *font_str;
    bool change_prefs = false;
    bool need_vertical_scrollbar;
    struct tv_preferences new_prefs;
    tv_copy_preferences(&new_prefs);

    font_str = (oldp && !font_changing)? oldp->font_name : rb->global_settings->font_file;

    /* change font */
    if (font_changing || rb->strcmp(font_str, preferences->font_name))
    {
        font_changing = true;
        if (!tv_set_font(preferences->font_name))
        {
            rb->strlcpy(new_prefs.font_name, font_str, MAX_PATH);
            change_prefs = true;
        }
    }

    if (tv_check_header_and_footer(&new_prefs) || change_prefs)
    {
        tv_set_preferences(&new_prefs);
        return;
    }

    font_changing = false;
#endif

#ifdef HAVE_LCD_BITMAP
    col_width = 2 * rb->font_get_width(preferences->font, ' ');
#else
    col_width = 1;
#endif

    if (cur_window >= preferences->windows)
        cur_window = 0;

    /* change viewport */
    tv_change_viewport();

#ifdef HAVE_LCD_BITMAP
    need_vertical_scrollbar = false;
    tv_set_layout(col_width, need_vertical_scrollbar);
    tv_get_drawarea_info(&window_width, &window_columns, &display_lines);
    tv_seek_top();
    tv_set_read_conditions(preferences->windows, window_width);
    if (tv_traverse_lines() && preferences->vertical_scrollbar)
    {
        need_vertical_scrollbar = true;
        tv_set_layout(col_width, need_vertical_scrollbar);
        tv_get_drawarea_info(&window_width, &window_columns, &display_lines);
    }
    tv_seek_top();
#else
    tv_set_layout(col_width);
    tv_get_drawarea_info(&window_width, &window_columns, &display_lines);
#endif

    window_columns = window_width / col_width;
    cur_column = 0;

    tv_set_read_conditions(preferences->windows, window_width);

    tv_init_display();
#ifdef HAVE_LCD_BITMAP
    tv_init_scrollbar(tv_get_total_text_size(), need_vertical_scrollbar);
#endif
}

bool tv_init_window(unsigned char **buf, size_t *size)
{
    tv_add_preferences_change_listner(tv_change_preferences);
    return tv_init_text_reader(buf, size);
}

void tv_finalize_window(void)
{
    tv_finalize_text_reader();

#ifdef HAVE_LCD_BITMAP
    /* restore font */
    if (rb->strcmp(rb->global_settings->font_file, preferences->font_name))
    {
        tv_set_font(rb->global_settings->font_file);
    }

    /* undo viewport */
    tv_undo_viewport();
#endif
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
