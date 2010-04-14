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
#include "tv_drawtext.h"

#define WRAP_TRIM          44  /* Max number of spaces to trim (arbitrary) */
#define NARROW_MAX_COLUMNS 64  /* Max displayable string len [narrow] (over-estimate) */
#define WIDE_MAX_COLUMNS  128  /* Max displayable string len [wide] (over-estimate) */
#define MAX_WIDTH         910  /* Max line length in WIDE mode */
#define READ_PREV_ZONE    (block_size*9/10)  /* Arbitrary number less than SMALL_BLOCK_SIZE */
#define SMALL_BLOCK_SIZE  block_size /* Smallest file chunk we will read */
#define LARGE_BLOCK_SIZE  (block_size << 1) /* Preferable size of file chunk to read */
#define TOP_SECTOR     buffer
#define MID_SECTOR     (buffer + SMALL_BLOCK_SIZE)
#define BOTTOM_SECTOR  (buffer + (SMALL_BLOCK_SIZE << 1))
#undef SCROLLBAR_WIDTH
#define SCROLLBAR_WIDTH rb->global_settings->scrollbar_width
#define MAX_PAGE         9999

/* Is a scrollbar called for on the current screen? */
#define NEED_SCROLLBAR() \
 ((!(ONE_SCREEN_FITS_ALL())) && (prefs.scrollbar_mode==SB_ON))

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

static void viewer_scroll_up(void)
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

static void viewer_scroll_down(bool autoscroll)
{
    if (cpage == lpage)
        return;

    if (next_line_ptr != NULL)
        screen_top_ptr = next_line_ptr;

    if (prefs.scroll_mode == LINE || autoscroll)
        increment_current_line();
}

static void viewer_scroll_to_top_line(void)
{
    int line;

    for (line = cline; line > 1; line--)
        viewer_scroll_up();
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
    if (prefs.header_mode == HD_SBAR || prefs.header_mode == HD_BOTH)
        rb->gui_syncstatusbar_draw(rb->statusbars, true);

    if (prefs.header_mode == HD_PATH || prefs.header_mode == HD_BOTH)
        rb->lcd_putsxy(0, header_height - pf->height, file_name);
}

static void viewer_show_footer(void)
{
    if (prefs.footer_mode == FT_SBAR || prefs.footer_mode == FT_BOTH)
        rb->gui_syncstatusbar_draw(rb->statusbars, true);

    if (prefs.footer_mode == FT_PAGE || prefs.footer_mode == FT_BOTH)
    {
        unsigned char buf[12];

        if (cline == 1)
            rb->snprintf(buf, sizeof(buf), "%d", cpage);
        else
            rb->snprintf(buf, sizeof(buf), "%d - %d", cpage, cpage+1);

        rb->lcd_putsxy(0, LCD_HEIGHT - footer_height, buf);
    }
}
#endif

void viewer_draw(int col)
{
    int i, j, k, line_len, line_width, spaces, left_col=0;
    int width, extra_spaces, indent_spaces, spaces_per_word;
    bool multiple_spacing, line_is_short;
    unsigned short ch;
    unsigned char *str, *oldstr;
    unsigned char *line_begin;
    unsigned char *line_end;
    unsigned char c;
    unsigned char scratch_buffer[max_columns + 1];
    unsigned char utf8_buffer[max_columns*4 + 1];
    unsigned char *endptr;

    /* If col==-1 do all calculations but don't display */
    if (col != -1) {
#ifdef HAVE_LCD_BITMAP
        left_col = prefs.need_scrollbar? SCROLLBAR_WIDTH:0;
#else
        left_col = 0;
#endif
        rb->lcd_clear_display();
    }
    max_line_len = 0;
    line_begin = line_end = screen_top_ptr;

    for (i = 0; i < display_lines; i++) {
        if (BUFFER_OOB(line_end))
        {
            if (lpage == cpage)
                break;  /* Happens after display last line at BUFFER_EOF() */

            if (lpage == 0 && cline == 1)
            {
                lpage = cpage;
                last_screen_top_ptr = screen_top_ptr;
                last_file_pos = file_pos;
            }
        }

        get_next_line_position(&line_begin, &line_end, &line_is_short);
        if (line_end == NULL)
        {
           if (BUFFER_OOB(line_begin))
                break;
           line_end = buffer_end + 1;
        }

        line_len = line_end - line_begin;

        /* calculate line_len */
        str = oldstr = line_begin;
        j = -1;
        while (str < line_end) {
            oldstr = str;
            str = crop_at_width(str);
            j++;
        }
        line_width = j*draw_columns;
        while (oldstr < line_end) {
            oldstr = get_ucs(oldstr, &ch);
            line_width += glyph_width(ch);
        }

        if (prefs.line_mode == JOIN) {
            if (line_begin[0] == 0) {
                line_begin++;
                if (prefs.word_mode == CHOP)
                    line_end++;
                else
                    line_len--;
            }
            for (j=k=spaces=0; j < line_len; j++) {
                if (k == max_columns)
                    break;

                c = line_begin[j];
                switch (c) {
                    case ' ':
                        spaces++;
                        break;
                    case 0:
                        spaces = 0;
                        scratch_buffer[k++] = ' ';
                        break;
                    default:
                        while (spaces) {
                            spaces--;
                            scratch_buffer[k++] = ' ';
                            if (k == max_columns - 1)
                                break;
                        }
                        scratch_buffer[k++] = c;
                        break;
                }
            }
            if (col != -1) {
                scratch_buffer[k] = 0;
                endptr = decode2utf8(scratch_buffer, utf8_buffer, col, draw_columns);
                *endptr = 0;
            }
        }
        else if (prefs.line_mode == REFLOW) {
            if (line_begin[0] == 0) {
                line_begin++;
                if (prefs.word_mode == CHOP)
                    line_end++;
                else
                    line_len--;
            }

            indent_spaces = 0;
            if (!line_is_short) {
                multiple_spacing = false;
                width=spaces=0;
                for (str = line_begin; str < line_end; ) {
                    str = get_ucs(str, &ch);
                    switch (ch) {
                        case ' ':
                        case 0:
                            if ((str == line_begin) && (prefs.word_mode==WRAP))
                                /* special case: indent the paragraph,
                                 * don't count spaces */
                                indent_spaces = par_indent_spaces;
                            else if (!multiple_spacing)
                                spaces++;
                            multiple_spacing = true;
                            break;
                        default:
                            multiple_spacing = false;
                            width += glyph_width(ch);
                            break;
                    }
                }
                if (multiple_spacing) spaces--;

                if (spaces) {
                    /* total number of spaces to insert between words */
                    extra_spaces = (max_width-width)/glyph_width(' ')
                            - indent_spaces;
                    /* number of spaces between each word*/
                    spaces_per_word = extra_spaces / spaces;
                    /* number of words with n+1 spaces (to fill up) */
                    extra_spaces = extra_spaces % spaces;
                    if (spaces_per_word > 2) { /* too much spacing is awful */
                        spaces_per_word = 3;
                        extra_spaces = 0;
                    }
                } else { /* this doesn't matter much... no spaces anyway */
                    spaces_per_word = extra_spaces = 0;
                }
            } else { /* end of a paragraph: don't fill line */
                spaces_per_word = 1;
                extra_spaces = 0;
            }

            multiple_spacing = false;
            for (j=k=spaces=0; j < line_len; j++) {
                if (k == max_columns)
                    break;

                c = line_begin[j];
                switch (c) {
                    case ' ':
                    case 0:
                        if (j==0 && prefs.word_mode==WRAP) { /* indent paragraph */
                            for (j=0; j<par_indent_spaces; j++)
                                scratch_buffer[k++] = ' ';
                            j=0;
                        }
                        else if (!multiple_spacing) {
                            for (width = spaces<extra_spaces ? -1:0; width < spaces_per_word; width++)
                                    scratch_buffer[k++] = ' ';
                            spaces++;
                        }
                        multiple_spacing = true;
                        break;
                    default:
                        scratch_buffer[k++] = c;
                        multiple_spacing = false;
                        break;
                }
            }
            if (col != -1) {
                scratch_buffer[k] = 0;
                endptr = decode2utf8(scratch_buffer, utf8_buffer, col, draw_columns);
                *endptr = 0;
            }
        }
        else { /* prefs.line_mode != JOIN && prefs.line_mode != REFLOW */
            if (col != -1)
                if (line_width > col) {
                    str = oldstr = line_begin;
                    k = col;
                    width = 0;
                    while( (width<draw_columns) && (oldstr<line_end) )
                    {
                        oldstr = get_ucs(oldstr, &ch);
                        if (k > 0) {
                            k -= glyph_width(ch);
                            line_begin = oldstr;
                        } else {
                            width += glyph_width(ch);
                        }
                    }

                    if(prefs.view_mode==WIDE)
                        endptr = rb->iso_decode(line_begin, utf8_buffer,
                                            prefs.encoding, oldstr-line_begin);
                    else
                        endptr = rb->iso_decode(line_begin, utf8_buffer,
                                            prefs.encoding, line_end-line_begin);
                    *endptr = 0;
                }
        }
        if (col != -1 && line_width > col)
        {
            int dpage = (cline+i <= display_lines)?cpage:cpage+1;
            int dline = cline+i - ((cline+i <= display_lines)?0:display_lines);
            bool bflag = (viewer_find_bookmark(dpage, dline) >= 0);
#ifdef HAVE_LCD_BITMAP
            int dy = i * pf->height + header_height;
#endif
            if (bflag)
#ifdef HAVE_LCD_BITMAP
            {
                rb->lcd_set_drawmode(DRMODE_BG|DRMODE_FG);
                rb->lcd_fillrect(left_col, dy, LCD_WIDTH, pf->height);
                rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            }
            rb->lcd_putsxy(left_col, dy, utf8_buffer);
            rb->lcd_set_drawmode(DRMODE_SOLID);
#else
            {
                rb->lcd_puts(left_col, i, BOOKMARK_ICON);
            }
            rb->lcd_puts(left_col+1, i, utf8_buffer);
#endif
        }
        if (line_width > max_line_len)
            max_line_len = line_width;

        if (i == 0)
            next_line_ptr = line_end;
    }
    next_screen_ptr = line_end;
    if (BUFFER_OOB(next_screen_ptr))
        next_screen_ptr = NULL;

#ifdef HAVE_LCD_BITMAP
    next_screen_to_draw_ptr = prefs.page_mode==OVERLAP? line_begin: next_screen_ptr;

    if (prefs.need_scrollbar)
        viewer_scrollbar();
#else
    next_screen_to_draw_ptr = next_screen_ptr;
#endif

#ifdef HAVE_LCD_BITMAP
    /* show header */
    viewer_show_header();
    
    /* show footer */
    viewer_show_footer();
#endif

    if (col != -1)
        rb->lcd_update();
}

#ifdef HAVE_LCD_BITMAP
static void init_need_scrollbar(void) {
    /* Call viewer_draw in quiet mode to initialize next_screen_ptr,
     and thus ONE_SCREEN_FITS_ALL(), and thus NEED_SCROLLBAR() */
    viewer_draw(-1);
    prefs.need_scrollbar = NEED_SCROLLBAR();
    draw_columns = prefs.need_scrollbar? display_columns-SCROLLBAR_WIDTH : display_columns;
    par_indent_spaces = draw_columns/(5*glyph_width(' '));
    calc_max_width();
}

static void init_header_and_footer(void)
{
    header_height = 0;
    footer_height = 0;
    if (rb->global_settings->statusbar == STATUSBAR_TOP)
    {
        if (prefs.header_mode == HD_SBAR || prefs.header_mode == HD_BOTH)
            header_height = STATUSBAR_HEIGHT;

        if (prefs.footer_mode == FT_SBAR)
            prefs.footer_mode = FT_NONE;
        else if (prefs.footer_mode == FT_BOTH)
            prefs.footer_mode = FT_PAGE;
    }
    else if (rb->global_settings->statusbar == STATUSBAR_BOTTOM)
    {
        if (prefs.footer_mode == FT_SBAR || prefs.footer_mode == FT_BOTH)
            footer_height = STATUSBAR_HEIGHT;

        if (prefs.header_mode == HD_SBAR)
            prefs.header_mode = HD_NONE;
        else if (prefs.header_mode == HD_BOTH)
            prefs.header_mode = HD_PATH;
    }
    else /* STATUSBAR_OFF || STATUSBAR_CUSTOM */
    {
        if (prefs.header_mode == HD_SBAR)
            prefs.header_mode = HD_NONE;
        else if (prefs.header_mode == HD_BOTH)
            prefs.header_mode = HD_PATH;

        if (prefs.footer_mode == FT_SBAR)
            prefs.footer_mode = FT_NONE;
        else if (prefs.footer_mode == FT_BOTH)
            prefs.footer_mode = FT_PAGE;
    }

    if (prefs.header_mode == HD_NONE || prefs.header_mode == HD_PATH ||
        prefs.footer_mode == FT_NONE || prefs.footer_mode == FT_PAGE)
        rb->gui_syncstatusbar_draw(rb->statusbars, false);

    if (prefs.header_mode == HD_PATH || prefs.header_mode == HD_BOTH)
        header_height += pf->height;
    if (prefs.footer_mode == FT_PAGE || prefs.footer_mode == FT_BOTH)
        footer_height += pf->height;

    display_lines = (LCD_HEIGHT - header_height - footer_height) / pf->height;

    lpage = 0;
    last_file_pos = 0;
    last_screen_top_ptr = NULL;
}

static void change_font(unsigned char *font)
{
    unsigned char buf[MAX_PATH];

    if (font == NULL || *font == '\0')
        return;

    rb->snprintf(buf, MAX_PATH, "%s/%s.fnt", FONT_DIR, font);
    if (rb->font_load(NULL, buf) < 0)
        rb->splash(HZ/2, "font load failed.");
}
#endif

/* When a file is UTF-8 file with BOM, if prefs.encoding is UTF-8,
 * then file size decreases only BOM_SIZE.
 */
static void get_filesize(void)
{
    file_size = rb->filesize(fd);
    if (file_size == -1)
        return;

    if (prefs.encoding == UTF_8 && is_bom)
        file_size -= BOM_SIZE;
}
