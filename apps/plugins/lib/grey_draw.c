/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* New greyscale framework
* Drawing functions
*
* This is a generic framework to display 129 shades of grey on low-depth
* bitmap LCDs (Archos b&w, Iriver & Ipod 4-grey) within plugins.
*
* Copyright (C) 2008 Jens Arnold
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
#include "grey.h"

extern struct viewport _grey_default_vp;

/*** low-level drawing functions ***/

static void setpixel(unsigned char *address)
{
    *address = _grey_info.vp->fg_pattern;
}

static void clearpixel(unsigned char *address)
{
    *address = _grey_info.vp->bg_pattern;
}

static void flippixel(unsigned char *address)
{
    *address = ~(*address);
}

static void nopixel(unsigned char *address)
{
    (void)address;
}

void (* const _grey_pixelfuncs[8])(unsigned char *address) = {
    flippixel, nopixel, setpixel, setpixel,
    nopixel, clearpixel, nopixel, clearpixel
};

/*** Drawing functions ***/

/* Clear the current viewport */
void grey_clear_viewport(void)
{
    struct viewport *vp = _grey_info.vp;
    int drawmode = vp->drawmode;
    vp->drawmode = DRMODE_SOLID | DRMODE_INVERSEVID;
    grey_fillrect(0, 0, vp->width, vp->height);
    vp->drawmode = drawmode;
}

/* Clear the whole display */
void grey_clear_display(void)
{
    struct viewport *vp = &_grey_default_vp;

    int value = (vp->drawmode & DRMODE_INVERSEVID) ?
                 vp->fg_pattern : vp->bg_pattern;

    rb->memset(_grey_info.curbuffer, value,
               _GREY_MULUQ(_grey_info.cb_width, _grey_info.cb_height));
}

/* Set a single pixel */
void grey_drawpixel(int x, int y)
{
    if (x >= _grey_info.clip_l && x < _grey_info.clip_r &&
        y >= _grey_info.clip_t && y < _grey_info.clip_b)
    {
        int dst_stride = _grey_info.cb_width;
        struct viewport *vp = _grey_info.vp;
        _grey_pixelfuncs[vp->drawmode](
                &_grey_info.curbuffer[
                    _GREY_MULUQ(dst_stride, vp->y - _grey_info.cb_y + y) +
                    vp->x - _grey_info.cb_x + x]);
    }
}

/* Draw a line */
void grey_drawline(int x1, int y1, int x2, int y2)
{
    struct viewport *vp = _grey_info.vp;
    int numpixels;
    int i;
    int deltax, deltay;
    int d, dinc1, dinc2;
    int x, xinc1, xinc2;
    int y, yinc1, yinc2;
    void (*pfunc)(unsigned char *address) = _grey_pixelfuncs[vp->drawmode];
    int dwidth;
    int xoffs, yoffs;

    deltax = abs(x2 - x1);
    deltay = abs(y2 - y1);
    xinc2 = 1;
    yinc2 = 1;

    if (deltax >= deltay)
    {
        numpixels = deltax;
        d = 2 * deltay - deltax;
        dinc1 = deltay * 2;
        dinc2 = (deltay - deltax) * 2;
        xinc1 = 1;
        yinc1 = 0;
    }
    else
    {
        numpixels = deltay;
        d = 2 * deltax - deltay;
        dinc1 = deltax * 2;
        dinc2 = (deltax - deltay) * 2;
        xinc1 = 0;
        yinc1 = 1;
    }
    numpixels++; /* include endpoints */

    if (x1 > x2)
    {
        xinc1 = -xinc1;
        xinc2 = -xinc2;
    }

    if (y1 > y2)
    {
        yinc1 = -yinc1;
        yinc2 = -yinc2;
    }

    x = x1;
    y = y1;

    dwidth = _grey_info.cb_width;
    xoffs  = vp->x - _grey_info.cb_x;
    yoffs  = vp->y - _grey_info.cb_y;

    for (i = 0; i < numpixels; i++)
    {
        if (x >= _grey_info.clip_l && x < _grey_info.clip_r &&
            y >= _grey_info.clip_t && y < _grey_info.clip_b)
        {
            pfunc(&_grey_info.curbuffer[_GREY_MULUQ(dwidth, yoffs + y) +
                                        xoffs + x]);
        }

        if (d < 0)
        {
            d += dinc1;
            x += xinc1;
            y += yinc1;
        }
        else
        {
            d += dinc2;
            x += xinc2;
            y += yinc2;
        }
    }
}

/* Draw a horizontal line (optimised) */
void grey_hline(int x1, int x2, int y)
{
    struct viewport *vp = _grey_info.vp;
    int x;
    int value = 0;
    unsigned char *dst;
    bool fillopt = false;
    int dwidth;

    /* direction flip */
    if (x2 < x1)
    {
        x = x1;
        x1 = x2;
        x2 = x;
    }

    /* nothing to draw? */
    if (y < _grey_info.clip_t || y >= _grey_info.clip_b ||
        x1 >= _grey_info.clip_r || x2 < _grey_info.clip_l)
        return;  
    
    /* drawmode and optimisation */
    if (vp->drawmode & DRMODE_INVERSEVID)
    {
        if (vp->drawmode & DRMODE_BG)
        {
            fillopt = true;
            value = vp->bg_pattern;
        }
    }
    else
    {
        if (vp->drawmode & DRMODE_FG)
        {
            fillopt = true;
            value = vp->fg_pattern;
        }
    }
    if (!fillopt && vp->drawmode != DRMODE_COMPLEMENT)
        return;

    /* clipping */
    if (x1 < _grey_info.clip_l)
        x1 = _grey_info.clip_l;
    if (x2 >= _grey_info.clip_r)
        x2 = _grey_info.clip_r - 1;

    dwidth = _grey_info.cb_width;
    dst    = &_grey_info.curbuffer[
                    _GREY_MULUQ(dwidth, vp->y - _grey_info.cb_y + y) +
                    vp->x - _grey_info.cb_x + x1];

    if (fillopt)
        rb->memset(dst, value, x2 - x1 + 1);
    else  /* DRMODE_COMPLEMENT */
    {
        unsigned char *dst_end = dst + x2 - x1;
        do
            *dst = ~(*dst);
        while (++dst <= dst_end);
    }
}

/* Draw a vertical line (optimised) */
void grey_vline(int x, int y1, int y2)
{
    struct viewport *vp = _grey_info.vp;
    int y;
    unsigned char *dst, *dst_end;
    void (*pfunc)(unsigned char *address);
    int dwidth;
    
    /* direction flip */
    if (y2 < y1)
    {
        y = y1;
        y1 = y2;
        y2 = y;
    }

    /* nothing to draw? */
    if (x < _grey_info.clip_l || x >= _grey_info.clip_r ||
        y1 >= _grey_info.clip_b || y2 < _grey_info.clip_t)
        return;
    
    /* clipping */
    if (y1 < _grey_info.clip_t)
        y1 = _grey_info.clip_t;
    if (y2 >= _grey_info.clip_b)
        y2 = _grey_info.clip_b - 1;

    dwidth = _grey_info.cb_width;
    pfunc  = _grey_pixelfuncs[vp->drawmode];
    dst    = &_grey_info.curbuffer[
                    _GREY_MULUQ(dwidth, vp->y - _grey_info.cb_y + y1) +
                                vp->x - _grey_info.cb_x + x];

    dst_end = dst + _GREY_MULUQ(dwidth, y2 - y1);
    do
    {
        pfunc(dst);
        dst += dwidth;
    }
    while (dst <= dst_end);
}

/* Draw a filled triangle */
void grey_filltriangle(int x1, int y1, int x2, int y2, int x3, int y3)
{
    int x, y;
    long fp_x1, fp_x2, fp_dx1, fp_dx2;

    /* sort vertices by increasing y value */
    if (y1 > y3)
    {
        if (y2 < y3)       /* y2 < y3 < y1 */
        {
            x = x1; x1 = x2; x2 = x3; x3 = x;
            y = y1; y1 = y2; y2 = y3; y3 = y;
        }
        else if (y2 > y1)  /* y3 < y1 < y2 */
        {
            x = x1; x1 = x3; x3 = x2; x2 = x;
            y = y1; y1 = y3; y3 = y2; y2 = y;
        }
        else               /* y3 <= y2 <= y1 */
        {
            x = x1; x1 = x3; x3 = x;
            y = y1; y1 = y3; y3 = y;
        }
    }
    else
    {
        if (y2 < y1)       /* y2 < y1 <= y3 */
        {
            x = x1; x1 = x2; x2 = x;
            y = y1; y1 = y2; y2 = y;
        }
        else if (y2 > y3)  /* y1 <= y3 < y2 */
        {
            x = x2; x2 = x3; x3 = x;
            y = y2; y2 = y3; y3 = y;
        }
        /* else already sorted */
    }

    if (y1 < y3)  /* draw */
    {
        fp_dx1 = ((x3 - x1) << 16) / (y3 - y1);
        fp_x1  = (x1 << 16) + (1<<15) + (fp_dx1 >> 1);

        if (y1 < y2)  /* first part */
        {
            fp_dx2 = ((x2 - x1) << 16) / (y2 - y1);
            fp_x2  = (x1 << 16) + (1<<15) + (fp_dx2 >> 1);
            for (y = y1; y < y2; y++)
            {
                grey_hline(fp_x1 >> 16, fp_x2 >> 16, y);
                fp_x1 += fp_dx1;
                fp_x2 += fp_dx2;
            }
        }
        if (y2 < y3)  /* second part */
        {
            fp_dx2 = ((x3 - x2) << 16) / (y3 - y2);
            fp_x2 = (x2 << 16) + (1<<15) + (fp_dx2 >> 1);
            for (y = y2; y < y3; y++)
            {
                grey_hline(fp_x1 >> 16, fp_x2 >> 16, y);
                fp_x1 += fp_dx1;
                fp_x2 += fp_dx2;
            }
        }
    }
}

/* Draw a rectangular box */
void grey_drawrect(int x, int y, int width, int height)
{
    if ((width <= 0) || (height <= 0))
        return;

    int x2 = x + width - 1;
    int y2 = y + height - 1;

    grey_vline(x, y, y2);
    grey_vline(x2, y, y2);
    grey_hline(x, x2, y);
    grey_hline(x, x2, y2);
}

/* Fill a rectangular area */
void grey_fillrect(int x, int y, int width, int height)
{
    struct viewport *vp = _grey_info.vp;
    int value = 0;
    unsigned char *dst, *dst_end;
    bool fillopt = false;
    int dwidth;

    /* drawmode and optimisation */
    if (vp->drawmode & DRMODE_INVERSEVID)
    {
        if (vp->drawmode & DRMODE_BG)
        {
            fillopt = true;
            value = vp->bg_pattern;
        }
    }
    else
    {
        if (vp->drawmode & DRMODE_FG)
        {
            fillopt = true;
            value = vp->fg_pattern;

        }
    }
    if (!fillopt && vp->drawmode != DRMODE_COMPLEMENT)
        return;

    /* clipping */
    if (x < _grey_info.clip_l)
    {
        width += x - _grey_info.clip_l;
        x = _grey_info.clip_l;
    }

    if (x + width > _grey_info.clip_r)
        width = _grey_info.clip_r - x;

    if (width <= 0)
        return;

    if (y < _grey_info.clip_t)
    {
        height += y - _grey_info.clip_t;
        y = _grey_info.clip_t;
    }

    if (y + height > _grey_info.clip_b)
        height = _grey_info.clip_b - y;

    if (height <= 0)
        return;
    
    dwidth  = _grey_info.cb_width;
    dst     = &_grey_info.curbuffer[
                _GREY_MULUQ(dwidth, _grey_info.vp->y - _grey_info.cb_y + y) +
                _grey_info.vp->x - _grey_info.cb_x + x];
    dst_end = dst + _GREY_MULUQ(dwidth, height);

    do
    {
        if (fillopt)
            rb->memset(dst, value, width);
        else  /* DRMODE_COMPLEMENT */
        {
            unsigned char *dst_row = dst;
            unsigned char *row_end = dst_row + width;

            do
                *dst_row = ~(*dst_row);
            while (++dst_row < row_end);
        }
        dst += dwidth;
    }
    while (dst < dst_end);
}

/* About Rockbox' internal monochrome bitmap format:
 *
 * A bitmap contains one bit for every pixel that defines if that pixel is
 * foreground (1) or background (0). Bits within a byte are arranged
 * vertically, LSB at top.
 * The bytes are stored in row-major order, with byte 0 being top left,
 * byte 1 2nd from left etc. The first row of bytes defines pixel rows
 * 0..7, the second row defines pixel row 8..15 etc. */

/* Draw a partial monochrome bitmap */
void grey_mono_bitmap_part(const unsigned char *src, int src_x, int src_y,
                           int stride, int x, int y, int width, int height)
{
    struct viewport *vp = _grey_info.vp;
    const unsigned char *src_end;
    unsigned char *dst, *dst_end;
    unsigned dmask = 0x100; /* bit 8 == sentinel */
    int drmode = vp->drawmode;
    int dwidth;

    /* clipping */
    if (x < _grey_info.clip_l)
    {
        int dx = x - _grey_info.clip_l;
        width += dx;
        src_x -= dx;
        x = _grey_info.clip_l;
    }

    if (x + width > _grey_info.clip_r)
        width = _grey_info.clip_r - x;

    if (width <= 0)
        return;

    if (y < _grey_info.clip_t)
    {
        int dy = y - _grey_info.clip_t;
        height += dy;
        src_y  += dy;
        y = _grey_info.clip_t;
    }

    if (y + height > _grey_info.clip_b)
        height = _grey_info.clip_b - y;

    if (height <= 0)
        return;

    src    += _GREY_MULUQ(stride, src_y >> 3) + src_x; /* move starting point */
    src_y  &= 7;
    src_end = src + width;
    dwidth  = _grey_info.cb_width;
    dst     = &_grey_info.curbuffer[
                        _GREY_MULUQ(dwidth, vp->y - _grey_info.cb_y + y) +
                                    vp->x - _grey_info.cb_x + x];
    dst_end = dst + _GREY_MULUQ(dwidth, height);

    if (drmode & DRMODE_INVERSEVID)
    {
        dmask = 0x1ff;          /* bit 8 == sentinel */
        drmode &= DRMODE_SOLID; /* mask out inversevid */
    }

    do
    {
        const unsigned char *src_col = src++;
        unsigned char *dst_col = dst++;
        unsigned data = (*src_col ^ dmask) >> src_y;
        int fg, bg;

#define UPDATE_SRC  do {                  \
            data >>= 1;                   \
            if (data == 0x001) {          \
                src_col += stride;        \
                data = *src_col ^ dmask;  \
            }                             \
        } while (0)

        switch (drmode)
        {
          case DRMODE_COMPLEMENT:
            do
            {
                if (data & 0x01)
                    *dst_col = ~(*dst_col);

                dst_col += dwidth;
                UPDATE_SRC;
            }
            while (dst_col < dst_end);
            break;

          case DRMODE_BG:
            bg = vp->bg_pattern;
            do
            {
                if (!(data & 0x01))
                    *dst_col = bg;

                dst_col += dwidth;
                UPDATE_SRC;
            }
            while (dst_col < dst_end);
            break;

          case DRMODE_FG:
            fg = vp->fg_pattern;
            do
            {
                if (data & 0x01)
                    *dst_col = fg;

                dst_col += dwidth;
                UPDATE_SRC;
            }
            while (dst_col < dst_end);
            break;

          case DRMODE_SOLID:
            fg = vp->fg_pattern;
            bg = vp->bg_pattern;
            do
            {
                *dst_col = (data & 0x01) ? fg : bg;
                dst_col += dwidth;
                UPDATE_SRC;
            }
            while (dst_col < dst_end);
            break;
        }
    }
    while (src < src_end);
}

/* Draw a full monochrome bitmap */
void grey_mono_bitmap(const unsigned char *src, int x, int y, int width, int height)
{
    grey_mono_bitmap_part(src, 0, 0, width, x, y, width, height);
}

/* Draw a partial greyscale bitmap, canonical format */
void grey_gray_bitmap_part(const unsigned char *src, int src_x, int src_y,
                           int stride, int x, int y, int width, int height)
{
    unsigned char *dst, *dst_end;
    int dwidth;

    /* clipping */
    if (x < _grey_info.clip_l)
    {
        int dx = x - _grey_info.clip_l;
        width += dx;
        src_x -= dx;
        x = _grey_info.clip_l;
    }

    if (x + width > _grey_info.clip_r)
        width = _grey_info.clip_r - x;

    if (width <= 0)
        return;

    if (y < 0)
    {
        int dy = y - _grey_info.clip_t;
        height += dy;
        src_y -= dy;
        y = _grey_info.clip_t;
    }

    if (y + height > _grey_info.clip_b)
        height = _grey_info.clip_b - y;

    if (height <= 0)
        return;

    dwidth = _grey_info.cb_width;
    src   += _GREY_MULUQ(stride, src_y) + src_x; /* move starting point */
    dst    = &_grey_info.curbuffer[
                _GREY_MULUQ(dwidth, _grey_info.vp->y - _grey_info.cb_y + y) +
                _grey_info.vp->x - _grey_info.cb_x + x];
    dst_end = dst + _GREY_MULUQ(dwidth, height);

    do
    {
        rb->memcpy(dst, src, width);
        dst += dwidth;
        src += stride;
    }
    while (dst < dst_end);
}

/* Draw a full greyscale bitmap, canonical format */
void grey_gray_bitmap(const unsigned char *src, int x, int y, int width,
                      int height)
{
    grey_gray_bitmap_part(src, 0, 0, width, x, y, width, height);
}

/* Put a string at a given pixel position, skipping first ofs pixel columns */
void grey_putsxyofs(int x, int y, int ofs, const unsigned char *str)
{
    int ch;
    unsigned short *ucs;
    struct font* pf;

    if (_grey_info.clip_b <= _grey_info.clip_t)
        return;

    pf = rb->font_get(_grey_info.vp->font);
    ucs = rb->bidi_l2v(str, 1);

    while ((ch = *ucs++) != 0 && x < _grey_info.clip_r)
    {
        int width;
        const unsigned char *bits;

        /* get proportional width and glyph bits */
        width = rb->font_get_width(pf, ch);

        if (ofs > width)
        {
            ofs -= width;
            continue;
        }

        bits = rb->font_get_bits(pf, ch);

        grey_mono_bitmap_part(bits, ofs, 0, width, x, y, width - ofs, pf->height);
        
        x += width - ofs;
        ofs = 0;
    }
}

/* Put a string at a given pixel position */
void grey_putsxy(int x, int y, const unsigned char *str)
{
    grey_putsxyofs(x, y, 0, str);
}

/*** Unbuffered drawing functions ***/

/* Clear the greyscale display (sets all pixels to white) */
void grey_ub_clear_display(void)
{
    struct viewport *vp = &_grey_default_vp;
    int value = _grey_info.gvalue[(vp->drawmode & DRMODE_INVERSEVID) ?
                                   vp->fg_pattern : vp->bg_pattern];

    rb->memset(_grey_info.values, value,
                          _GREY_MULUQ(_grey_info.width, _grey_info.height));
#ifdef SIMULATOR
    rb->sim_lcd_ex_update_rect(_grey_info.x, _grey_info.y,
                                          _grey_info.width, _grey_info.height);
#endif
}

/* Assembler optimised helper function for copying a single line to the 
 * greyvalue buffer. */
void _grey_line1(int width, unsigned char *dst, const unsigned char *src,
                 const unsigned char *lut);

/* Draw a partial greyscale bitmap, canonical format */
void grey_ub_gray_bitmap_part(const unsigned char *src, int src_x, int src_y,
                              int stride, int x, int y, int width, int height)
{
    int yc, ye;
    unsigned char *dst;

    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= _grey_info.width)
        || (y >= _grey_info.height) || (x + width <= 0) || (y + height <= 0))
        return;
        
    /* clipping */
    if (x < 0)
    {
        width += x;
        src_x -= x;
        x = 0;
    }
    if (y < 0)
    {
        height += y;
        src_y -= y;
        y = 0;
    }
    if (x + width > _grey_info.width)
        width = _grey_info.width - x;
    if (y + height > _grey_info.height)
        height = _grey_info.height - y;

    src += _GREY_MULUQ(stride, src_y) + src_x; /* move starting point */ 
    yc = y;
    ye = y + height;
    dst = _grey_info.values + (x << _GREY_BSHIFT);

    do
    {
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
        int idx = _GREY_MULUQ(_grey_info.width, yc);
#else /* vertical packing or vertical interleaved */
        int idx = _GREY_MULUQ(_grey_info.width, yc & ~_GREY_BMASK)
                + (~yc & _GREY_BMASK);
#endif /* LCD_PIXELFORMAT */

#if ((LCD_PIXELFORMAT == VERTICAL_PACKING) && (LCD_DEPTH == 2) && defined(CPU_COLDFIRE)) \
 || ((LCD_PIXELFORMAT == VERTICAL_INTERLEAVED) && defined(CPU_COLDFIRE))
        _grey_line1(width, dst + idx, src, _grey_info.gvalue);
#else
        unsigned char *dst_row = dst + idx;
        const unsigned char *src_row = src;
        const unsigned char *src_end = src + width;

        do
        {
            *dst_row = _grey_info.gvalue[*src_row++];
            dst_row += _GREY_BSIZE;
        }
        while (src_row < src_end);
#endif
        
        src += stride;
    }
    while (++yc < ye);
#ifdef SIMULATOR
    rb->sim_lcd_ex_update_rect(_grey_info.x + x, _grey_info.y + y,
                                          width, height);
#endif
}

/* Draw a full greyscale bitmap, canonical format */
void grey_ub_gray_bitmap(const unsigned char *src, int x, int y, int width,
                         int height)
{
    grey_ub_gray_bitmap_part(src, 0, 0, width, x, y, width, height);
}

static void output_row_grey_8(uint32_t row, void * row_in,
                              struct scaler_context *ctx)
{
    uint8_t *dest = (uint8_t*)ctx->bm->data + ctx->bm->width * row;
    rb->memcpy(dest, row_in, ctx->bm->width);
}

static void output_row_grey_32(uint32_t row, void * row_in,
                               struct scaler_context *ctx)
{
    int col;
    uint32_t *qp = (uint32_t*)row_in;
    uint8_t *dest = (uint8_t*)ctx->bm->data + ctx->bm->width * row;
    for (col = 0; col < ctx->bm->width; col++)
        *dest++ = SC_OUT(*qp++, ctx);
}

static unsigned int get_size_grey(struct bitmap *bm)
{
    return bm->width * bm->height;
}

const struct custom_format format_grey = {
    .output_row_8 = output_row_grey_8,
    .output_row_32 = output_row_grey_32,
    .get_size = get_size_grey
};
