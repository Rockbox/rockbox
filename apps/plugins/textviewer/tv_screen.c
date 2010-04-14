/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Gilles Roux, 2003 Garrett Derner
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
#include "tv_screen.h"
#include "tv_settings.h"

static int draw_columns; /* number of (pixel) columns available for text */
static int par_indent_spaces; /* number of spaces to indent first paragraph */

#ifdef HAVE_LCD_BITMAP
static struct font *pf;
static int header_height = 0;
static int footer_height = 0;
#endif

void viewer_init_screen(void)
{
#ifdef HAVE_LCD_BITMAP
    /* initialize fonts */
    pf = rb->font_get(FONT_UI);
    draw_columns = display_columns = LCD_WIDTH;
#else
    /* REAL fixed pitch :) all chars use up 1 cell */
    display_lines = 2;
    draw_columns = display_columns = 11;
    par_indent_spaces = 2;
#endif
}

void viewer_finalize_screen(void)
{
    change_font(rb->global_settings->font_file);
}

void viewer_draw(void)
{
}

void viewer_top(void)
{
    /* Read top of file into buffer
      and point screen pointer to top */
    if (file_pos != 0)
    {
        rb->splash(0, "Loading...");

        file_pos = 0;
        buffer_end = BUFFER_END();  /* Update whenever file_pos changes */
        fill_buffer(0, buffer, buffer_size);
    }

    screen_top_ptr = buffer;
    cpage = 1;
    cline = 1;
}

void viewer_bottom(void)
{
    unsigned char *line_begin;
    unsigned char *line_end;

    rb->splash(0, "Loading...");

    if (last_screen_top_ptr)
    {
        cpage = lpage;
        cline = 1;
        screen_top_ptr = last_screen_top_ptr;
        file_pos = last_file_pos;
        fill_buffer(file_pos, buffer, buffer_size);
        buffer_end = BUFFER_END();
        return;
    }

    line_end = screen_top_ptr;

    while (!BUFFER_EOF() || !BUFFER_OOB(line_end))
    {
        get_next_line_position(&line_begin, &line_end, NULL);
        if (line_end == NULL)
            break;

        increment_current_line();
        if (cline == 1)
            screen_top_ptr = line_end;
    }
    lpage = cpage;
    cline = 1;
    last_screen_top_ptr = screen_top_ptr;
    last_file_pos = file_pos;
    buffer_end = BUFFER_END();
}

static void increment_current_line(void)
{
    if (cline < display_lines)
        cline++;
    else if (cpage < MAX_PAGE)
    {
        cpage++;
        cline = 1;
    }
}

static void decrement_current_line(void)
{
    if (cline > 1)
        cline--;
    else if (cpage > 1)
    {
        cpage--;
        cline = display_lines;
    }
}

static void viewer_line_scroll_up(void)
{
    unsigned char *p;

    p = find_prev_line(screen_top_ptr);
    if (p == NULL && !BUFFER_BOF()) {
        read_and_synch(-1);
        p = find_prev_line(screen_top_ptr);
    }
    if (p != NULL)
        screen_top_ptr = p;

    decrement_current_line();
}

static void viewer_line_scroll_down(void)
{
    if (cpage == lpage)
        return;

    if (next_line_ptr != NULL)
        screen_top_ptr = next_line_ptr;

    increment_current_line();
}

static void viewer_scroll_to_top_line(void)
{
    int line;

    for (line = cline; line > 1; line--)
        viewer_scroll_up();
}

void viewer_scroll_up(int mode)
{
    int i;
    int line_count = 1;
    struct viewer_preference *prefs = viewer_get_preference();

    if ((mode == VIEWER_SCROLL_PAGE) ||
        (mode == VIEWER_SCROLL_PREFS && prefs->scroll_mode == PAGE))
    {
#ifdef HAVE_LCD_BITMAP
        line_count = display_lines - ((prefs->page_mode==OVERLAP)? 1:0);
#else
        line_count = display_lines;
#endif
    }

    for (i = 0; i < line_count; i++)
        viewer_line_scroll_up();
}

void viewer_scroll_down(int mode)
{
    struct viewer_preference *prefs = viewer_get_preference();

    if ((mode == VIEWER_SCROLL_PAGE) ||
        (mode == VIEWER_SCROLL_PREFS && prefs->scroll_mode == PAGE))
    {
        /* Page down */
        if (next_screen_ptr != NULL)
        {
            screen_top_ptr = next_screen_to_draw_ptr;
            if (cpage < MAX_PAGE)
                cpage++;
        }
    }
    else
        viewer_line_scroll_down();
}

#ifdef HAVE_LCD_BITMAP
static void viewer_scrollbar(void) {
    int items, min_shown, max_shown, sb_begin_y, sb_height;

    items = (int) file_size;  /* (SH1 int is same as long) */
    min_shown = (int) file_pos + (screen_top_ptr - buffer);

    if (next_screen_ptr == NULL)
        max_shown = items;
    else
        max_shown = min_shown + (next_screen_ptr - screen_top_ptr);

    sb_begin_y = header_height;
    sb_height  = LCD_HEIGHT - header_height - footer_height;

    rb->gui_scrollbar_draw(rb->screens[SCREEN_MAIN],0, sb_begin_y,
                           SCROLLBAR_WIDTH-1, sb_height,
                           items, min_shown, max_shown, VERTICAL);
}

static void viewer_show_header(void)
{
    struct viewer_preference *prefs = viewer_get_preference();

    if (prefs->header_mode == HD_SBAR || prefs->header_mode == HD_BOTH)
        rb->gui_syncstatusbar_draw(rb->statusbars, true);

    if (prefs->header_mode == HD_PATH || prefs->header_mode == HD_BOTH)
        rb->lcd_putsxy(0, header_height - pf->height, file_name);
}

static void viewer_show_footer(void)
{
    struct viewer_preference *prefs = viewer_get_preference();

    if (prefs->footer_mode == FT_SBAR || prefs->footer_mode == FT_BOTH)
        rb->gui_syncstatusbar_draw(rb->statusbars, true);

    if (prefs->footer_mode == FT_PAGE || prefs->footer_mode == FT_BOTH)
    {
        unsigned char buf[12];

        if (cline == 1)
            rb->snprintf(buf, sizeof(buf), "%d", cpage);
        else
            rb->snprintf(buf, sizeof(buf), "%d - %d", cpage, cpage+1);

        rb->lcd_putsxy(0, LCD_HEIGHT - footer_height, buf);
    }
}

static bool need_scrollbar(void)
{
    struct viewer_preference *prefs = viewer_get_preference();

    return prefs->need_scrollbar;
}

static void init_need_scrollbar(void) {
    draw_columns = need_scrollbar()? display_columns-SCROLLBAR_WIDTH : display_columns;
    par_indent_spaces = draw_columns/(5*glyph_width(' '));
    calc_max_width();
}

static void init_header_and_footer(void)
{
    struct viewer_preference *prefs = viewer_get_preference();

    header_height = 0;
    footer_height = 0;
    if (rb->global_settings->statusbar == STATUSBAR_TOP)
    {
        if (prefs->header_mode == HD_SBAR || prefs->header_mode == HD_BOTH)
            header_height = STATUSBAR_HEIGHT;

        if (prefs->footer_mode == FT_SBAR)
            prefs->footer_mode = FT_NONE;
        else if (prefs->footer_mode == FT_BOTH)
            prefs->footer_mode = FT_PAGE;
    }
    else if (rb->global_settings->statusbar == STATUSBAR_BOTTOM)
    {
        if (prefs->footer_mode == FT_SBAR || prefs->footer_mode == FT_BOTH)
            footer_height = STATUSBAR_HEIGHT;

        if (prefs->header_mode == HD_SBAR)
            prefs->header_mode = HD_NONE;
        else if (prefs->header_mode == HD_BOTH)
            prefs->header_mode = HD_PATH;
    }
    else /* STATUSBAR_OFF || STATUSBAR_CUSTOM */
    {
        if (prefs->header_mode == HD_SBAR)
            prefs->header_mode = HD_NONE;
        else if (prefs->header_mode == HD_BOTH)
            prefs->header_mode = HD_PATH;

        if (prefs->footer_mode == FT_SBAR)
            prefs->footer_mode = FT_NONE;
        else if (prefs->footer_mode == FT_BOTH)
            prefs->footer_mode = FT_PAGE;
    }

    if (prefs->header_mode == HD_NONE || prefs->header_mode == HD_PATH ||
        prefs->footer_mode == FT_NONE || prefs->footer_mode == FT_PAGE)
        rb->gui_syncstatusbar_draw(rb->statusbars, false);

    if (prefs->header_mode == HD_PATH || prefs->header_mode == HD_BOTH)
        header_height += pf->height;
    if (prefs->footer_mode == FT_PAGE || prefs->footer_mode == FT_BOTH)
        footer_height += pf->height;

    display_lines = (LCD_HEIGHT - header_height - footer_height) / pf->height;

    lpage = 0;
    last_file_pos = 0;
    last_screen_top_ptr = NULL;
}

void change_font(unsigned char *font)
{
    unsigned char buf[MAX_PATH];
    struct viewer_preference *prefs = viewer_get_preference();

    if (font == NULL || *font == '\0')
        return;

    if (rb->strcmp(prefs->font, rb->global_settings->font_file) == 0)
        return;

    rb->snprintf(buf, MAX_PATH, "%s/%s.fnt", FONT_DIR, font);
    if (rb->font_load(NULL, buf) < 0)
        rb->splash(HZ/2, "font load failed.");
}
#else
#define change_font(f)
#endif

static void calc_page(void)
{
    int i;
    unsigned char *line_begin;
    unsigned char *line_end;
    off_t sfp;
    unsigned char *sstp;
    struct viewer_preference *prefs = viewer_get_preference();

    rb->splash(0, "Calculating page/line number...");

    /* add reading page to bookmarks */
    viewer_add_last_read_bookmark();

    rb->qsort(bookmarks, bookmark_count, sizeof(struct bookmark_info),
              bm_comp);

    cpage = 1;
    cline = 1;
    file_pos = 0;
    screen_top_ptr = buffer;
    buffer_end = BUFFER_END();

    fill_buffer(file_pos, buffer, buffer_size);
    line_end = line_begin = buffer;

    for (i = 0; i < bookmark_count; i++)
    {
        sfp = bookmarks[i].file_position;
        sstp = buffer;

        while ((line_begin > sstp || sstp >= line_end) ||
               (file_pos > sfp || sfp >= file_pos + BUFFER_END() - buffer))
        {
            get_next_line_position(&line_begin, &line_end, NULL);
            if (line_end == NULL)
                break;

            next_line_ptr = line_end;

            if (sstp == buffer &&
                file_pos <= sfp && sfp < file_pos + BUFFER_END() - buffer)
                sstp = sfp - file_pos + buffer;

            increment_current_line();
        }

        decrement_current_line();
        bookmarks[i].page = cpage;
        bookmarks[i].line = cline;
        bookmarks[i].file_position = file_pos + (line_begin - buffer);
        increment_current_line();
    }

    /* remove reading page's bookmark */
    for (i = 0; i < bookmark_count; i++)
    {
        if (bookmarks[i].flag & BOOKMARK_LAST)
        {
            int screen_pos;
            int screen_top;

            screen_pos = bookmarks[i].file_position;
            screen_top = screen_pos % buffer_size;
            file_pos = screen_pos - screen_top;
            screen_top_ptr = buffer + screen_top;

            cpage = bookmarks[i].page;
            cline = bookmarks[i].line;
            bookmarks[i].flag ^= BOOKMARK_LAST;
            buffer_end = BUFFER_END();

            fill_buffer(file_pos, buffer, buffer_size);

            if (bookmarks[i].flag == 0)
                viewer_remove_bookmark(i);

            if (prefs->scroll_mode == PAGE && cline > 1)
                viewer_scroll_to_top_line();
            break;
        }
    }
}

static int col_limit(int col)
{
    if (col < 0)
        col = 0;
    else
        if (col >= max_width)
            col = max_width - draw_columns;

    return col;
}

void viewer_scroll_left(int mode)
{
    if (mode == VIEWER_SCROLL_COLUMN)
    {
        /* Scroll left one column */
        col -= glyph_width('o');
    }
    else
    {
        /* Screen left */
        col -= draw_columns;
    }
    col = col_limit(col);
}

void viewer_scroll_right(int mode)
{
    if (mode == VIEWER_SCROLL_COLUMN)
    {
        /* Scroll right one column */
        col += glyph_width('o');
    }
    else
    {
        /* Screen right */
        col += draw_columns;
    }
    col = col_limit(col);
}
