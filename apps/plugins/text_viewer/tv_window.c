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
#include "tv_preferences.h"
#include "tv_screen_pos.h"
#include "tv_text_reader.h"
#include "tv_window.h"

#define TV_SCROLLBAR_WIDTH  rb->global_settings->scrollbar_width
#define TV_SCROLLBAR_HEIGHT 4

#ifndef HAVE_LCD_BITMAP
#define TV_BOOKMARK_ICON 0xe101
#endif

#ifdef HAVE_LCD_BITMAP
static int header_height;
static int footer_height;
static bool need_vertical_scrollbar = false;
#endif

static int start_width;
static int display_lines;

static int window_width;
static int window_columns;
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

static void tv_show_header(void)
{
    unsigned header_mode = header_mode;
    if (header_mode == HD_SBAR || header_mode == HD_BOTH)
        rb->gui_syncstatusbar_draw(rb->statusbars, true);

    if (header_mode == HD_PATH || header_mode == HD_BOTH)
        rb->lcd_putsxy(0, header_height - preferences->font->height, preferences->file_name);
}

static void tv_show_footer(const struct tv_screen_pos *pos)
{
    unsigned char buf[12];
    unsigned footer_mode = preferences->footer_mode;

    if (footer_mode == FT_SBAR || footer_mode == FT_BOTH)
        rb->gui_syncstatusbar_draw(rb->statusbars, true);

    if (footer_mode == FT_PAGE || footer_mode == FT_BOTH)
    {
        if (pos->line == 0)
            rb->snprintf(buf, sizeof(buf), "%d", pos->page + 1);
        else
            rb->snprintf(buf, sizeof(buf), "%d - %d", pos->page + 1, pos->page + 2);

        rb->lcd_putsxy(0, LCD_HEIGHT - footer_height, buf);
    }
}

static void tv_show_scrollbar(off_t cur_pos, int size)
{
    int items;
    int min_shown;
    int max_shown;
    int sb_width;
    int sb_height;

    sb_height = LCD_HEIGHT - header_height - footer_height;
    if (preferences->horizontal_scrollbar)
    {
        items     = preferences->windows * window_columns;
        min_shown = cur_window * window_columns + cur_column;
        max_shown = min_shown + window_columns;
        sb_width  = (need_vertical_scrollbar)? TV_SCROLLBAR_WIDTH : 0;
        sb_height -= TV_SCROLLBAR_HEIGHT;

        rb->gui_scrollbar_draw(rb->screens[SCREEN_MAIN],
                               sb_width,
                               LCD_HEIGHT - footer_height - TV_SCROLLBAR_HEIGHT,
                               LCD_WIDTH - sb_width, TV_SCROLLBAR_HEIGHT,
                               items, min_shown, max_shown, HORIZONTAL);
    }

    if (need_vertical_scrollbar)
    {
        items     = (int) tv_get_total_text_size();
        min_shown = (int) cur_pos;
        max_shown = min_shown + size;

        rb->gui_scrollbar_draw(rb->screens[SCREEN_MAIN], 0, header_height,
                               TV_SCROLLBAR_WIDTH-1, sb_height,
                               items, min_shown, max_shown, VERTICAL);
    }
}

static int tv_calc_display_lines(void)
{
    int scrollbar_height = preferences->horizontal_scrollbar ? TV_SCROLLBAR_HEIGHT : 0;
    unsigned header_mode = preferences->header_mode;
    unsigned footer_mode = preferences->footer_mode;

    header_height = (header_mode == HD_SBAR || header_mode == HD_BOTH)?
                    STATUSBAR_HEIGHT : 0;

    footer_height = (footer_mode == FT_SBAR || footer_mode == FT_BOTH)?
                    STATUSBAR_HEIGHT : 0;

    if (header_mode == HD_NONE || header_mode == HD_PATH ||
        footer_mode == FT_NONE || footer_mode == FT_PAGE)
        rb->gui_syncstatusbar_draw(rb->statusbars, false);

    if (header_mode == HD_PATH || header_mode == HD_BOTH)
        header_height += preferences->font->height;

    if (footer_mode == FT_PAGE || footer_mode == FT_BOTH)
        footer_height += preferences->font->height;

    return (LCD_HEIGHT - header_height - footer_height - scrollbar_height) / preferences->font->height;
}
#endif

static void tv_show_bookmarks(const struct tv_screen_pos *top_pos)
{
    struct tv_screen_pos bookmarks[TV_MAX_BOOKMARKS];
    int count = tv_get_bookmark_positions(bookmarks);
    int line;

#ifdef HAVE_LCD_BITMAP
    rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
#endif
    while (count--)
    {
        line = (bookmarks[count].page - top_pos->page) * display_lines
                                      + (bookmarks[count].line - top_pos->line);
        if (line >= 0 && line < display_lines)
        {
#ifdef HAVE_LCD_BITMAP
            rb->lcd_fillrect(start_width, header_height + line * preferences->font->height,
                             window_width, preferences->font->height);
#else
            rb->lcd_putc(start_width - 1, line, TV_BOOKMARK_ICON);
#endif
        }
    }
#ifdef HAVE_LCD_BITMAP
    rb->lcd_set_drawmode(DRMODE_SOLID);
#endif
}

void tv_draw_window(void)
{
    struct tv_screen_pos pos;
    const unsigned char *line_buf;
    int line;
    int offset = cur_column * col_width;
    int size = 0;
    int line_width;
    int draw_width = (preferences->windows - cur_window) * LCD_WIDTH - offset;
    int dx = start_width - offset;

    tv_copy_screen_pos(&pos);
    rb->lcd_clear_display();

    if (preferences->alignment == LEFT)
        tv_read_start(cur_window, (cur_column > 0));
    else
        tv_read_start(0, preferences->windows > 1);

    for (line = 0; line < display_lines; line++)
    {
        if (!tv_get_next_line(&line_buf))
            break;

        if (preferences->alignment == RIGHT)
        {
            rb->lcd_getstringsize(line_buf, &line_width, NULL);
            dx = draw_width - line_width;
        }

#ifdef HAVE_LCD_BITMAP
        rb->lcd_putsxy(dx, header_height + line * preferences->font->height, line_buf);
#else
        rb->lcd_puts(dx, line, line_buf);
#endif
    }

    size = tv_read_end();

    tv_show_bookmarks(&pos);

#ifdef HAVE_LCD_BITMAP
    tv_show_scrollbar(pos.file_pos, size);
    tv_show_header();
    tv_show_footer(&pos);
#endif

    rb->lcd_update();
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
#ifdef HAVE_LCD_BITMAP
    static bool font_changing = false;
    const unsigned char *font_str;
    bool change_prefs = false;
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

    /* calculates display lines */
    display_lines = tv_calc_display_lines();
#else
    (void)oldp;

    /* REAL fixed pitch :) all chars use up 1 cell */
    display_lines = 2;
#endif

#ifdef HAVE_LCD_BITMAP
    col_width = 2 * rb->font_get_width(preferences->font, ' ');
#else
    col_width = 1;
#endif

    if (cur_window >= preferences->windows)
        cur_window = 0;

    window_width = LCD_WIDTH;
#ifdef HAVE_LCD_BITMAP
    need_vertical_scrollbar = false;
    start_width = 0;
    tv_seek_top();
    tv_set_read_conditions(preferences->windows, window_width);
    if (tv_traverse_lines() && preferences->vertical_scrollbar)
    {
        need_vertical_scrollbar = true;
        start_width = TV_SCROLLBAR_WIDTH;
    }
    tv_seek_top();
#else
    start_width = 1;
#endif
    window_width -= start_width;
    window_columns = window_width / col_width;

    cur_column = 0;

    tv_set_read_conditions(preferences->windows, window_width);
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
