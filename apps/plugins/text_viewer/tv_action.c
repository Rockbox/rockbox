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

bool bookmarks_changed = false;
bool scrolled = false;

bool tv_init_action(unsigned char **buf, size_t *size)
{
    /* initialize bookmarks and window modules */
    return tv_init_bookmark(buf, size) && tv_init_window(buf, size);
}

static void tv_finalize_action(void)
{
    /* finalize bookmark modules */
    tv_finalize_bookmark();

    /* finalize window modules */
    tv_finalize_window();
}

void tv_exit(void)
{
    /* save preference and bookmarks */
    DEBUGF("preferences_changed=%d\tbookmarks_changed=%d\tscrolled=%d\n",preferences_changed,bookmarks_changed,scrolled);
    if  ( preferences_changed || bookmarks_changed
#ifndef HAVE_DISK_STORAGE
         || scrolled
#endif
        )
    {
        DEBUGF("Saving settings\n");
        if (!tv_save_settings())
            rb->splash(HZ, "Can't save preferences and bookmarks");
    }
    else
        DEBUGF("Skip saving settings\n");

    /* finalize modules */
    tv_finalize_action();
}

bool tv_load_file(const unsigned char *file)
{
    /* load the preferences and bookmark */
    if (!tv_load_settings(file))
        return false;

    /* select to read the page */
    tv_select_bookmark();

    return true;
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

void tv_scroll_up(unsigned mode)
{
    int offset_page = 0;
    int offset_line = -1;

    if ((mode == TV_VERTICAL_SCROLL_PAGE) ||
        (mode == TV_VERTICAL_SCROLL_PREFS && preferences->vertical_scroll_mode == VS_PAGE))
    {
        offset_page--;
#ifdef HAVE_LCD_BITMAP
        offset_line = (preferences->overlap_page_mode)? 1:0;
#endif
    }
    tv_move_screen(offset_page, offset_line, SEEK_CUR);
    scrolled = true;
}

void tv_scroll_down(unsigned mode)
{
    int offset_page = 0;
    int offset_line = 1;

    if ((mode == TV_VERTICAL_SCROLL_PAGE) ||
        (mode == TV_VERTICAL_SCROLL_PREFS && preferences->vertical_scroll_mode == VS_PAGE))
    {
        offset_page++;
#ifdef HAVE_LCD_BITMAP
        offset_line = (preferences->overlap_page_mode)? -1:0;
#endif
    }
    tv_move_screen(offset_page, offset_line, SEEK_CUR);
    scrolled = true;
}

void tv_scroll_left(unsigned mode)
{
    int offset_window = 0;
    int offset_column = 0;

    if ((mode == TV_HORIZONTAL_SCROLL_COLUMN) ||
        (mode == TV_HORIZONTAL_SCROLL_PREFS && preferences->horizontal_scroll_mode == HS_COLUMN))
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
    scrolled = true;
}

void tv_scroll_right(unsigned mode)
{
    int offset_window = 0;
    int offset_column = 0;

    if ((mode == TV_HORIZONTAL_SCROLL_COLUMN) ||
        (mode == TV_HORIZONTAL_SCROLL_PREFS && preferences->horizontal_scroll_mode == HS_COLUMN))
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
    scrolled = true;
}

void tv_top(void)
{
    tv_move_screen(0, 0, SEEK_SET);
}

void tv_bottom(void)
{
    tv_move_screen(0, 0, SEEK_END);
    if (preferences->vertical_scroll_mode == VS_PAGE)
        tv_move_screen(0, -tv_get_screen_pos()->line, SEEK_CUR);
}

unsigned tv_menu(void)
{
    unsigned res;
    struct tv_screen_pos cur_pos;
    off_t cur_file_pos = tv_get_screen_pos()->file_pos;

    res = tv_display_menu();

    if (res == TV_MENU_RESULT_EXIT_MENU)
    {
        tv_convert_fpos(cur_file_pos, &cur_pos);

        tv_move_screen(cur_pos.page, cur_pos.line, SEEK_SET);
    }
    else if (res == TV_MENU_RESULT_MOVE_PAGE)
        res = TV_MENU_RESULT_EXIT_MENU;

    return res;
}

void tv_add_or_remove_bookmark(void)
{
    tv_toggle_bookmark();
    bookmarks_changed = true;
}
