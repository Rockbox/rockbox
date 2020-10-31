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

#ifdef LOGF_ENABLE
#include "panic.h"
#endif

#ifndef LCDFN /* Not compiling for remote - define macros for main LCD. */
#define LCDFN(fn) lcd_ ## fn
#define FBFN(fn)  fb_ ## fn
#define LCDM(ma) LCD_ ## ma
#define LCDNAME "lcd_"
#define MAIN_LCD
#endif

#ifdef MAIN_LCD
#define THIS_STRIDE STRIDE_MAIN
#else
#define THIS_STRIDE STRIDE_REMOTE
#endif

extern void viewport_set_buffer(struct viewport *vp,
                                struct frame_buffer_t *buffer,
                                const enum screen_type screen); /* viewport.c */
/*
 * draws the borders of the current viewport
 **/
void LCDFN(draw_border_viewport)(void)
{
    LCDFN(drawrect)(0, 0, LCDFN(current_viewport)->width, LCDFN(current_viewport)->height);
}

/*
 * fills the rectangle formed by LCDFN(current_viewport)
 **/
void LCDFN(fill_viewport)(void)
{
    LCDFN(fillrect)(0, 0, LCDFN(current_viewport)->width, LCDFN(current_viewport)->height);
}


/*** Viewports ***/
/* init_viewport Notes: When a viewport is initialized
 * if vp->buffer is NULL the default frame_buffer is assigned
 * likewise the actual buffer, stride, get_address_fn
 * are all filled with values from the default buffer if they are not set
 * RETURNS either the viewport you passed or the default viewport if vp == NULL
 */
struct viewport* LCDFN(init_viewport)(struct viewport* vp)
{
    struct frame_buffer_t *fb_default = &LCDFN(framebuffer_default);
    if (!vp) /* NULL vp grabs default viewport */
    {
        vp = &default_vp;
        vp->buffer = fb_default;
    }

    /* use defaults if no buffer is provided */
    if (vp->buffer == NULL || vp->buffer->elems == 0)
        vp->buffer = fb_default;
    else
    {
        if (vp->buffer->stride == 0)
            vp->buffer->stride = fb_default->stride;

        if (vp->buffer->data == NULL)
            vp->buffer->data = fb_default->data;

        if (vp->buffer->get_address_fn == NULL)
            vp->buffer->get_address_fn = fb_default->get_address_fn;

#ifdef LOGF_ENABLE
        if ((size_t)LCD_NBELEMS(vp->width, vp->height) > vp->buffer->elems)
        {
            if (vp->buffer != fb_default)
                panicf("viewport %d x %d > buffer", vp->width, vp->height);
            logf("viewport %d x %d, %d x %d [%lu] > buffer [%lu]", vp->x, vp->y,
                 vp->width, vp->height,
                 (unsigned long) LCD_NBELEMS(vp->width, vp->height),
                 (unsigned long) vp->buffer->elems);

        }
#endif
    }
    return vp;
}
struct viewport* LCDFN(set_viewport_ex)(struct viewport* vp, int flags)
{
    vp = LCDFN(init_viewport)(vp);
    struct viewport* last_vp = LCDFN(current_viewport);
    LCDFN(current_viewport) = vp;

#if LCDM(DEPTH) > 1
    LCDFN(set_foreground)(vp->fg_pattern);
    LCDFN(set_background)(vp->bg_pattern);
#endif

#if defined(SIMULATOR)
    /* Force the viewport to be within bounds.  If this happens it should
     *  be considered an error - the viewport will not draw as it might be
     *  expected.
     */

    if((unsigned) vp->x > (unsigned) LCDM(WIDTH)
        || (unsigned) vp->y > (unsigned) LCDM(HEIGHT)
        || vp->x + vp->width > LCDM(WIDTH)
        || vp->y + vp->height > LCDM(HEIGHT))
    {
#if !defined(HAVE_VIEWPORT_CLIP)
        DEBUGF("ERROR: "
#else
        DEBUGF("NOTE: "
#endif
            "set_viewport out of bounds: x: %d y: %d width: %d height:%d\n",
            vp->x, vp->y, vp->width, vp->height);
    }
#endif
    if(last_vp)
    {
        if ((flags & VP_FLAG_CLEAR_FLAG) == VP_FLAG_CLEAR_FLAG)
            last_vp->flags &= ~flags;
        else
            last_vp->flags |= flags;
    }

    return last_vp;
}

struct viewport* LCDFN(set_viewport)(struct viewport* vp)
{
    return LCDFN(set_viewport_ex)(vp, VP_FLAG_VP_DIRTY);
}

struct viewport *LCDFN(get_viewport)(bool *is_default)
{
#if 0
    *is_default = memcmp(LCDFN(current_viewport),
                         &default_vp, sizeof(struct viewport)) == 0;
#else
    *is_default = LCDFN(current_viewport) == &default_vp;
#endif

    return LCDFN(current_viewport);
}

void LCDFN(update_viewport)(void)
{
    struct viewport* vp = LCDFN(current_viewport);
    if (vp->buffer->stride != LCDFN(framebuffer_default.stride))
    {
        LCDFN(update_viewport_rect)(0,0, vp->width, vp->height);
        return;
    }
    LCDFN(update_rect)(vp->x, vp->y, vp->width, vp->height);
}

void LCDFN(update_viewport_rect)(int x, int y, int width, int height)
{
    struct viewport* vp = LCDFN(current_viewport);

    /* handle the case of viewport with differing stride from main screen */
    if (vp->buffer->stride != LCDFN(framebuffer_default.stride))
    {
        struct frame_buffer_t *fb = vp->buffer;
        viewport_set_buffer(vp, NULL, 0);

        LCDFN(bitmap_part)
             (fb->FBFN(ptr), vp->x, vp->y, fb->stride,
              vp->x + x, vp->y + y, width, height);

        LCDFN(update_rect)(vp->x + x, vp->y + y, width, height);
        viewport_set_buffer(vp, fb, 0);
        return;
    }

    LCDFN(update_rect)(vp->x + x, vp->y + y, width, height);
}

#ifndef BOOTLOADER
/* put a string at a given pixel position, skipping first ofs pixel columns */
static void LCDFN(putsxyofs)(int x, int y, int ofs, const unsigned char *str)
{
    unsigned short *ucs;
    font_lock(LCDFN(current_viewport)->font, true);
    struct font* pf = font_get(LCDFN(current_viewport)->font);
    int vp_flags = LCDFN(current_viewport)->flags;
    int rtl_next_non_diac_width, last_non_diacritic_width;

    if ((vp_flags & VP_FLAG_ALIGNMENT_MASK) != 0)
    {
        int w;

        LCDFN(getstringsize)(str, &w, NULL);
        /* center takes precedence */
        if (vp_flags & VP_FLAG_ALIGN_CENTER)
        {
            x = ((LCDFN(current_viewport)->width - w)/ 2) + x;
            if (x < 0)
                x = 0;
        }
        else
        {
            x = LCDFN(current_viewport)->width - w - x;
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

        if (x >= LCDFN(current_viewport)->width)
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
            drawmode = LCDFN(current_viewport)->drawmode;
            LCDFN(current_viewport)->drawmode = DRMODE_FG;

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
            LCDFN(current_viewport)->drawmode = drawmode;
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
    font_lock(LCDFN(current_viewport)->font, false);
}
#else /* BOOTLOADER */
/* put a string at a given pixel position, skipping first ofs pixel columns */
static void LCDFN(putsxyofs)(int x, int y, int ofs, const unsigned char *str)
{
    unsigned short *ucs;
    struct font* pf = font_get(LCDFN(current_viewport)->font);
    int vp_flags = LCDFN(current_viewport)->flags;
    const unsigned char *bits;
    int width;

    if ((vp_flags & VP_FLAG_ALIGNMENT_MASK) != 0)
    {
        int w;

        LCDFN(getstringsize)(str, &w, NULL);
        /* center takes precedence */
        if (vp_flags & VP_FLAG_ALIGN_CENTER)
        {
            x = ((LCDFN(current_viewport)->width - w)/ 2) + x;
            if (x < 0)
                x = 0;
        }
        else
        {
            x = LCDFN(current_viewport)->width - w - x;
            x += ofs;
            ofs = 0;
        }
    }

    /* allow utf but no diacritics or rtl lang */
    for (ucs = bidi_l2v(str, 1); *ucs; ucs++)
    {
        const unsigned short next_ch = ucs[1];

        if (x >= LCDFN(current_viewport)->width)
            break;

        /* Get proportional width and glyph bits */
        width = font_get_width(pf, *ucs);

        if (ofs > width)
        {
            ofs -= width;
            continue;
        }

        bits = font_get_bits(pf, *ucs);

#if defined(MAIN_LCD) && defined(HAVE_LCD_COLOR)
        if (pf->depth)
            lcd_alpha_bitmap_part(bits, ofs, 0, width, x, y,
                                  width - ofs, pf->height);
        else
#endif
            LCDFN(mono_bitmap_part)(bits, ofs, 0, width, x,
                                    y, width - ofs, pf->height);
        if (next_ch)
        {
            x += width - ofs;
            ofs = 0;
        }
    }
}
#endif


/*** pixel oriented text output ***/

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

/*** Line oriented text output ***/

void LCDFN(puts)(int x, int y, const unsigned char *str)
{
    int h;
    x *= LCDFN(getstringsize)(" ", NULL, &h);
    y *= h;
    LCDFN(putsxyofs)(x, y, 0, str);
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

/*** scrolling ***/

static struct scrollinfo* find_scrolling_line(int x, int y)
{
    struct scrollinfo* s = NULL;
    int i;

    for(i=0; i<LCDFN(scroll_info).lines; i++)
    {
        s = &LCDFN(scroll_info).scroll[i];
        if (s->x == x && s->y == y && s->vp == LCDFN(current_viewport))
            return s;
    }
    return NULL;
}

void LCDFN(scroll_fn)(struct scrollinfo* s)
{
    /* with line == NULL when scrolling stops. This scroller
     * maintains no userdata so there is nothing left to do */
    if (!s->line)
        return;
    /* Fill with background/backdrop to clear area.
     * cannot use clear_viewport_rect() since would stop scrolling as well */
    LCDFN(set_drawmode)(DRMODE_SOLID|DRMODE_INVERSEVID);
    LCDFN(fillrect)(s->x, s->y, s->width, s->height);
    LCDFN(set_drawmode)(DRMODE_SOLID);
    LCDFN(putsxyofs)(s->x, s->y, s->offset, s->line);
}

static bool LCDFN(puts_scroll_worker)(int x, int y, const unsigned char *string,
                                     int x_offset,
                                     bool linebased,
                                     void (*scroll_func)(struct scrollinfo *),
                                     void *data)
{
    struct scrollinfo* s;
    int width, height;
    int w, h, cwidth;
    bool restart;
    struct viewport * vp = LCDFN(current_viewport);

    if (!string)
        return false;

    /* prepare rectangle for scrolling. x and y must be calculated early
     * for find_scrolling_line() to work */

    cwidth = font_get(vp->font)->maxwidth;
    /* get width (pixels) of the string */
    LCDFN(getstringsize)(string, &w, &h);
    height = h;

    y = y * (linebased ? height : 1);
    x = x * (linebased ? cwidth : 1);
    width = vp->width - x;

    if (y >= vp->height || (height + y) > (vp->height))
        return false;

    s = find_scrolling_line(x, y);
    restart = !s;

    /* Remove any previously scrolling line at the same location. If
     * the string width is too small to scroll the scrolling line is
     * cleared as well */
    if (w < width || restart) {
        LCDFN(scroll_stop_viewport_rect)(vp, x, y, width, height);
        LCDFN(putsxyofs)(x, y, x_offset, string);
        /* nothing to scroll, or out of scrolling lines. Either way, get out */
        if (w < width || LCDFN(scroll_info).lines >= LCDM(SCROLLABLE_LINES))
            return false;
        /* else restarting: prepare scroll line */
        s = &LCDFN(scroll_info).scroll[LCDFN(scroll_info).lines];
    }

    /* copy contents to the line buffer */
    strlcpy(s->linebuffer, string, sizeof(s->linebuffer));
    /* scroll bidirectional or forward only depending on the string width */
    if ( LCDFN(scroll_info).bidir_limit ) {
        s->bidir = w < (vp->width) *
            (100 + LCDFN(scroll_info).bidir_limit) / 100;
    }
    else
        s->bidir = false;

    if (restart) {
        s->offset = x_offset;
        s->backward = false;
        /* assign the rectangle. not necessary if continuing an earlier line */
        s->x = x;
        s->y = y;
        s->width = width;
        s->height = height;
        s->vp = vp;
        s->start_tick = current_tick + LCDFN(scroll_info).delay;
        LCDFN(scroll_info).lines++;
    } else {
        /* not restarting, however we are about to assign new userdata;
         * therefore tell the scroller that it can release the previous userdata */
        s->line = NULL;
        s->scroll_func(s);
    }

    s->scroll_func = scroll_func;
    s->userdata = data;

    /* if only the text was updated render immediately */
    if (!restart)
        LCDFN(scroll_now(s));

    return true;
}

bool LCDFN(putsxy_scroll_func)(int x, int y, const unsigned char *string,
                                     void (*scroll_func)(struct scrollinfo *),
                                     void *data, int x_offset)
{
    bool retval = false;
    if (!scroll_func)
        LCDFN(putsxyofs)(x, y, x_offset, string);
    else
        retval = LCDFN(puts_scroll_worker)(x, y, string, x_offset, false, scroll_func, data);

    return retval;
}

bool LCDFN(puts_scroll)(int x, int y, const unsigned char *string)
{
    return LCDFN(puts_scroll_worker)(x, y, string, 0, true, LCDFN(scroll_fn), NULL);
}

#if !defined(HAVE_LCD_COLOR) || !defined(MAIN_LCD)
/* see lcd-16bit-common.c for others */

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
    int seg_w, seg_h, src_x, src_y, dst_x, dst_y;
    /* if the bitmap dimensions are not multiples of 3 bytes reduce the
     * inner segments accordingly. A 8x8 image becomes 3x3 for each
     * corner, and 2x2 for the inner segments */
    int corner_w = (bm->width + 2) / 3;
    int corner_h = (bm->height + 2) / 3;
    seg_w = bm->width  - (2 * corner_w);
    seg_h = bm->height - (2 * corner_h);

    /* top & bottom in a single loop*/
    src_x = corner_w;
    dst_x = corner_w;
    int src_y_top = 0;
    int dst_y_top = 0;
    int src_y_btm = bm->height - corner_h;
    int dst_y_btm = height - corner_h;
    for (; dst_x < width - seg_w; dst_x += seg_w)
    {
        /* cap the last segment to the remaining width */
        int w = MIN(seg_w, (width - dst_x - seg_w));
        LCDFN(bmp_part)(bm, src_x, src_y_top, dst_x, dst_y_top, w, seg_h);
        LCDFN(bmp_part)(bm, src_x, src_y_btm, dst_x, dst_y_btm, w, seg_h);
    }

    /* left & right in a single loop */
    src_y = corner_h;
    dst_y = corner_h;
    int src_x_l = 0;
    int dst_x_l = 0;
    int src_x_r = bm->width - corner_w;
    int dst_x_r = width - corner_w;
    for (; dst_y < height - seg_h; dst_y += seg_h)
    {
        /* cap the last segment to the remaining height */
        int h = MIN(seg_h, (height - dst_y - seg_h));
        LCDFN(bmp_part)(bm, src_x_l, src_y, dst_x_l, dst_y, seg_w, h);
        LCDFN(bmp_part)(bm, src_x_r, src_y, dst_x_r, dst_y, seg_w, h);
    }
    /* center, need not be drawn if the desired rectangle is smaller than
     * the sides. in that case the rect is completely filled already */
    if (width > (2*corner_w) && height > (2*corner_h))
    {
        dst_y = src_y = corner_h;
        src_x = corner_w;
        for (; dst_y < height - seg_h; dst_y += seg_h)
        {
            /* cap the last segment to the remaining height */
            int h = MIN(seg_h, (height - dst_y - seg_h));
            dst_x = corner_w;
            for (; dst_x < width - seg_w; dst_x += seg_w)
            {
                /* cap the last segment to the remaining width */
                int w = MIN(seg_w, (width - dst_x - seg_w));
                LCDFN(bmp_part)(bm, src_x, src_y, dst_x, dst_y, w, h);
            }
        }
    }
    /* 4 corners */
    LCDFN(bmp_part)(bm, 0, 0, x, y, corner_w, corner_h);
    LCDFN(bmp_part)(bm, bm->width - corner_w, 0, width - corner_w, 0, corner_w, corner_h);
    LCDFN(bmp_part)(bm, 0, bm->width - corner_h, 0, height - corner_h, corner_w, corner_h);
    LCDFN(bmp_part)(bm, bm->width - corner_w, bm->width - corner_h,
            width - corner_w, height - corner_h, corner_w, corner_h);
}
