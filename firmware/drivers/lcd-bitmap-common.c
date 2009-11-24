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
#include "stdarg.h"
#include "sprintf.h"
#include "diacritic.h"

#ifndef LCDFN /* Not compiling for remote - define macros for main LCD. */
#define LCDFN(fn) lcd_ ## fn
#define FBFN(fn)  fb_ ## fn
#define LCDM(ma) LCD_ ## ma
#define LCDNAME "lcd_"
#define MAIN_LCD
#endif

#if defined(MAIN_LCD) && defined(HAVE_LCD_COLOR)
/* Fill a rectangle with a gradient */
static void lcd_gradient_rect(int x1, int x2, int y, unsigned h,
                              int num_lines, int cur_line)
{
    int old_pattern = current_vp->fg_pattern;
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
    h_r = (h_r << 16) + (1 << 15);
    h_g = (h_g << 16) + (1 << 15);
    h_b = (h_b << 16) + (1 << 15);
    if (cur_line)
    {
        h_r -= cur_line * rstep;
        h_g -= cur_line * gstep;
        h_b -= cur_line * bstep;
    }
    unsigned     count;

    for(count = 0; count < h; count++) {
        current_vp->fg_pattern = LCD_RGBPACK(h_r >> 16, h_g >> 16, h_b >> 16);
        lcd_hline(x1, x2, y + count);
        h_r -= rstep;
        h_g -= gstep;
        h_b -= bstep;
    }

    current_vp->fg_pattern = old_pattern;
}
#endif

struct lcd_bitmap_char
{
    char is_rtl;
    char is_diacritic;
    unsigned char width;
    unsigned char base_width;
};

/* put a string at a given pixel position, skipping first ofs pixel columns */
static void LCDFN(putsxyofs)(int x, int y, int ofs, const unsigned char *str)
{
    unsigned short *ucs;
    struct font* pf = font_get(current_vp->font);
    int vp_flags = current_vp->flags;
    int i, len;
    static struct lcd_bitmap_char chars[SCROLL_LINE_SIZE];

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

    ucs = bidi_l2v(str, 1);
    /* Mark diacritic and rtl flags for each character */
    for (i = 0; i < SCROLL_LINE_SIZE && ucs[i]; i++)
    {
        bool is_rtl;
        chars[i].is_diacritic = is_diacritic(ucs[i], &is_rtl);
        chars[i].is_rtl=is_rtl;
    }
    len = i;

    /* Get proportional width and glyph bits */
    for (i = 0; i < len; i++)
        chars[i].width = font_get_width(pf, ucs[i]);

    /* Calculate base width for each character */
    for (i = 0; i < len; i++)
    {
        if (chars[i].is_rtl)
        {
            /* Forward-seek the next non-diacritic character for base width */
            if (chars[i].is_diacritic)
            {
                int j;

                /* Jump to next non-diacritic character, and calc its width */
                for (j = i; j < len && chars[j].is_diacritic; j++);

                /* Set all related diacritic char's base-width accordingly */
                for (; i <= j; i++)
                    chars[i].base_width = chars[j].width;
            }
            else
            {
                chars[i].base_width = chars[i].width;
            }
        }
        else
        {
            static int last_non_diacritic_width = 0;

            if (!chars[i].is_diacritic)
                last_non_diacritic_width = chars[i].width;

            chars[i].base_width = last_non_diacritic_width;
        }
    }

    for (i = 0; i < len; i++)
    {
        const unsigned char *bits;
        unsigned short ch = ucs[i];
        int width = chars[i].width;
        int drawmode = 0, base_ofs = 0;
        bool next_is_diacritic;

        if (x >= current_vp->width)
            break;

        if (ofs > width)
        {
            ofs -= width;
            continue;
        }

        if (chars[i].is_diacritic)
        {
            drawmode = current_vp->drawmode;
            current_vp->drawmode = DRMODE_FG;

            base_ofs = (chars[i].base_width - width) / 2;
        }

        bits = font_get_bits(pf, ch);
        LCDFN(mono_bitmap_part)(bits, ofs, 0, width,
            x + base_ofs, y, width - ofs, pf->height);

        if (chars[i].is_diacritic)
        {
            current_vp->drawmode = drawmode;
        }

        /* Increment if next char is not diacritic (non-rtl),
         * or current char is non-diacritic and next char is diacritic (rtl)*/
        next_is_diacritic = (ucs[i + 1] && chars[i + 1].is_diacritic);

        if (ucs[i + 1] &&
            ((chars[i].is_rtl && !chars[i].is_diacritic) ||
            (!chars[i].is_rtl && (chars[i + 1].is_rtl || !chars[i + 1].is_diacritic))))
        {
            x += chars[i].base_width - ofs;
            ofs = 0;
        }
    }
}
/* put a string at a given pixel position */
void LCDFN(putsxy)(int x, int y, const unsigned char *str)
{
    LCDFN(putsxyofs)(x, y, 0, str);
}

static void LCDFN(putsxyofs_style)(int xpos, int ypos,
                                   const unsigned char *str, int style,
                                   int w, int h, int offset)
{
    int lastmode = current_vp->drawmode;
    int xrect = xpos + MAX(w - offset, 0);
    int x = VP_IS_RTL(current_vp) ? xpos : xrect;
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
        lcd_gradient_rect(xpos, current_vp->width, ypos, h,
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
        lcd_fillrect(x, ypos, current_vp->width - xrect, h);
        current_vp->drawmode = (style & STYLE_INVERT) ?
        (DRMODE_SOLID|DRMODE_INVERSEVID) : DRMODE_SOLID;
    }
    lcd_putsxyofs(xpos, ypos, offset, str);
    current_vp->fg_pattern = oldfgcolor;
    current_vp->bg_pattern = oldbgcolor;
#else
    current_vp->drawmode = DRMODE_SOLID | ((style & STYLE_INVERT) ?
                                           0 : DRMODE_INVERSEVID);
    LCDFN(fillrect)(x, ypos, current_vp->width - xrect, h);
    current_vp->drawmode ^= DRMODE_INVERSEVID;
    LCDFN(putsxyofs)(xpos, ypos, offset, str);
#endif
    current_vp->drawmode = lastmode;
}

/*** Line oriented text output ***/

/* put a string at a given char position */
void LCDFN(puts_style_offset)(int x, int y, const unsigned char *str,
                              int style, int offset)
{
    int xpos, ypos, w, h;
    LCDFN(scroll_stop_line)(current_vp, y);
    if(!str || !str[0])
        return;

    LCDFN(getstringsize)(str, &w, &h);
    xpos = x * LCDFN(getstringsize)(" ", NULL, NULL);
    ypos = y * h;
    LCDFN(putsxyofs_style)(xpos, ypos, str, style, w, h, offset);
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

void LCDFN(puts_scroll_style_offset)(int x, int y, const unsigned char *string,
                                     int style, int offset)
{
    struct scrollinfo* s;
    char *end;
    int w, h;

    if ((unsigned)y >= (unsigned)current_vp->height)
        return;

    /* remove any previously scrolling line at the same location */
    lcd_scroll_stop_line(current_vp, y);

    if (LCDFN(scroll_info).lines >= LCDM(SCROLLABLE_LINES)) return;
    if (!string)
        return;
    LCDFN(puts_style_offset)(x, y, string, style, offset);

    LCDFN(getstringsize)(string, &w, &h);

    if (current_vp->width - x * 8 >= w)
        return;

    /* prepare scroll line */
    s = &LCDFN(scroll_info).scroll[LCDFN(scroll_info).lines];
    s->start_tick = current_tick + LCDFN(scroll_info).delay;
    s->style = style;

    strlcpy(s->line, string, sizeof s->line);

    /* get width */
    s->width = LCDFN(getstringsize)(s->line, &w, &h);

    /* scroll bidirectional or forward only depending on the string
       width */
    if ( LCDFN(scroll_info).bidir_limit ) {
        s->bidir = s->width < (current_vp->width) *
            (100 + LCDFN(scroll_info).bidir_limit) / 100;
    }
    else
        s->bidir = false;

    if (!s->bidir) { /* add spaces if scrolling in the round */
        strcat(s->line, "   ");
        /* get new width incl. spaces */
        s->width = LCDFN(getstringsize)(s->line, &w, &h);
    }

    end = strchr(s->line, '\0');
    strlcpy(end, string, current_vp->width/2);

    s->vp = current_vp;
    s->y = y;
    s->len = utf8length(string);
    s->offset = offset;
    s->startx = x * LCDFN(getstringsize)(" ", NULL, NULL);
    s->backward = false;

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
    struct font* pf;
    struct scrollinfo* s;
    int index;
    int xpos, ypos;
    struct viewport* old_vp = current_vp;

    for ( index = 0; index < LCDFN(scroll_info).lines; index++ ) {
        s = &LCDFN(scroll_info).scroll[index];

        /* check pause */
        if (TIME_BEFORE(current_tick, s->start_tick))
            continue;

        LCDFN(set_viewport)(s->vp);

        if (s->backward)
            s->offset -= LCDFN(scroll_info).step;
        else
            s->offset += LCDFN(scroll_info).step;

        pf = font_get(current_vp->font);
        xpos = s->startx;
        ypos = s->y * pf->height;

        if (s->bidir) { /* scroll bidirectional */
            if (s->offset <= 0) {
                /* at beginning of line */
                s->offset = 0;
                s->backward = false;
                s->start_tick = current_tick + LCDFN(scroll_info).delay * 2;
            }
            if (s->offset >= s->width - (current_vp->width - xpos)) {
                /* at end of line */
                s->offset = s->width - (current_vp->width - xpos);
                s->backward = true;
                s->start_tick = current_tick + LCDFN(scroll_info).delay * 2;
            }
        }
        else {
            /* scroll forward the whole time */
            if (s->offset >= s->width)
                s->offset %= s->width;
        }
        LCDFN(putsxyofs_style)(xpos, ypos, s->line, s->style, s->width,
                               pf->height, s->offset);
        LCDFN(update_viewport_rect)(xpos, ypos, current_vp->width - xpos,
                                    pf->height);
    }
    LCDFN(set_viewport)(old_vp);
}
