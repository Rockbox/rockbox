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
 */

#define TV_SCROLLBAR_WIDTH  rb->global_settings->scrollbar_width
#define TV_SCROLLBAR_HEIGHT 4

struct tv_rect {
    int x;
    int y;
    int w;
    int h;
};

static struct viewport vp_info;
static struct viewport vp_text;
static bool is_initialized_vp = false;

static struct screen* display;

/* layout */
static struct tv_rect header;
static struct tv_rect footer;
static struct tv_rect horizontal_scrollbar;
static struct tv_rect vertical_scrollbar;

static bool show_horizontal_scrollbar;
static bool show_vertical_scrollbar;

static int display_columns;
static int display_rows;

static int col_width  = 1;
static int row_height = 1;

static int totalsize;

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

void tv_init_scrollbar(off_t total, bool show_scrollbar)
{
    totalsize = total;
    show_horizontal_scrollbar = (show_scrollbar && preferences->horizontal_scrollbar);
    show_vertical_scrollbar   = (show_scrollbar && preferences->vertical_scrollbar);
}

void tv_show_bookmarks(const int *rows, int count)
{
    display->set_viewport(&vp_text);
    display->set_drawmode(DRMODE_COMPLEMENT);

    while (count--)
    {
        display->fillrect(0, rows[count] * row_height,
                          vp_text.width, row_height);
    }
    display->set_drawmode(DRMODE_SOLID);
    display->set_viewport(&vp_info);
}

void tv_update_extra(int window, int col, const struct tv_screen_pos *pos, int size)
{
    tv_show_scrollbar(window, col, pos->file_pos, size);
    tv_show_header();
    tv_show_footer(pos);
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
        xpos += ((offset > 0)? vp_text.width * 2 : vp_text.width) - text_width;
    }

    display->set_viewport(&vp_text);
    tv_night_mode();
    display->putsxy(xpos, row * row_height, text);
    display->set_viewport(&vp_info);
}

void tv_start_display(void)
{
    display->set_viewport(&vp_info);
    tv_night_mode();
    display->set_drawmode(DRMODE_SOLID);

#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif
    display->clear_viewport();

}

void tv_end_display(void)
{
    display->update_viewport();
    display->set_viewport(NULL);
}

void tv_set_layout(bool show_scrollbar)
{
    int scrollbar_width  = (show_scrollbar && preferences->vertical_scrollbar)?
                           TV_SCROLLBAR_WIDTH  + 1 : 0;
    int scrollbar_height = (show_scrollbar && preferences->horizontal_scrollbar)?
                           TV_SCROLLBAR_HEIGHT + 1 : 0;

    row_height = rb->font_get(preferences->font_id)->height;

    header.x = 0;
    header.y = 0;
    header.w = vp_info.width;
    header.h = (preferences->header_mode)? row_height + 1 : 0;

    footer.x = 0;
    footer.w = vp_info.width;
    footer.h = (preferences->footer_mode)? row_height + 1 : 0;
    footer.y = vp_info.height - footer.h;

    horizontal_scrollbar.x = scrollbar_width;
    horizontal_scrollbar.y = footer.y - scrollbar_height;
    horizontal_scrollbar.w = vp_info.width - scrollbar_width;
    horizontal_scrollbar.h = scrollbar_height;

    vertical_scrollbar.x = 0;
    vertical_scrollbar.y = header.y + header.h;
    vertical_scrollbar.w = scrollbar_width;
    vertical_scrollbar.h = footer.y - vertical_scrollbar.y - scrollbar_height;

    vp_info.font = preferences->font_id;
    vp_text = vp_info;
    vp_text.x += horizontal_scrollbar.x;
    vp_text.y += vertical_scrollbar.y;
    vp_text.width = horizontal_scrollbar.w;
    vp_text.height = vertical_scrollbar.h;

    display_columns = vp_text.width / col_width;
    display_rows    = vp_text.height / row_height;
}

void tv_get_drawarea_info(int *width, int *cols, int *rows)
{
    *width = vp_text.width;
    *cols  = display_columns;
    *rows  = display_rows;
}

static void tv_change_viewport(void)
{
    bool show_statusbar = preferences->statusbar;

    if (is_initialized_vp)
        rb->viewportmanager_theme_undo(SCREEN_MAIN, false);
    else
        is_initialized_vp = true;

    rb->viewportmanager_theme_enable(SCREEN_MAIN, show_statusbar, &vp_info);
    vp_info.flags &= ~VP_FLAG_ALIGNMENT_MASK;
    display->set_viewport(&vp_info);
}

static bool tv_set_font(const unsigned char *font)
{
    unsigned char path[MAX_PATH];
    if (font != NULL && *font != '\0')
    {
        rb->snprintf(path, MAX_PATH, "%s/%s.fnt", FONT_DIR, font);
        if (preferences->font_id >= 0 &&
            (preferences->font_id != rb->global_status->font_id[SCREEN_MAIN]))
            rb->font_unload(preferences->font_id);
        tv_change_fontid(rb->font_load(path));
        if (preferences->font_id < 0)
        {
            rb->splash(HZ/2, "font load failed");
            return false;
        }
    }
    vp_text.font = preferences->font_id;
    return true;
}

static int tv_change_preferences(const struct tv_preferences *oldp)
{
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
        col_width = 2 * rb->font_get_width(rb->font_get(preferences->font_id), ' ');
        font_changing = false;
    }
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

void tv_night_mode(void)
{
#ifdef HAVE_LCD_COLOR
     if(preferences->night_mode)
    {
        rb->lcd_set_foreground(LCD_RGBPACK(0xBF,0xBF,0x00));
        rb->lcd_set_background(LCD_RGBPACK(0x96,0x0D,0x00));
    }else
    {
        rb->lcd_set_foreground(LCD_WHITE);
        rb->lcd_set_background(LCD_BLACK);
    }
#endif
}

void tv_finalize_display(void)
{
    /* restore font */
    if (preferences->font_id >= 0 &&
        (preferences->font_id != rb->global_status->font_id[SCREEN_MAIN]))
    {
        rb->font_unload(preferences->font_id);
    }

    /* undo viewport */
    if (is_initialized_vp)
        rb->viewportmanager_theme_undo(SCREEN_MAIN, false);
}

bool tv_exist_scrollbar(void)
{
    return true;
}
