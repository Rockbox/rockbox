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
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#include "plugin.h"
#include "grey.h"

/*** low-level drawing functions ***/

static void setpixel(unsigned char *address)
{
    *address = _grey_info.fg_brightness;
}

static void clearpixel(unsigned char *address)
{
    *address = _grey_info.bg_brightness;
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

/* Clear the whole display */
void grey_clear_display(void)
{
    int value = (_grey_info.drawmode & DRMODE_INVERSEVID) ?
                 _grey_info.fg_brightness : _grey_info.bg_brightness;

    _grey_info.rb->memset(_grey_info.buffer, value,
                          _GREY_MULUQ(_grey_info.width, _grey_info.height));
}

/* Set a single pixel */
void grey_drawpixel(int x, int y)
{
    if (((unsigned)x < (unsigned)_grey_info.width)
        && ((unsigned)y < (unsigned)_grey_info.height))
        _grey_pixelfuncs[_grey_info.drawmode](&_grey_info.buffer[_GREY_MULUQ(
                                               _grey_info.width, y) + x]);
}

/* Draw a line */
void grey_drawline(int x1, int y1, int x2, int y2)
{
    int numpixels;
    int i;
    int deltax, deltay;
    int d, dinc1, dinc2;
    int x, xinc1, xinc2;
    int y, yinc1, yinc2;
    void (*pfunc)(unsigned char *address) = _grey_pixelfuncs[_grey_info.drawmode];

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

    for (i = 0; i < numpixels; i++)
    {
        if (((unsigned)x < (unsigned)_grey_info.width)
            && ((unsigned)y < (unsigned)_grey_info.height))
            pfunc(&_grey_info.buffer[_GREY_MULUQ(_grey_info.width, y) + x]);

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
    int x;
    int value = 0;
    unsigned char *dst;
    bool fillopt = false;
    void (*pfunc)(unsigned char *address);
    
    /* direction flip */
    if (x2 < x1)
    {
        x = x1;
        x1 = x2;
        x2 = x;
    }

    /* nothing to draw? */
    if (((unsigned)y >= (unsigned)_grey_info.height)
        || (x1 >= _grey_info.width) || (x2 < 0))
        return;  
    
    /* clipping */
    if (x1 < 0)
        x1 = 0;
    if (x2 >= _grey_info.width)
        x2 = _grey_info.width - 1;
        
    if (_grey_info.drawmode & DRMODE_INVERSEVID)
    {
        if (_grey_info.drawmode & DRMODE_BG)
        {
            fillopt = true;
            value = _grey_info.bg_brightness;
        }
    }
    else
    {
        if (_grey_info.drawmode & DRMODE_FG)
        {
            fillopt = true;
            value = _grey_info.fg_brightness;
        }
    }
    pfunc = _grey_pixelfuncs[_grey_info.drawmode];
    dst   = &_grey_info.buffer[_GREY_MULUQ(_grey_info.width, y) + x1];

    if (fillopt)
        _grey_info.rb->memset(dst, value, x2 - x1 + 1);
    else
    {
        unsigned char *dst_end = dst + x2 - x1;
        do
            pfunc(dst++);
        while (dst <= dst_end);
    }
}

/* Draw a vertical line (optimised) */
void grey_vline(int x, int y1, int y2)
{
    int y;
    unsigned char *dst, *dst_end;
    void (*pfunc)(unsigned char *address);
    
    /* direction flip */
    if (y2 < y1)
    {
        y = y1;
        y1 = y2;
        y2 = y;
    }

    /* nothing to draw? */
    if (((unsigned)x >= (unsigned)_grey_info.width)
        || (y1 >= _grey_info.height) || (y2 < 0))
        return;
    
    /* clipping */
    if (y1 < 0)
        y1 = 0;
    if (y2 >= _grey_info.height)
        y2 = _grey_info.height - 1;

    pfunc = _grey_pixelfuncs[_grey_info.drawmode];
    dst   = &_grey_info.buffer[_GREY_MULUQ(_grey_info.width, y1) + x];

    dst_end = dst + _GREY_MULUQ(_grey_info.width, y2 - y1);
    do
    {
        pfunc(dst);
        dst += _grey_info.width;
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
    int value = 0;
    unsigned char *dst, *dst_end;
    bool fillopt = false;
    void (*pfunc)(unsigned char *address);

    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= _grey_info.width)
        || (y >= _grey_info.height) || (x + width <= 0) || (y + height <= 0))
        return;

    /* clipping */
    if (x < 0)
    {
        width += x;
        x = 0;
    }
    if (y < 0)
    {
        height += y;
        y = 0;
    }
    if (x + width > _grey_info.width)
        width = _grey_info.width - x;
    if (y + height > _grey_info.height)
        height = _grey_info.height - y;
    
    if (_grey_info.drawmode & DRMODE_INVERSEVID)
    {
        if (_grey_info.drawmode & DRMODE_BG)
        {
            fillopt = true;
            value = _grey_info.bg_brightness;
        }
    }
    else
    {
        if (_grey_info.drawmode & DRMODE_FG)
        {
            fillopt = true;
            value = _grey_info.fg_brightness;
        }
    }
    pfunc = _grey_pixelfuncs[_grey_info.drawmode];
    dst   = &_grey_info.buffer[_GREY_MULUQ(_grey_info.width, y) + x];
    dst_end = dst + _GREY_MULUQ(_grey_info.width, height);

    do
    {
        if (fillopt)
            _grey_info.rb->memset(dst, value, width);
        else
        {
            unsigned char *dst_row = dst;
            unsigned char *row_end = dst_row + width;

            do
                pfunc(dst_row++);
            while (dst_row < row_end);
        }
        dst += _grey_info.width;
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
    const unsigned char *src_end;
    unsigned char *dst, *dst_end;
    void (*fgfunc)(unsigned char *address);
    void (*bgfunc)(unsigned char *address);

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

    src    += _GREY_MULUQ(stride, src_y >> 3) + src_x; /* move starting point */
    src_y  &= 7;
    src_end = src + width;

    fgfunc = _grey_pixelfuncs[_grey_info.drawmode];
    bgfunc = _grey_pixelfuncs[_grey_info.drawmode ^ DRMODE_INVERSEVID];
    dst    = &_grey_info.buffer[_GREY_MULUQ(_grey_info.width, y) + x];

    do
    {
        const unsigned char *src_col = src++;
        unsigned char *dst_col = dst++;
        unsigned data = *src_col >> src_y;
        int numbits = 8 - src_y;
        
        dst_end = dst_col + _GREY_MULUQ(_grey_info.width, height);
        do
        {
            if (data & 0x01)
                fgfunc(dst_col);
            else
                bgfunc(dst_col);

            dst_col += _grey_info.width;

            data >>= 1;
            if (--numbits == 0)
            {
                src_col += stride;
                data = *src_col;
                numbits = 8;
            }
        }
        while (dst_col < dst_end);
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

    src    += _GREY_MULUQ(stride, src_y) + src_x; /* move starting point */
    dst     = &_grey_info.buffer[_GREY_MULUQ(_grey_info.width, y) + x];
    dst_end = dst + _GREY_MULUQ(_grey_info.width, height);

    do
    {
        _grey_info.rb->memcpy(dst, src, width);
        dst += _grey_info.width;
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
    struct font* pf = _grey_info.rb->font_get(_grey_info.curfont);

    while ((ch = *str++) != '\0' && x < _grey_info.width)
    {
        int width;
        const unsigned char *bits;

        /* check input range */
        if (ch < pf->firstchar || ch >= pf->firstchar+pf->size)
            ch = pf->defaultchar;
        ch -= pf->firstchar;

        /* get proportional width and glyph bits */
        width = pf->width ? pf->width[ch] : pf->maxwidth;

        if (ofs > width)
        {
            ofs -= width;
            continue;
        }

        bits = pf->bits + (pf->offset ?
               pf->offset[ch] : (((pf->height + 7) >> 3) * pf->maxwidth * ch));

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
    int value = _grey_info.gvalue[(_grey_info.drawmode & DRMODE_INVERSEVID) ?
                                  _grey_info.fg_brightness :
                                  _grey_info.bg_brightness];

    _grey_info.rb->memset(_grey_info.values, value,
                          _GREY_MULUQ(_grey_info.width, _grey_info.height));
#ifdef SIMULATOR
    _grey_info.rb->sim_lcd_ex_update_rect(_grey_info.x, _grey_info.y,
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

#if (LCD_PIXELFORMAT == VERTICAL_PACKING) && \
     ((LCD_DEPTH == 2) && defined(CPU_COLDFIRE) \
      || (LCD_DEPTH == 1) && (CONFIG_CPU == SH7034))
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
    _grey_info.rb->sim_lcd_ex_update_rect(_grey_info.x + x, _grey_info.y + y,
                                          width, height);
#endif
}

/* Draw a full greyscale bitmap, canonical format */
void grey_ub_gray_bitmap(const unsigned char *src, int x, int y, int width,
                         int height)
{
    grey_ub_gray_bitmap_part(src, 0, 0, width, x, y, width, height);
}
