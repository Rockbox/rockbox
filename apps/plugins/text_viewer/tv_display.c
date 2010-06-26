/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Yoshihisa Uchida
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
#include "tv_display.h"
#include "tv_preferences.h"

/*
 * display layout
 *
 * when is defined HAVE_LCD_BITMAP
 * +-------------------------+
 * |statusbar (1)            |
 * +-------------------------+
 * |filename  (2)            |
 * +---+---------------------+
 * |s  |                     |
 * |c  |                     |
 * |r  | draw area           |
 * |o  |                     |
 * |l  |                     |
 * |l  |                     |
 * |b  |                     |
 * |a  |                     |
 * |r  |                     |
 * |(3)|                     |
 * +---+---------------------+
 * |   |scrollbar (4)        |
 * +---+---------------------+
 * |page (5)                 |
 * +-------------------------+
 * |statusbar (6)            |
 * +-------------------------+
 *
 * (1) displays when rb->global_settings->statusbar == STATUSBAR_TOP
 *              and  preferences->header_mode is HD_SBAR or HD_BOTH.
 * (2) displays when preferences->header_mode is HD_PATH or HD_BOTH.
 * (3) displays when preferences->vertical_scrollbar is SB_ON.
 * (4) displays when preferences->horizontal_scrollbar is SB_ON.
 * (5) displays when preferences->footer_mode is FT_PAGE or FT_BOTH.
 * (6) displays when rb->global_settings->statusbar == STATUSBAR_BOTTOM
 *              and  preferences->footer_mode is FT_SBAR or FT_BOTH.
 *
 *
 * when isn't defined HAVE_LCD_BITMAP
 * +---+---------------------+
 * |   |                     |
 * |(7)| draw area           |
 * |   |                     |
 * +---+---------------------+
 * (7) bookmark
 */

#define TV_SCROLLBAR_WIDTH  rb->global_settings->scrollbar_width
#define TV_SCROLLBAR_HEIGHT 4

#ifndef HAVE_LCD_BITMAP
#define TV_BOOKMARK_ICON 0xe101
#endif

struct tv_rect {
    int x;
    int y;
    int w;
    int h;
};

static struct viewport vp_info;
static bool is_initialized_vp = false;

#ifdef HAVE_LCD_BITMAP
static int drawmode = DRMODE_SOLID;
static int totalsize;
static bool show_vertical_scrollbar;
#endif

static struct screen* display;

/* layout */
#ifdef HAVE_LCD_BITMAP
static struct tv_rect header;
static struct tv_rect footer;
static struct tv_rect horizontal_scrollbar;
static struct tv_rect vertical_scrollbar;
#else
static struct tv_rect bookmark;
#endif
static struct tv_rect drawarea;

static int display_columns;
static int display_rows;

static int col_width;
static int row_height;

#ifdef HAVE_LCD_BITMAP

void tv_show_header(void)
{
    unsigned header_mode = header_mode;

    if (preferences->header_mode == HD_PATH || preferences->header_mode == HD_BOTH)
        display->putsxy(header.x, header.y, preferences->file_name);
}

void tv_show_footer(const struct tv_screen_pos *pos)
{
    unsigned char buf[12];
    unsigned footer_mode = preferences->footer_mode;

    if (footer_mode == FT_PAGE || footer_mode == FT_BOTH)
    {
        if (pos->line == 0)
            rb->snprintf(buf, sizeof(buf), "%d", pos->page + 1);
        else
            rb->snprintf(buf, sizeof(buf), "%d - %d", pos->page + 1, pos->page + 2);
        display->putsxy(footer.x, footer.y, buf);
    }
}

void tv_init_scrollbar(off_t total, bool show_scrollbar)
{
    totalsize = total;
    show_vertical_scrollbar = show_scrollbar;
}

void tv_show_scrollbar(int window, int col, off_t cur_pos, int size)
{
    int items;
    int min_shown;
    int max_shown;

    if (preferences->horizontal_scrollbar)
    {
        items     = preferences->windows * display_columns;
        min_shown = window * display_columns + col;
        max_shown = min_shown + display_columns;

        rb->gui_scrollbar_draw(display,
                               horizontal_scrollbar.x, horizontal_scrollbar.y,
                               horizontal_scrollbar.w, TV_SCROLLBAR_HEIGHT,
                               items, min_shown, max_shown, HORIZONTAL);
    }

    if (show_vertical_scrollbar)
    {
        items     = (int) totalsize;
        min_shown = (int) cur_pos;
        max_shown = min_shown + size;

        rb->gui_scrollbar_draw(display,
                               vertical_scrollbar.x, vertical_scrollbar.y,
                               TV_SCROLLBAR_WIDTH-1, vertical_scrollbar.h,
                               items, min_shown, max_shown, VERTICAL);
    }
}

void tv_fillrect(int col, int row, int rows)
{
    display->fillrect(drawarea.x + col * col_width, drawarea.y + row * row_height,
                      drawarea.w - col * col_width, rows * row_height);
}

void tv_set_drawmode(int mode)
{
    rb->lcd_set_drawmode(mode);
}

#endif

void tv_draw_text(int row, const unsigned char *text, int offset)
{
    int xpos = -offset * col_width;
    int text_width;

    if (row < 0 || row >= display_rows)
        return;

    if (preferences->alignment == AL_RIGHT)
    {
        display->getstringsize(text, &text_width, NULL);
        xpos += ((offset > 0)? drawarea.w * 2 : drawarea.w) - text_width;
    }

#ifdef HAVE_LCD_BITMAP
    display->putsxy(drawarea.x + xpos, drawarea.y + row * row_height, text);
#else
    display->puts(drawarea.x + xpos, drawarea.y + row, text);
#endif
}

#ifndef HAVE_LCD_BITMAP
void tv_put_bookmark_icon(int row)
{
    display->putchar(bookmark.x, drawarea.y + row, TV_BOOKMARK_ICON);
}
#endif

void tv_init_display(void)
{
    display = rb->screens[SCREEN_MAIN];
    display->clear_viewport();
}

void tv_start_display(void)
{
    display->set_viewport(&vp_info);
#ifdef HAVE_LCD_BITMAP
    drawmode = rb->lcd_get_drawmode();
#endif
}

void tv_end_display(void)
{
#ifdef HAVE_LCD_BITMAP
    rb->lcd_set_drawmode(drawmode);
#endif
    display->set_viewport(NULL);
}

void tv_clear_display(void)
{
    display->clear_viewport();
}

void tv_update_display(void)
{
    display->update_viewport();
}

#ifdef HAVE_LCD_BITMAP
void tv_set_layout(int col_w, bool show_scrollbar)
#else
void tv_set_layout(int col_w)
#endif
{
#ifdef HAVE_LCD_BITMAP
    int scrollbar_width  = (show_scrollbar)?                    TV_SCROLLBAR_WIDTH  : 0;
    int scrollbar_height = (preferences->horizontal_scrollbar)? TV_SCROLLBAR_HEIGHT : 0;
    unsigned header_mode = preferences->header_mode;
    unsigned footer_mode = preferences->footer_mode;

    row_height = preferences->font->height;

    header.x = 0;
    header.y = 0;
    header.w = vp_info.width;
    header.h = (header_mode == HD_PATH || header_mode == HD_BOTH)? row_height : 0;

    footer.x = 0;
    footer.w = vp_info.width;
    footer.h = (footer_mode == FT_PAGE || footer_mode == FT_BOTH)? row_height : 0;
    footer.y = vp_info.height - footer.h;

    drawarea.x = scrollbar_width;
    drawarea.y = header.y + header.h;
    drawarea.w = vp_info.width - scrollbar_width;
    drawarea.h = footer.y - drawarea.y - scrollbar_height;

    horizontal_scrollbar.x = drawarea.x;
    horizontal_scrollbar.y = footer.y - scrollbar_height;
    horizontal_scrollbar.w = drawarea.w;
    horizontal_scrollbar.h = scrollbar_height;

    vertical_scrollbar.x = 0;
    vertical_scrollbar.y = drawarea.y;
    vertical_scrollbar.w = scrollbar_width;
    vertical_scrollbar.h = drawarea.h;
#else
    row_height = 1;

    bookmark.x = 0;
    bookmark.y = 0;
    bookmark.w = 1;
    bookmark.h = vp_info.height;

    drawarea.x = 1;
    drawarea.y = 0;
    drawarea.w = vp_info.width - 1;
    drawarea.h = vp_info.height;
#endif
    col_width = col_w;

    display_columns = drawarea.w / col_width;
    display_rows    = drawarea.h / row_height;
}

void tv_get_drawarea_info(int *width, int *cols, int *rows)
{
    *width = drawarea.w;
    *cols  = display_columns;
    *rows  = display_rows;
}

void tv_change_viewport(void)
{
#ifdef HAVE_LCD_BITMAP
    struct viewport vp;
    bool show_statusbar = (preferences->header_mode == HD_SBAR ||
                           preferences->header_mode == HD_BOTH ||
                           preferences->footer_mode == FT_SBAR ||
                           preferences->footer_mode == FT_BOTH);

    if (is_initialized_vp)
        tv_undo_viewport();
    else
        is_initialized_vp = true;

    rb->viewportmanager_theme_enable(SCREEN_MAIN, show_statusbar, &vp);
    vp_info = vp;
    vp_info.flags &= ~VP_FLAG_ALIGNMENT_MASK;

#else
    if (!is_initialized_vp)
    {
        rb->viewport_set_defaults(&vp_info, SCREEN_MAIN);
        is_initialized_vp = true;
    }
#endif
}

void tv_undo_viewport(void)
{
#ifdef HAVE_LCD_BITMAP
    if (is_initialized_vp)
        rb->viewportmanager_theme_undo(SCREEN_MAIN, false);
#endif
}
