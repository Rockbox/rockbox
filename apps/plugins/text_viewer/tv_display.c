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
#include "plugin.h"
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
 * (1) displays when rb->global_settings->statusbar == STATUSBAR_TOP.
 * (2) displays when preferences->header_mode is HD_PATH.
 * (3) displays when preferences->vertical_scrollbar is SB_ON.
 * (4) displays when preferences->horizontal_scrollbar is SB_ON.
 * (5) displays when preferences->footer_mode is FT_PAGE.
 * (6) displays when rb->global_settings->statusbar == STATUSBAR_BOTTOM.
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

static struct screen* display;

#ifdef HAVE_LCD_BITMAP
static int drawmode = DRMODE_SOLID;
#endif

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

static bool show_horizontal_scrollbar;
static bool show_vertical_scrollbar;

static int display_columns;
static int display_rows;

static int col_width  = 1;
static int row_height = 1;

static int totalsize;

#ifdef HAVE_LCD_BITMAP

static void tv_show_header(void)
{
    if (preferences->header_mode)
        display->putsxy(header.x, header.y, preferences->file_name);
}

static void tv_show_footer(const struct tv_screen_pos *pos)
{
    unsigned char buf[12];

    if (preferences->footer_mode)
    {
        if (pos->line == 0)
            rb->snprintf(buf, sizeof(buf), "%d", pos->page + 1);
        else
            rb->snprintf(buf, sizeof(buf), "%d - %d", pos->page + 1, pos->page + 2);
        display->putsxy(footer.x, footer.y + 1, buf);
    }
}

static void tv_show_scrollbar(int window, int col, off_t cur_pos, int size)
{
    int items;
    int min_shown;
    int max_shown;

    if (show_horizontal_scrollbar)
    {
        items     = preferences->windows * display_columns;
        min_shown = window * display_columns + col;
        max_shown = min_shown + display_columns;

        rb->gui_scrollbar_draw(display,
                               horizontal_scrollbar.x, horizontal_scrollbar.y + 1,
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
                               TV_SCROLLBAR_WIDTH, vertical_scrollbar.h,
                               items, min_shown, max_shown, VERTICAL);
    }
}

#endif

void tv_init_scrollbar(off_t total, bool show_scrollbar)
{
    totalsize = total;
    show_horizontal_scrollbar = (show_scrollbar && preferences->horizontal_scrollbar);
    show_vertical_scrollbar   = (show_scrollbar && preferences->vertical_scrollbar);
}

void tv_show_bookmarks(const int *rows, int count)
{
#ifdef HAVE_LCD_BITMAP
    rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
#endif

    while (count--)
    {
#ifdef HAVE_LCD_BITMAP
        display->fillrect(drawarea.x, drawarea.y + rows[count] * row_height,
                          drawarea.w, row_height);
#else
        display->putchar(bookmark.x, drawarea.y + rows[count], TV_BOOKMARK_ICON);
#endif
    }
}

void tv_update_extra(int window, int col, const struct tv_screen_pos *pos, int size)
{
#ifdef HAVE_LCD_BITMAP
    tv_show_scrollbar(window, col, pos->file_pos, size);
    tv_show_header();
    tv_show_footer(pos);
#else
    (void)window;
    (void)col;
    (void)pos;
    (void)size;
#endif
}

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

void tv_start_display(void)
{
    display->set_viewport(&vp_info);
#ifdef HAVE_LCD_BITMAP
    drawmode = rb->lcd_get_drawmode();
    rb->lcd_set_drawmode(DRMODE_SOLID);
#endif

#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif
    display->clear_viewport();
}

void tv_end_display(void)
{
    display->update_viewport();

#ifdef HAVE_LCD_BITMAP
    rb->lcd_set_drawmode(drawmode);
#endif

    display->set_viewport(NULL);
}

void tv_set_layout(bool show_scrollbar)
{
#ifdef HAVE_LCD_BITMAP
    int scrollbar_width  = (show_scrollbar && preferences->vertical_scrollbar)?
                           TV_SCROLLBAR_WIDTH  + 1 : 0;
    int scrollbar_height = (show_scrollbar && preferences->horizontal_scrollbar)?
                           TV_SCROLLBAR_HEIGHT + 1 : 0;

    row_height = preferences->font->height;

    header.x = 0;
    header.y = 1;
    header.w = vp_info.width;
    header.h = (preferences->header_mode)? row_height + 1 : 0;

    footer.x = 0;
    footer.w = vp_info.width;
    footer.h = (preferences->footer_mode)? row_height + 1 : 0;
    footer.y = vp_info.height - 1 - footer.h;

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
    (void) show_scrollbar;

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

    display_columns = drawarea.w / col_width;
    display_rows    = drawarea.h / row_height;
}

void tv_get_drawarea_info(int *width, int *cols, int *rows)
{
    *width = drawarea.w;
    *cols  = display_columns;
    *rows  = display_rows;
}

static void tv_change_viewport(void)
{
#ifdef HAVE_LCD_BITMAP
    bool show_statusbar = (rb->global_settings->statusbar != STATUSBAR_OFF &&
                           preferences->statusbar);

    if (is_initialized_vp)
        rb->viewportmanager_theme_undo(SCREEN_MAIN, false);
    else
        is_initialized_vp = true;

    if (show_statusbar)
        rb->memcpy(&vp_info, rb->sb_skin_get_info_vp(SCREEN_MAIN), sizeof(struct viewport));
    else
        rb->viewport_set_defaults(&vp_info, SCREEN_MAIN);

    rb->viewportmanager_theme_enable(SCREEN_MAIN, show_statusbar, &vp_info);
    vp_info.flags &= ~VP_FLAG_ALIGNMENT_MASK;
    display->set_viewport(&vp_info);
#else
    if (!is_initialized_vp)
    {
        rb->viewport_set_defaults(&vp_info, SCREEN_MAIN);
        is_initialized_vp = true;
    }
#endif
}

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
#endif

static int tv_change_preferences(const struct tv_preferences *oldp)
{
#ifdef HAVE_LCD_BITMAP
    static bool font_changing = false;
    const unsigned char *font_str;
    struct tv_preferences new_prefs;

    font_str = (oldp && !font_changing)? oldp->font_name : rb->global_settings->font_file;

    /* change font */
    if (font_changing || rb->strcmp(font_str, preferences->font_name))
    {
        if (!tv_set_font(preferences->font_name))
        {
            /*
             * tv_set_font(rb->global_settings->font_file) doesn't fail usually.
             * if it fails, a fatal problem occurs in Rockbox. 
             */
            if (!rb->strcmp(preferences->font_name, rb->global_settings->font_file))
                return TV_CALLBACK_ERROR;

            font_changing = true;
            tv_copy_preferences(&new_prefs);
            rb->strlcpy(new_prefs.font_name, font_str, MAX_PATH);

            return (tv_set_preferences(&new_prefs))? TV_CALLBACK_STOP : TV_CALLBACK_ERROR;
        }
        col_width = 2 * rb->font_get_width(preferences->font, ' ');
        font_changing = false;
    }
#else
    (void)oldp;
#endif
    tv_change_viewport();
    return TV_CALLBACK_OK;
}

bool tv_init_display(unsigned char **buf, size_t *size)
{
    (void)buf;
    (void)size;

    display = rb->screens[SCREEN_MAIN];
    display->clear_viewport();

    tv_add_preferences_change_listner(tv_change_preferences);

    return true;
}

void tv_finalize_display(void)
{
#ifdef HAVE_LCD_BITMAP
    /* restore font */
    if (rb->strcmp(rb->global_settings->font_file, preferences->font_name))
    {
        tv_set_font(rb->global_settings->font_file);
    }

    /* undo viewport */
    if (is_initialized_vp)
        rb->viewportmanager_theme_undo(SCREEN_MAIN, false);
#endif
}

bool tv_exist_scrollbar(void)
{
#ifdef HAVE_LCD_BITMAP
    return true;
#else
    return false;
#endif
}
