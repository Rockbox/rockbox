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
void lcd_gradient_fillrect(int x, int y, int width, int height,
        unsigned start_rgb, unsigned end_rgb)
{
    int old_pattern = current_vp->fg_pattern;
    int step_mul, i;
    int x1, x2;
    x1 = x;
    x2 = x + width;
    
    if (height == 0) return;

    step_mul = (1 << 16) / height;
    int h_r = RGB_UNPACK_RED(start_rgb);
    int h_g = RGB_UNPACK_GREEN(start_rgb);
    int h_b = RGB_UNPACK_BLUE(start_rgb);
    int rstep = (h_r - RGB_UNPACK_RED(end_rgb)) * step_mul;
    int gstep = (h_g - RGB_UNPACK_GREEN(end_rgb)) * step_mul;
    int bstep = (h_b - RGB_UNPACK_BLUE(end_rgb)) * step_mul;
    h_r = (h_r << 16) + (1 << 15);
    h_g = (h_g << 16) + (1 << 15);
    h_b = (h_b << 16) + (1 << 15);

    for(i = y; i < y + height; i++) {
        current_vp->fg_pattern = LCD_RGBPACK(h_r >> 16, h_g >> 16, h_b >> 16);
        lcd_hline(x1, x2, i);
        h_r -= rstep;
        h_g -= gstep;
        h_b -= bstep;
    }

    current_vp->fg_pattern = old_pattern;
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

/* put a string at a given char position */
void LCDFN(puts_style_xyoffset)(int x, int y, const unsigned char *str,
                              int style, int x_offset, int y_offset)
{
    int xpos, ypos, h;
    LCDFN(scroll_stop_line)(current_vp, y);
    if(!str)
        return;

    h = current_vp->line_height ?: (int)font_get(current_vp->font)->height;
    if ((style&STYLE_XY_PIXELS) == 0)
    {
        xpos = x * LCDFN(getstringsize)(" ", NULL, NULL);
        ypos = y * h + y_offset;
    }
    else
    {
        xpos = x;
        ypos = y + y_offset;
    }
    LCDFN(putsxyofs_style)(xpos, ypos, str, style, h, x_offset);
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

void LCDFN(puts_offset)(int x, int y, const unsigned char *str, int offset)
{
    LCDFN(puts_style_offset)(x, y, str, STYLE_DEFAULT, offset);
}

/*** scrolling ***/

static struct scrollinfo* find_scrolling_line(int line)
{
    struct scrollinfo* s = NULL;
    int i;

    for(i=0; i<LCDFN(scroll_info).lines; i++)
    {
        s = &LCDFN(scroll_info).scroll[i];
        if (s->y == line && s->vp == current_vp)
            return s;
    }
    return NULL;
}

void LCDFN(puts_scroll_style_xyoffset)(int x, int y, const unsigned char *string,
                                     int style, int x_offset, int y_offset)
{
    struct scrollinfo* s;
    char *end;
    int w, h;
    int len;
    bool restart = false;
    int space_width;

    if (!string || ((unsigned)y >= (unsigned)current_vp->height))
        return;

    s = find_scrolling_line(y);
    if (!s)
        restart = true;

    if (restart)
    {
        /* remove any previously scrolling line at the same location */
        LCDFN(scroll_stop_line)(current_vp, y);

        if (LCDFN(scroll_info).lines >= LCDM(SCROLLABLE_LINES)) return;
        LCDFN(puts_style_xyoffset)(x, y, string, style, x_offset, y_offset);
    }

    LCDFN(getstringsize)(string, &w, &h);

    if (current_vp->width - x * 8 >= w)
        return;

    if (restart)
    {
        /* prepare scroll line */
        s = &LCDFN(scroll_info).scroll[LCDFN(scroll_info).lines];
        s->start_tick = current_tick + LCDFN(scroll_info).delay;
    }
    strlcpy(s->line, string, sizeof s->line);
    space_width = LCDFN(getstringsize)(" ", NULL, NULL);

    /* get width */
    LCDFN(getstringsize)(s->line, &w, &h);
    if (!restart && s->width > w)
    {
        if (s->startx > w)
            s->startx = w;
    }
    s->width = w;

    /* scroll bidirectional or forward only depending on the string
       width */
    if ( LCDFN(scroll_info).bidir_limit ) {
        s->bidir = s->width < (current_vp->width) *
            (100 + LCDFN(scroll_info).bidir_limit) / 100;
    }
    else
        s->bidir = false;

    if (!s->bidir) { /* add spaces if scrolling in the round */
        strlcat(s->line, "   ", sizeof s->line);
        /* get new width incl. spaces */
        s->width += space_width * 3;
    }

    end = strchr(s->line, '\0');
    len = sizeof s->line - (end - s->line);
    strlcpy(end, string, MIN(current_vp->width/2, len));

    s->vp = current_vp;
    s->y = y;
    if (restart)
    {
        s->offset = x_offset;
        s->startx = x * space_width;
        s->backward = false;
        s->style = style;
    }
    s->y_offset = y_offset;

    if (restart)
        LCDFN(scroll_info).lines++;
}

void LCDFN(puts_scroll)(int x, int y, const unsigned char *string)
{
    LCDFN(puts_scroll_style)(x, y, string, STYLE_DEFAULT);
}

void LCDFN(puts_scroll_style)(int x, int y, const unsigned char *string,
                              int style)
{
     LCDFN(puts_scroll_style_offset)(x, y, string, style, 0);
}

void LCDFN(puts_scroll_offset)(int x, int y, const unsigned char *string,
                               int offset)
{
     LCDFN(puts_scroll_style_offset)(x, y, string, STYLE_DEFAULT, offset);
}

void LCDFN(scroll_fn)(void)
{
    struct scrollinfo* s;
    int index;
    int xpos, ypos, height;
    struct viewport* old_vp = current_vp;
    bool makedelay;

    for ( index = 0; index < LCDFN(scroll_info).lines; index++ ) {
        s = &LCDFN(scroll_info).scroll[index];

        /* check pause */
        if (TIME_BEFORE(current_tick, s->start_tick))
            continue;

        LCDFN(set_viewport)(s->vp);
        height = s->vp->line_height ?: (int)font_get(s->vp->font)->height;

        if (s->backward)
            s->offset -= LCDFN(scroll_info).step;
        else
            s->offset += LCDFN(scroll_info).step;

        xpos = s->startx;
        ypos = s->y * height + s->y_offset;

        makedelay = false;
        if (s->bidir) { /* scroll bidirectional */
            if (s->offset <= 0) {
                /* at beginning of line */
                s->offset = 0;
                s->backward = false;
                makedelay = true;
            }
            else if (s->offset >= s->width - (current_vp->width - xpos)) {
                /* at end of line */
                s->offset = s->width - (current_vp->width - xpos);
                s->backward = true;
                makedelay = true;
            }
        }
        else {
            /* scroll forward the whole time */
            if (s->offset >= s->width) {
                s->offset = 0;
                makedelay = true;
            }
        }

        if (makedelay)
            s->start_tick = current_tick + LCDFN(scroll_info).delay +
                    LCDFN(scroll_info).ticks;

        LCDFN(putsxyofs_style)(xpos, ypos, s->line, s->style, height, s->offset);
        LCDFN(update_viewport_rect)(xpos, ypos, current_vp->width-xpos, height);
    }
    LCDFN(set_viewport)(old_vp);
}

void LCDFN(puts_scroll_style_offset)(int x, int y, const unsigned char *string,
                                     int style, int x_offset)
{
    LCDFN(puts_scroll_style_xyoffset)(x, y, string, style, x_offset, 0);
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
