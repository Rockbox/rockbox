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
#include "tv_action.h"
#include "tv_bookmark.h"
#include "tv_pager.h"
#include "tv_screen_pos.h"
#include "tv_settings.h"
#include "tv_window.h"

static const struct tv_preferences *prefs;

bool tv_init(const unsigned char *file)
{
    size_t req_size = 0;
    size_t size;
    size_t used_size;
    unsigned char *buffer;

    /* get the plugin buffer */
    buffer = rb->plugin_get_buffer(&req_size);
    size = req_size;
    if (buffer == NULL || size == 0)
        return false;

    prefs = tv_get_preferences();

    tv_init_bookmark();

    /* initialize modules */
    if (!tv_init_window(buffer, size, &used_size))
        return false;

    /* load the preferences and bookmark */
    tv_load_settings(file);

    /* select to read the page */
    tv_select_bookmark();
    return true;
}

void tv_exit(void *parameter)
{
    (void)parameter;

    /* save preference and bookmarks */
    if (!tv_save_settings())
        rb->splash(HZ, "Can't save preferences and bookmarks");

    /* finalize modules */
    tv_finalize_window();
}

void tv_draw(void)
{
    struct tv_screen_pos pos;

    tv_copy_screen_pos(&pos);
    tv_draw_window();
    if (pos.line == 0)
        tv_new_page();

    tv_move_screen(pos.page, pos.line, SEEK_SET);
}

void tv_scroll_up(enum tv_vertical_scroll_mode mode)
{
    int offset_page = 0;
    int offset_line = -1;

    if ((mode == TV_VERTICAL_SCROLL_PAGE) ||
        (mode == TV_VERTICAL_SCROLL_PREFS && prefs->vertical_scroll_mode == PAGE))
    {
        offset_page--;
#ifdef HAVE_LCD_BITMAP
        offset_line = (prefs->page_mode == OVERLAP)? 1:0;
#endif
    }
    tv_move_screen(offset_page, offset_line, SEEK_CUR);
}

void tv_scroll_down(enum tv_vertical_scroll_mode mode)
{
    int offset_page = 0;
    int offset_line = 1;

    if ((mode == TV_VERTICAL_SCROLL_PAGE) ||
        (mode == TV_VERTICAL_SCROLL_PREFS && prefs->vertical_scroll_mode == PAGE))
    {
        offset_page++;
#ifdef HAVE_LCD_BITMAP
        offset_line = (prefs->page_mode == OVERLAP)? -1:0;
#endif
    }
    tv_move_screen(offset_page, offset_line, SEEK_CUR);
}

void tv_scroll_left(enum tv_horizontal_scroll_mode mode)
{
    int offset_window = 0;
    int offset_column = 0;

    if ((mode == TV_HORIZONTAL_SCROLL_COLUMN) ||
        (mode == TV_HORIZONTAL_SCROLL_PREFS && prefs->horizontal_scroll_mode == COLUMN))
    {
        /* Scroll left one column */
        offset_column--;
    }
    else
    {
        /* Scroll left one window */
        offset_window--;
    }
    tv_move_window(offset_window, offset_column);
}

void tv_scroll_right(enum tv_horizontal_scroll_mode mode)
{
    int offset_window = 0;
    int offset_column = 0;

    if ((mode == TV_HORIZONTAL_SCROLL_COLUMN) ||
        (mode == TV_HORIZONTAL_SCROLL_PREFS && prefs->horizontal_scroll_mode == COLUMN))
    {
        /* Scroll right one column */
        offset_column++;
    }
    else
    {
        /* Scroll right one window */
        offset_window++;
    }
    tv_move_window(offset_window, offset_column);
}

void tv_top(void)
{
    tv_move_screen(0, 0, SEEK_SET);
}

void tv_bottom(void)
{
    tv_move_screen(0, 0, SEEK_END);
    if (prefs->vertical_scroll_mode == PAGE)
        tv_move_screen(0, -tv_get_screen_pos()->line, SEEK_CUR);
}

enum tv_menu_result tv_menu(void)
{
    enum tv_menu_result res;
    struct tv_screen_pos cur_pos;
    off_t cur_file_pos = tv_get_screen_pos()->file_pos;

    res = tv_display_menu();

    tv_convert_fpos(cur_file_pos, &cur_pos);
    if (prefs->vertical_scroll_mode == PAGE)
        cur_pos.line = 0;

    tv_move_screen(cur_pos.page, cur_pos.line, SEEK_SET);

    return res;
}

void tv_add_or_remove_bookmark(void)
{
    tv_toggle_bookmark();
}
