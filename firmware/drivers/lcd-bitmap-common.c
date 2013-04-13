/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Dave Chapman
 *     Text rendering
 * Copyright (C) 2006 Shachar Liberman
 *     Offset text, scrolling
 * Copyright (C) 2007 Nicolas Pennequin, Tom Ross, Ken Fazzone, Akio Idehara
 *     Color gradient background
 * Copyright (C) 2009 Andrew Mahone
 *     Merged common LCD bitmap code
 *
 * Rockbox common bitmap LCD functions
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
#include <stdarg.h>
#include <stdio.h>
#include "string-extra.h"
#include "diacritic.h"

#ifndef LCDFN /* Not compiling for remote - define macros for main LCD. */
#define LCDFN(fn) lcd_ ## fn
#define FBFN(fn)  fb_ ## fn
#define LCDM(ma) LCD_ ## ma
#define LCDNAME "lcd_"
#define MAIN_LCD
#endif

#if defined(MAIN_LCD) && defined(HAVE_LCD_COLOR)
/* Fill a rectangle with a gradient. This function draws only the partial
 * gradient. It assumes the original gradient is src_height high and skips
 * the first few rows. This is useful for drawing only the bottom half of
 * a full gradient.
 *
 * height == src_height and row_skip == 0 will draw the full gradient
 *
 * x, y, width, height - dimensions describing the rectangle
 * start_rgb - beginning color of the gradient
 * end_rgb - end color of the gradient
 * src_height - assumed original height (only height rows will be drawn)
 * row_skip - how many rows of the original gradient to skip
 */
void lcd_gradient_fillrect_part(int x, int y, int width, int height,
        unsigned start_rgb, unsigned end_rgb, int src_height, int row_skip)
{
    int old_pattern = current_vp->fg_pattern;
    int step_mul, i;
    int x1, x2;
    x1 = x;
    x2 = x + width;
    
    if (height == 0) return;

    step_mul = (1 << 16) / src_height;
    int h_r = RGB_UNPACK_RED(start_rgb);
    int h_g = RGB_UNPACK_GREEN(start_rgb);
    int h_b = RGB_UNPACK_BLUE(start_rgb);
    int rstep = (h_r - RGB_UNPACK_RED(end_rgb)) * step_mul;
    int gstep = (h_g - RGB_UNPACK_GREEN(end_rgb)) * step_mul;
    int bstep = (h_b - RGB_UNPACK_BLUE(end_rgb)) * step_mul;
    h_r = (h_r << 16) + (1 << 15);
    h_g = (h_g << 16) + (1 << 15);
    h_b = (h_b << 16) + (1 << 15);

    if (row_skip > 0)
    {
        h_r -= rstep * row_skip;
        h_g -= gstep * row_skip;
        h_b -= bstep * row_skip;
    }

    for(i = y; i < y + height; i++) {
        current_vp->fg_pattern = LCD_RGBPACK(h_r >> 16, h_g >> 16, h_b >> 16);
        lcd_hline(x1, x2, i);
        h_r -= rstep;
        h_g -= gstep;
        h_b -= bstep;
    }

    current_vp->fg_pattern = old_pattern;
}

/* Fill a rectangle with a gradient. The gradient's color will fade from
 * start_rgb to end_rgb over the height of the rectangle
 *
 * x, y, width, height - dimensions describing the rectangle
 * start_rgb - beginning color of the gradient
 * end_rgb - end color of the gradient
 */
void lcd_gradient_fillrect(int x, int y, int width, int height,
        unsigned start_rgb, unsigned end_rgb)
{
    lcd_gradient_fillrect_part(x, y, width, height, start_rgb, end_rgb, height, 0);
}

/* Fill a text line with a gradient:
 * x1, x2 - x pixel coordinates to start/stop
 * y - y pixel to start from
 * h - line height
 * num_lines - number of lines to span the gradient over
 * cur_line - current line being draw
 */
static void lcd_do_gradient_line(int x1, int x2, int y, unsigned h,
                              int num_lines, int cur_line)
{
    int step_mul;
    if (h == 0) return;

    num_lines *= h;
    cur_line *= h;
    step_mul = (1 << 16) / (num_lines);
    int h_r = RGB_UNPACK_RED(current_vp->lss_pattern);
    int h_g = RGB_UNPACK_GREEN(current_vp->lss_pattern);
    int h_b = RGB_UNPACK_BLUE(current_vp->lss_pattern);
    int rstep = (h_r - RGB_UNPACK_RED(current_vp->lse_pattern)) * step_mul;
    int gstep = (h_g - RGB_UNPACK_GREEN(current_vp->lse_pattern)) * step_mul;
    int bstep = (h_b - RGB_UNPACK_BLUE(current_vp->lse_pattern)) * step_mul;
    unsigned start_rgb, end_rgb;
    h_r = (h_r << 16) + (1 << 15);
    h_g = (h_g << 16) + (1 << 15);
    h_b = (h_b << 16) + (1 << 15);
    if (cur_line)
    {
        h_r -= cur_line * rstep;
        h_g -= cur_line * gstep;
        h_b -= cur_line * bstep;
    }
    start_rgb = LCD_RGBPACK(h_r >> 16, h_g >> 16, h_b >> 16);

    h_r -= h * rstep;
    h_g -= h * gstep;
    h_b -= h * bstep;
    end_rgb = LCD_RGBPACK(h_r >> 16, h_g >> 16, h_b >> 16);
    lcd_gradient_fillrect(x1, y, x2 - x1, h, start_rgb, end_rgb);
}

#endif

void LCDFN(set_framebuffer)(FBFN(data) *fb)
{
    if (fb)
        LCDFN(framebuffer) = fb;
    else
        LCDFN(framebuffer) = &LCDFN(static_framebuffer)[0][0];
}

/*
 * draws the borders of the current viewport
 **/
void LCDFN(draw_border_viewport)(void)
{
    LCDFN(drawrect)(0, 0, current_vp->width, current_vp->height);
}

/*
 * fills the rectangle formed by current_vp
 **/
void LCDFN(fill_viewport)(void)
{
    LCDFN(fillrect)(0, 0, current_vp->width, current_vp->height);
}


/*** Viewports ***/

void LCDFN(set_viewport)(struct viewport* vp)
{
    if (vp == NULL)
        current_vp = &default_vp;
    else
        current_vp = vp;

#if LCDM(DEPTH) > 1
    LCDFN(set_foreground)(current_vp->fg_pattern);
    LCDFN(set_background)(current_vp->bg_pattern);
#endif

#if defined(SIMULATOR)
    /* Force the viewport to be within bounds.  If this happens it should
     *  be considered an error - the viewport will not draw as it might be
     *  expected.
     */
    if((unsigned) current_vp->x > (unsigned) LCDM(WIDTH)
        || (unsigned) current_vp->y > (unsigned) LCDM(HEIGHT)
        || current_vp->x + current_vp->width > LCDM(WIDTH)
        || current_vp->y + current_vp->height > LCDM(HEIGHT))
    {
#if !defined(HAVE_VIEWPORT_CLIP)
        DEBUGF("ERROR: "
#else
        DEBUGF("NOTE: "
#endif
            "set_viewport out of bounds: x: %d y: %d width: %d height:%d\n",
            current_vp->x, current_vp->y,
            current_vp->width, current_vp->height);
    }
#endif
}

struct viewport *LCDFN(get_viewport)(bool *is_default)
{
    *is_default = (current_vp == &default_vp);
    return current_vp;
}

void LCDFN(update_viewport)(void)
{
    LCDFN(update_rect)(current_vp->x, current_vp->y,
                    current_vp->width, current_vp->height);
}

void LCDFN(update_viewport_rect)(int x, int y, int width, int height)
{
    LCDFN(update_rect)(current_vp->x + x, current_vp->y + y, width, height);
}

/* put a string at a given pixel position, skipping first ofs pixel columns */
static void LCDFN(putsxyofs)(int x, int y, int ofs, const unsigned char *str)
{
    unsigned short *ucs;
    font_lock(current_vp->font, true);
    struct font* pf = font_get(current_vp->font);
    int vp_flags = current_vp->flags;
    int rtl_next_non_diac_width, last_non_diacritic_width;

    if ((vp_flags & VP_FLAG_ALIGNMENT_MASK) != 0)
    {
        int w;

        LCDFN(getstringsize)(str, &w, NULL);
        /* center takes precedence */
        if (vp_flags & VP_FLAG_ALIGN_CENTER)
        {
            x = ((current_vp->width - w)/ 2) + x;
            if (x < 0)
                x = 0;
        }
        else
        {
            x = current_vp->width - w - x;
            x += ofs;
            ofs = 0;
        }
    }

    rtl_next_non_diac_width = 0;
    last_non_diacritic_width = 0;
    /* Mark diacritic and rtl flags for each character */
    for (ucs = bidi_l2v(str, 1); *ucs; ucs++)
    {
        bool is_rtl, is_diac;
        const unsigned char *bits;
        int width, base_width, drawmode = 0, base_ofs = 0;
        const unsigned short next_ch = ucs[1];

        if (x >= current_vp->width)
            break;

        is_diac = is_diacritic(*ucs, &is_rtl);

        /* Get proportional width and glyph bits */
        width = font_get_width(pf, *ucs);

        /* Calculate base width */
        if (is_rtl)
        {
            /* Forward-seek the next non-diacritic character for base width */
            if (is_diac)
            {
                if (!rtl_next_non_diac_width)
                {
                    const unsigned short *u;

                    /* Jump to next non-diacritic char, and calc its width */
                    for (u = &ucs[1]; *u && is_diacritic(*u, NULL); u++);

                    rtl_next_non_diac_width = *u ?  font_get_width(pf, *u) : 0;
                }
                base_width = rtl_next_non_diac_width;
            }
            else
            {
                rtl_next_non_diac_width = 0; /* Mark */
                base_width = width;
            }
        }
        else
        {
            if (!is_diac)
                last_non_diacritic_width = width;

            base_width = last_non_diacritic_width;
        }

        if (ofs > width)
        {
            ofs -= width;
            continue;
        }

        if (is_diac)
        {
            /* XXX: Suggested by amiconn:
             * This will produce completely wrong results if the original
             * drawmode is DRMODE_COMPLEMENT. We need to pre-render the current
             * character with all its diacritics at least (in mono) and then
             * finally draw that. And we'll need an extra buffer that can hold
             * one char's bitmap. Basically we can't just change the draw mode
             * to something else irrespective of the original mode and expect
             * the result to look as intended and with DRMODE_COMPLEMENT (which
             * means XORing pixels), overdrawing this way will cause odd results
             * if the diacritics and the base char both have common pixels set.
             * So we need to combine the char and its diacritics in a temp
             * buffer using OR, and then draw the final bitmap instead of the
             * chars, without touching the drawmode
             **/
            drawmode = current_vp->drawmode;
            current_vp->drawmode = DRMODE_FG;

            base_ofs = (base_width - width) / 2;
        }

        bits = font_get_bits(pf, *ucs);

#if defined(MAIN_LCD) && defined(HAVE_LCD_COLOR)
        if (pf->depth)
            lcd_alpha_bitmap_part(bits, ofs, 0, width, x + base_ofs, y,
                                  width - ofs, pf->height);
        else
#endif
            LCDFN(mono_bitmap_part)(bits, ofs, 0, width, x + base_ofs,
                                    y, width - ofs, pf->height);
        if (is_diac)
        {
            current_vp->drawmode = drawmode;
        }

        if (next_ch)
        {
            bool next_is_rtl;
            bool next_is_diacritic = is_diacritic(next_ch, &next_is_rtl);

            /* Increment if:
             *  LTR: Next char is not diacritic,
             *  RTL: Current char is non-diacritic and next char is diacritic */
            if ((is_rtl && !is_diac) ||
                    (!is_rtl && (!next_is_diacritic || next_is_rtl)))
            {
                x += base_width - ofs;
                ofs = 0;
            }
        }
    }
    font_lock(current_vp->font, false);
}

/* put a string at a given pixel position */
void LCDFN(putsxy)(int x, int y, const unsigned char *str)
{
    LCDFN(putsxyofs)(x, y, 0, str);
}

/* Formatting version of LCDFN(putsxy) */
void LCDFN(putsxyf)(int x, int y, const unsigned char *fmt, ...)
{
    va_list ap;
    char buf[256];
    va_start(ap, fmt);
    vsnprintf(buf, sizeof (buf), fmt, ap);
    va_end(ap);
    LCDFN(putsxy)(x, y, buf);
}

static void LCDFN(putsxyofs_style)(int xpos, int ypos,
                                   const unsigned char *str, int style,
                                   int h, int offset)
{
    int lastmode = current_vp->drawmode;
    int text_ypos = ypos;
    int line_height = font_get(current_vp->font)->height;
    text_ypos += h/2 - line_height/2; /* center the text in the line */

    if ((style & STYLE_MODE_MASK) == STYLE_NONE) {
        if (str[0])
            LCDFN(putsxyofs)(xpos, text_ypos, offset, str);
        return;
    }
#if defined(MAIN_LCD) && defined(HAVE_LCD_COLOR)
    int oldfgcolor = current_vp->fg_pattern;
    int oldbgcolor = current_vp->bg_pattern;
    current_vp->drawmode = DRMODE_SOLID | ((style & STYLE_INVERT) ?
                                           DRMODE_INVERSEVID : 0);
    if (style & STYLE_COLORED) {
        if (current_vp->drawmode == DRMODE_SOLID)
            current_vp->fg_pattern = style & STYLE_COLOR_MASK;
        else
            current_vp->bg_pattern = style & STYLE_COLOR_MASK;
    }
    current_vp->drawmode ^= DRMODE_INVERSEVID;
    if (style & STYLE_GRADIENT) {
        current_vp->drawmode = DRMODE_FG;
        lcd_do_gradient_line(xpos, current_vp->width, ypos, h,
                          NUMLN_UNPACK(style), CURLN_UNPACK(style));
        current_vp->fg_pattern = current_vp->lst_pattern;
    }
    else if (style & STYLE_COLORBAR) {
        current_vp->drawmode = DRMODE_FG;
        current_vp->fg_pattern = current_vp->lss_pattern;
        lcd_fillrect(xpos, ypos, current_vp->width - xpos, h);
        current_vp->fg_pattern = current_vp->lst_pattern;
    }
    else {
        lcd_fillrect(xpos, ypos, current_vp->width - xpos, h);
        current_vp->drawmode = (style & STYLE_INVERT) ?
        (DRMODE_SOLID|DRMODE_INVERSEVID) : DRMODE_SOLID;
    }
    if (str[0])
        lcd_putsxyofs(xpos, text_ypos, offset, str);
    current_vp->fg_pattern = oldfgcolor;
    current_vp->bg_pattern = oldbgcolor;
#else
    current_vp->drawmode = DRMODE_SOLID | ((style & STYLE_INVERT) ?
                                           0 : DRMODE_INVERSEVID);
    LCDFN(fillrect)(xpos, ypos, current_vp->width - xpos, h);
    current_vp->drawmode ^= DRMODE_INVERSEVID;
    if (str[0])
        LCDFN(putsxyofs)(xpos, text_ypos, offset, str);
#endif
    current_vp->drawmode = lastmode;
}

/*** Line oriented text output ***/

void LCDFN(puts_style_xyoffset)(int x, int y, const unsigned char *str,
                              int style, int x_offset, int y_offset)
{
    int xpos, ypos, h;
    if(!str)
        return;

    h = current_vp->line_height ?: (int)font_get(current_vp->font)->height;
    if ((style&STYLE_XY_PIXELS) == 0)
    {
        xpos = x * LCDFN(getstringsize)(" ", NULL, NULL);
        ypos = y * h;
    }
    else
    {
        xpos = x;
        ypos = y;
    }
    LCDFN(scroll_stop_viewport_rect)(current_vp, x, y, current_vp->width - x, h);
    LCDFN(putsxyofs_style)(xpos, ypos+y_offset, str, style, h, x_offset);
}

void LCDFN(puts_style_offset)(int x, int y, const unsigned char *str,
                              int style, int x_offset)
{
    LCDFN(puts_style_xyoffset)(x, y, str, style, x_offset, 0);
}

void LCDFN(puts)(int x, int y, const unsigned char *str)
{
    LCDFN(puts_style_offset)(x, y, str, STYLE_DEFAULT, 0);
}

/* Formatting version of LCDFN(puts) */
void LCDFN(putsf)(int x, int y, const unsigned char *fmt, ...)
{
    va_list ap;
    char buf[256];
    va_start(ap, fmt);
    vsnprintf(buf, sizeof (buf), fmt, ap);
    va_end(ap);
    LCDFN(puts)(x, y, buf);
}

void LCDFN(puts_style)(int x, int y, const unsigned char *str, int style)
{
    LCDFN(puts_style_offset)(x, y, str, style, 0);
}

/*** scrolling ***/

static struct scrollinfo* find_scrolling_line(int x, int y)
{
    struct scrollinfo* s = NULL;
    int i;

    for(i=0; i<LCDFN(scroll_info).lines; i++)
    {
        s = &LCDFN(scroll_info).scroll[i];
        if (s->x == x && s->y == y && s->vp == current_vp)
            return s;
    }
    return NULL;
}

void LCDFN(scroll_fn)(struct scrollinfo* s)
{
    LCDFN(putsxyofs_style)(s->x, s->y, s->line, s->style, s->height, s->offset);
}

static void LCDFN(puts_scroll_worker)(int x, int y, const unsigned char *string,
                                     int style, int x_offset, int y_offset,
                                     bool linebased,
                                     void (*scroll_func)(struct scrollinfo *),
                                     void *data)
{
    struct scrollinfo* s;
    int width, height;
    int w, h, cwidth, margin;
    bool restart;

    if (!string)
        return;

    /* prepare rectangle for scrolling. x and y must be calculated early
     * for find_scrolling_line() to work */
    cwidth = font_get(current_vp->font)->maxwidth;
    height = current_vp->line_height ?: (int)font_get(current_vp->font)->height;
    y = y * (linebased ? height : 1) + y_offset;
    x = x * (linebased ? cwidth : 1);
    width = current_vp->width - x;

    if (y >= current_vp->height)
        return;

    s = find_scrolling_line(x, y);
    restart = !s;

    if (restart) {
        /* remove any previously scrolling line at the same location */
        LCDFN(scroll_stop_viewport_rect)(current_vp, x, y, width, height);
        LCDFN(putsxyofs_style)(x, y, string, style, height, x_offset);

        if (LCDFN(scroll_info).lines >= LCDM(SCROLLABLE_LINES))
            return;
    }

    /* get width (pixeks) of the string */
    LCDFN(getstringsize)(string, &w, &h);

    /* check if scrolling is actually necessary (consider the actual start
     * of the line) */
    margin = x * linebased ? cwidth : 1;
    if (current_vp->width >= margin+w)
        return;

    if (restart) {
        /* prepare scroll line */
        s = &LCDFN(scroll_info).scroll[LCDFN(scroll_info).lines];
        s->start_tick = current_tick + LCDFN(scroll_info).delay;
    }

    /* copy contents to the line buffer */
    strlcpy(s->linebuffer, string, sizeof(s->linebuffer));
    /* scroll bidirectional or forward only depending on the string width */
    if ( LCDFN(scroll_info).bidir_limit ) {
        s->bidir = w < (current_vp->width) *
            (100 + LCDFN(scroll_info).bidir_limit) / 100;
    }
    else
        s->bidir = false;

    s->scroll_func = scroll_func;
    s->userdata = data;

    if (restart) {
        s->offset = x_offset;
        s->backward = false;
        s->style = style;
        /* assign the rectangle. not necessary if continuing an earlier line */
        s->x = x;
        s->y = y;
        s->width = width;
        s->height = height;
        s->vp = current_vp;
        LCDFN(scroll_info).lines++;
    }
}

void LCDFN(puts_scroll_style_xyoffset)(int x, int y, const unsigned char *string,
                                     int style, int x_offset, int y_offset)
{
    LCDFN(puts_scroll_worker)(x, y, string, style, x_offset, y_offset,
                              true, LCDFN(scroll_fn), NULL);
}

void LCDFN(putsxy_scroll_func)(int x, int y, const unsigned char *string,
                                     void (*scroll_func)(struct scrollinfo *),
                                     void *data, int x_offset)
{
    if (!scroll_func)
        LCDFN(putsxyofs)(x, y, x_offset, string);
    else
        LCDFN(puts_scroll_worker)(x, y, string, STYLE_NONE, x_offset, 0,
                              false, scroll_func, data);
}

void LCDFN(puts_scroll)(int x, int y, const unsigned char *string)
{
    LCDFN(puts_scroll_style)(x, y, string, STYLE_DEFAULT);
}

void LCDFN(puts_scroll_style)(int x, int y, const unsigned char *string,
                              int style)
{
     LCDFN(puts_scroll_style_xyoffset)(x, y, string, style, 0, 0);
}

#if !defined(HAVE_LCD_COLOR) || !defined(MAIN_LCD)
/* see lcd-16bit-common.c for others */
#ifdef MAIN_LCD
#define THIS_STRIDE STRIDE_MAIN
#else
#define THIS_STRIDE STRIDE_REMOTE
#endif

void LCDFN(bmp_part)(const struct bitmap* bm, int src_x, int src_y,
                                int x, int y, int width, int height)
{
#if LCDM(DEPTH) > 1
    if (bm->format != FORMAT_MONO)
        LCDFN(bitmap_part)((FBFN(data)*)(bm->data),
            src_x, src_y, THIS_STRIDE(bm->width, bm->height), x, y, width, height);
    else
#endif
        LCDFN(mono_bitmap_part)(bm->data,
            src_x, src_y, THIS_STRIDE(bm->width, bm->height), x, y, width, height);
}

void LCDFN(bmp)(const struct bitmap* bm, int x, int y)
{
    LCDFN(bmp_part)(bm, 0, 0, x, y, bm->width, bm->height);
}

#endif

void LCDFN(nine_segment_bmp)(const struct bitmap* bm, int x, int y,
                                int width, int height)
{
    int seg_w = bm->width / 3;
    int seg_h = bm->height / 3;
    int src_x, src_y, dst_x, dst_y;

    /* top */
    src_x = seg_w; src_y = 0;
    dst_x = seg_w; dst_y = 0;
    for (; dst_x < width - seg_w; dst_x += seg_w)
        LCDFN(bmp_part)(bm, src_x, src_y, dst_x, dst_y, seg_w, seg_h);
    /* bottom */
    src_x = seg_w; src_y = bm->height - seg_h;
    dst_x = seg_w; dst_y = height - seg_h;
    for (; dst_x < width - seg_w; dst_x += seg_w)
        LCDFN(bmp_part)(bm, src_x, src_y, dst_x, dst_y, seg_w, seg_h);
        
    /* left */
    src_x = 0; src_y = seg_h;
    dst_x = 0; dst_y = seg_h;
    for (; dst_y < height - seg_h; dst_y += seg_h)
        LCDFN(bmp_part)(bm, src_x, src_y, dst_x, dst_y, seg_w, seg_h);
    /* right */
    src_x = bm->width - seg_w; src_y = seg_h;
    dst_x = width - seg_w; dst_y = seg_h;
    for (; dst_y < height - seg_h; dst_y += seg_h)
        LCDFN(bmp_part)(bm, src_x, src_y, dst_x, dst_y, seg_w, seg_h);
    /* center */
    dst_y = seg_h; src_y = seg_h; src_x = seg_w;
    for (; dst_y < height - seg_h; dst_y += seg_h)
    {
        dst_x = seg_w;
        for (; dst_x < width - seg_w; dst_x += seg_w)
            LCDFN(bmp_part)(bm, src_x, src_y, dst_x, dst_y, seg_w, seg_h);
    }

    /* 4 corners */
    LCDFN(bmp_part)(bm, 0, 0, x, y, seg_w, seg_h);
    LCDFN(bmp_part)(bm, bm->width - seg_w, 0, width - seg_w, 0, seg_w, seg_h);
    LCDFN(bmp_part)(bm, 0, bm->width - seg_h, 0, height - seg_h, seg_w, seg_h);
    LCDFN(bmp_part)(bm, bm->width - seg_w, bm->width - seg_h,
            width - seg_w, height - seg_h, seg_w, seg_h);
}
