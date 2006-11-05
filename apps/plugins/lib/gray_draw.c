/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Greyscale framework
* Drawing functions
*
* This is a generic framework to display up to 33 shades of grey
* on low-depth bitmap LCDs (Archos b&w, Iriver 4-grey, iPod 4-grey)
* within plugins.
*
* Copyright (C) 2004-2006 Jens Arnold
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#include "plugin.h"

#ifdef HAVE_LCD_BITMAP
#include "gray.h"

/*** low-level drawing functions ***/

static void setpixel(unsigned char *address)
{
    *address = _gray_info.fg_index;
}

static void clearpixel(unsigned char *address)
{
    *address = _gray_info.bg_index;
}

static void flippixel(unsigned char *address)
{
    *address = _gray_info.depth - *address;
}

static void nopixel(unsigned char *address)
{
    (void)address;
}

void (* const _gray_pixelfuncs[8])(unsigned char *address) = {
    flippixel, nopixel, setpixel, setpixel,
    nopixel, clearpixel, nopixel, clearpixel
};

/*** Drawing functions ***/

/* Clear the whole display */
void gray_clear_display(void)
{
    int brightness = (_gray_info.drawmode & DRMODE_INVERSEVID) ?
                     _gray_info.fg_index : _gray_info.bg_index;

    _gray_rb->memset(_gray_info.cur_buffer, brightness,
                     _GRAY_MULUQ(_gray_info.width, _gray_info.height));
}

/* Set a single pixel */
void gray_drawpixel(int x, int y)
{
    if (((unsigned)x < (unsigned)_gray_info.width)
        && ((unsigned)y < (unsigned)_gray_info.height))
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
        _gray_pixelfuncs[_gray_info.drawmode](&_gray_info.cur_buffer[_GRAY_MULUQ(y,
                                               _gray_info.width) + x]);
#else
        _gray_pixelfuncs[_gray_info.drawmode](&_gray_info.cur_buffer[_GRAY_MULUQ(x,
                                               _gray_info.height) + y]);
#endif
}

/* Draw a line */
void gray_drawline(int x1, int y1, int x2, int y2)
{
    int numpixels;
    int i;
    int deltax, deltay;
    int d, dinc1, dinc2;
    int x, xinc1, xinc2;
    int y, yinc1, yinc2;
    void (*pfunc)(unsigned char *address) = _gray_pixelfuncs[_gray_info.drawmode];

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
        if (((unsigned)x < (unsigned)_gray_info.width) 
            && ((unsigned)y < (unsigned)_gray_info.height))
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
            pfunc(&_gray_info.cur_buffer[_GRAY_MULUQ(y, _gray_info.width) + x]);
#else
            pfunc(&_gray_info.cur_buffer[_GRAY_MULUQ(x, _gray_info.height) + y]);
#endif

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

#if LCD_PIXELFORMAT == HORIZONTAL_PACKING

/* Draw a horizontal line (optimised) */
void gray_hline(int x1, int x2, int y)
{
    int x;
    int bits = 0;
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
    if (((unsigned)y >= (unsigned)_gray_info.height) 
        || (x1 >= _gray_info.width) || (x2 < 0))
        return;  
    
    /* clipping */
    if (x1 < 0)
        x1 = 0;
    if (x2 >= _gray_info.width)
        x2 = _gray_info.width - 1;
        
    if (_gray_info.drawmode & DRMODE_INVERSEVID)
    {
        if (_gray_info.drawmode & DRMODE_BG)
        {
            fillopt = true;
            bits = _gray_info.bg_index;
        }
    }
    else
    {
        if (_gray_info.drawmode & DRMODE_FG)
        {
            fillopt = true;
            bits = _gray_info.fg_index;
        }
    }
    pfunc = _gray_pixelfuncs[_gray_info.drawmode];
    dst   = &_gray_info.cur_buffer[_GRAY_MULUQ(y, _gray_info.width) + x1];

    if (fillopt)
        _gray_rb->memset(dst, bits, x2 - x1 + 1);
    else
    {
        unsigned char *dst_end = dst + x2 - x1;
        do
            pfunc(dst++);
        while (dst <= dst_end);
    }
}

/* Draw a vertical line (optimised) */
void gray_vline(int x, int y1, int y2)
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
    if (((unsigned)x >= (unsigned)_gray_info.width) 
        || (y1 >= _gray_info.height) || (y2 < 0))
        return;  
    
    /* clipping */
    if (y1 < 0)
        y1 = 0;
    if (y2 >= _gray_info.height)
        y2 = _gray_info.height - 1;

    pfunc = _gray_pixelfuncs[_gray_info.drawmode];
    dst   = &_gray_info.cur_buffer[_GRAY_MULUQ(y1, _gray_info.width) + x];

    dst_end = dst + _GRAY_MULUQ(y2 - y1, _gray_info.width);
    do
    {
        pfunc(dst);
        dst += _gray_info.width;
    }
    while (dst <= dst_end);
}

/* Draw a filled triangle */
void gray_filltriangle(int x1, int y1, int x2, int y2, int x3, int y3)
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
                gray_hline(fp_x1 >> 16, fp_x2 >> 16, y);
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
                gray_hline(fp_x1 >> 16, fp_x2 >> 16, y);
                fp_x1 += fp_dx1;
                fp_x2 += fp_dx2;
            }
        }
    }
}
#else /* LCD_PIXELFORMAT == VERTICAL_PACKING */

/* Draw a horizontal line (optimised) */
void gray_hline(int x1, int x2, int y)
{
    int x;
    unsigned char *dst, *dst_end;
    void (*pfunc)(unsigned char *address);

    /* direction flip */
    if (x2 < x1)
    {
        x = x1;
        x1 = x2;
        x2 = x;
    }
    
    /* nothing to draw? */
    if (((unsigned)y >= (unsigned)_gray_info.height) 
        || (x1 >= _gray_info.width) || (x2 < 0))
        return;  
    
    /* clipping */
    if (x1 < 0)
        x1 = 0;
    if (x2 >= _gray_info.width)
        x2 = _gray_info.width - 1;
        
    pfunc = _gray_pixelfuncs[_gray_info.drawmode];
    dst   = &_gray_info.cur_buffer[_GRAY_MULUQ(x1, _gray_info.height) + y];

    dst_end = dst + _GRAY_MULUQ(x2 - x1, _gray_info.height);
    do
    {
        pfunc(dst);
        dst += _gray_info.height;
    }
    while (dst <= dst_end);
}

/* Draw a vertical line (optimised) */
void gray_vline(int x, int y1, int y2)
{
    int y;
    int bits = 0;
    unsigned char *dst;
    bool fillopt = false;
    void (*pfunc)(unsigned char *address);

    /* direction flip */
    if (y2 < y1)
    {
        y = y1;
        y1 = y2;
        y2 = y;
    }

    /* nothing to draw? */
    if (((unsigned)x >= (unsigned)_gray_info.width) 
        || (y1 >= _gray_info.height) || (y2 < 0))
        return;  
    
    /* clipping */
    if (y1 < 0)
        y1 = 0;
    if (y2 >= _gray_info.height)
        y2 = _gray_info.height - 1;
        
    if (_gray_info.drawmode & DRMODE_INVERSEVID)
    {
        if (_gray_info.drawmode & DRMODE_BG)
        {
            fillopt = true;
            bits = _gray_info.bg_index;
        }
    }
    else
    {
        if (_gray_info.drawmode & DRMODE_FG)
        {
            fillopt = true;
            bits = _gray_info.fg_index;
        }
    }
    pfunc = _gray_pixelfuncs[_gray_info.drawmode];
    dst   = &_gray_info.cur_buffer[_GRAY_MULUQ(x, _gray_info.height) + y1];

    if (fillopt)
        _gray_rb->memset(dst, bits, y2 - y1 + 1);
    else
    {
        unsigned char *dst_end = dst + y2 - y1;
        do
            pfunc(dst++);
        while (dst <= dst_end);
    }
}

/* Draw a filled triangle */
void gray_filltriangle(int x1, int y1, int x2, int y2, int x3, int y3)
{
    int x, y;
    long fp_y1, fp_y2, fp_dy1, fp_dy2;

    /* sort vertices by increasing x value */
    if (x1 > x3)
    {
        if (x2 < x3)       /* x2 < x3 < x1 */
        {
            x = x1; x1 = x2; x2 = x3; x3 = x;
            y = y1; y1 = y2; y2 = y3; y3 = y;
        }
        else if (x2 > x1)  /* x3 < x1 < x2 */
        {
            x = x1; x1 = x3; x3 = x2; x2 = x;
            y = y1; y1 = y3; y3 = y2; y2 = y;
        }
        else               /* x3 <= x2 <= x1 */
        {
            x = x1; x1 = x3; x3 = x;
            y = y1; y1 = y3; y3 = y;
        }
    }
    else
    {
        if (x2 < x1)       /* x2 < x1 <= x3 */
        {
            x = x1; x1 = x2; x2 = x;
            y = y1; y1 = y2; y2 = y;
        }
        else if (x2 > x3)  /* x1 <= x3 < x2 */
        {
            x = x2; x2 = x3; x3 = x;
            y = y2; y2 = y3; y3 = y;
        }
        /* else already sorted */
    }

    if (x1 < x3)  /* draw */
    {
        fp_dy1 = ((y3 - y1) << 16) / (x3 - x1);
        fp_y1  = (y1 << 16) + (1<<15) + (fp_dy1 >> 1);

        if (x1 < x2)  /* first part */
        {   
            fp_dy2 = ((y2 - y1) << 16) / (x2 - x1);
            fp_y2  = (y1 << 16) + (1<<15) + (fp_dy2 >> 1);
            for (x = x1; x < x2; x++)
            {
                gray_vline(x, fp_y1 >> 16, fp_y2 >> 16);
                fp_y1 += fp_dy1;
                fp_y2 += fp_dy2;
            }
        }
        if (x2 < x3)  /* second part */
        {
            fp_dy2 = ((y3 - y2) << 16) / (x3 - x2);
            fp_y2 = (y2 << 16) + (1<<15) + (fp_dy2 >> 1);
            for (x = x2; x < x3; x++)
            {
                gray_vline(x, fp_y1 >> 16, fp_y2 >> 16);
                fp_y1 += fp_dy1;
                fp_y2 += fp_dy2;
            }
        }
    }
}
#endif /* LCD_PIXELFORMAT */

/* Draw a rectangular box */
void gray_drawrect(int x, int y, int width, int height)
{
    if ((width <= 0) || (height <= 0))
        return;

    int x2 = x + width - 1;
    int y2 = y + height - 1;

    gray_vline(x, y, y2);
    gray_vline(x2, y, y2);
    gray_hline(x, x2, y);
    gray_hline(x, x2, y2);
}

/* Fill a rectangular area */
void gray_fillrect(int x, int y, int width, int height)
{
    int bits = 0;
    unsigned char *dst, *dst_end;
    bool fillopt = false;
    void (*pfunc)(unsigned char *address);

    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= _gray_info.width) 
        || (y >= _gray_info.height) || (x + width <= 0) || (y + height <= 0))
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
    if (x + width > _gray_info.width)
        width = _gray_info.width - x;
    if (y + height > _gray_info.height)
        height = _gray_info.height - y;
    
    if (_gray_info.drawmode & DRMODE_INVERSEVID)
    {
        if (_gray_info.drawmode & DRMODE_BG)
        {
            fillopt = true;
            bits = _gray_info.bg_index;
        }
    }
    else
    {
        if (_gray_info.drawmode & DRMODE_FG)
        {
            fillopt = true;
            bits = _gray_info.fg_index;
        }
    }
    pfunc = _gray_pixelfuncs[_gray_info.drawmode];
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
    dst   = &_gray_info.cur_buffer[_GRAY_MULUQ(y, _gray_info.width) + x];
    dst_end = dst + _GRAY_MULUQ(height, _gray_info.width);

    do
    {
        if (fillopt)
            _gray_rb->memset(dst, bits, width);
        else
        {
            unsigned char *dst_row = dst;
            unsigned char *row_end = dst_row + width;
            
            do
                pfunc(dst_row++);
            while (dst_row < row_end);
        }
        dst += _gray_info.width;
    }
    while (dst < dst_end);
#else
    dst   = &_gray_info.cur_buffer[_GRAY_MULUQ(x, _gray_info.height) + y];
    dst_end = dst + _GRAY_MULUQ(width, _gray_info.height);

    do
    {
        if (fillopt)
            _gray_rb->memset(dst, bits, height);
        else
        {
            unsigned char *dst_col = dst;
            unsigned char *col_end = dst_col + height;
            
            do
                pfunc(dst_col++);
            while (dst_col < col_end);
        }
        dst += _gray_info.height;
    }
    while (dst < dst_end);
#endif
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
void gray_mono_bitmap_part(const unsigned char *src, int src_x, int src_y,
                           int stride, int x, int y, int width, int height)
{
    const unsigned char *src_end;
    unsigned char *dst, *dst_end;
    void (*fgfunc)(unsigned char *address);
    void (*bgfunc)(unsigned char *address);

    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= _gray_info.width) 
        || (y >= _gray_info.height) || (x + width <= 0) || (y + height <= 0))
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
    if (x + width > _gray_info.width)
        width = _gray_info.width - x;
    if (y + height > _gray_info.height)
        height = _gray_info.height - y;

    src    += _GRAY_MULUQ(stride, src_y >> 3) + src_x; /* move starting point */
    src_y  &= 7;
    src_end = src + width;

    fgfunc = _gray_pixelfuncs[_gray_info.drawmode];
    bgfunc = _gray_pixelfuncs[_gray_info.drawmode ^ DRMODE_INVERSEVID];
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
    dst    = &_gray_info.cur_buffer[_GRAY_MULUQ(y, _gray_info.width) + x];
    
    do
    {
        const unsigned char *src_col = src++;
        unsigned char *dst_col = dst++;
        unsigned data = *src_col >> src_y;
        int numbits = 8 - src_y;
        
        dst_end = dst_col + _GRAY_MULUQ(height, _gray_info.width);
        do
        {
            if (data & 0x01)
                fgfunc(dst_col);
            else
                bgfunc(dst_col);

            dst_col += _gray_info.width;

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
#else /* LCD_PIXELFORMAT == VERTICAL_PACKING */
    dst    = &_gray_info.cur_buffer[_GRAY_MULUQ(x, _gray_info.height) + y];
    
    do
    {
        const unsigned char *src_col = src++;
        unsigned char *dst_col = dst;
        unsigned data = *src_col >> src_y;
        int numbits = 8 - src_y;
        
        dst_end = dst_col + height;
        do
        {
            if (data & 0x01)
                fgfunc(dst_col++);
            else
                bgfunc(dst_col++);

            data >>= 1;
            if (--numbits == 0)
            {
                src_col += stride;
                data = *src_col;
                numbits = 8;
            }
        }
        while (dst_col < dst_end);
        
        dst += _gray_info.height;
    }
    while (src < src_end);
#endif /* LCD_PIXELFORMAT */
}

/* Draw a full monochrome bitmap */
void gray_mono_bitmap(const unsigned char *src, int x, int y, int width, int height)
{
    gray_mono_bitmap_part(src, 0, 0, width, x, y, width, height);
}

/* Draw a partial greyscale bitmap, canonical format */
void gray_gray_bitmap_part(const unsigned char *src, int src_x, int src_y,
                           int stride, int x, int y, int width, int height)
{
    unsigned char *dst, *dst_end;

    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= _gray_info.width)
        || (y >= _gray_info.height) || (x + width <= 0) || (y + height <= 0))
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
    if (x + width > _gray_info.width)
        width = _gray_info.width - x;
    if (y + height > _gray_info.height)
        height = _gray_info.height - y;

    src    += _GRAY_MULUQ(stride, src_y) + src_x; /* move starting point */
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
    dst     = &_gray_info.cur_buffer[_GRAY_MULUQ(y, _gray_info.width) + x];
    dst_end = dst + _GRAY_MULUQ(height, _gray_info.width);

    do
    {
        const unsigned char *src_row = src;
        unsigned char *dst_row = dst;
        unsigned char *row_end = dst_row + width;

        do
            *dst_row++ = _gray_info.idxtable[*src_row++];
        while (dst_row < row_end);

        src += stride;
        dst += _gray_info.width;
    }
    while (dst < dst_end);
#else /* LCD_PIXELFORMAT == VERTICAL_PACKING */
    dst     = &_gray_info.cur_buffer[_GRAY_MULUQ(x, _gray_info.height) + y];
    dst_end = dst + height;

    do
    {
        const unsigned char *src_row = src;
        unsigned char *dst_row = dst++;
        unsigned char *row_end = dst_row + _GRAY_MULUQ(width, _gray_info.height);

        do
        {
            *dst_row = _gray_info.idxtable[*src_row++];
            dst_row += _gray_info.height;
        }
        while (dst_row < row_end);

        src += stride;
    }
    while (dst < dst_end);
#endif /* LCD_PIXELFORMAT */
}

/* Draw a full greyscale bitmap, canonical format */
void gray_gray_bitmap(const unsigned char *src, int x, int y, int width,
                      int height)
{
    gray_gray_bitmap_part(src, 0, 0, width, x, y, width, height);
}

/* Put a string at a given pixel position, skipping first ofs pixel columns */
void gray_putsxyofs(int x, int y, int ofs, const unsigned char *str)
{
    int ch;
    struct font* pf = _gray_rb->font_get(_gray_info.curfont);

    while ((ch = *str++) != '\0' && x < _gray_info.width)
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
               pf->offset[ch] : ((pf->height + 7) / 8 * pf->maxwidth * ch));

        gray_mono_bitmap_part(bits, ofs, 0, width, x, y, width - ofs, pf->height);
        
        x += width - ofs;
        ofs = 0;
    }
}

/* Put a string at a given pixel position */
void gray_putsxy(int x, int y, const unsigned char *str)
{
    gray_putsxyofs(x, y, 0, str);
}

/*** Unbuffered drawing functions ***/

#ifdef SIMULATOR

/* Clear the greyscale display (sets all pixels to white) */
void gray_ub_clear_display(void)
{
    _gray_rb->memset(_gray_info.cur_buffer, _gray_info.depth,
                     _GRAY_MULUQ(_gray_info.width, _gray_info.height));
    gray_update();
}

/* Draw a partial greyscale bitmap, canonical format */
void gray_ub_gray_bitmap_part(const unsigned char *src, int src_x, int src_y,
                              int stride, int x, int y, int width, int height)
{
    gray_gray_bitmap_part(src, src_x, src_y, stride, x, y, width, height);
    gray_update_rect(x, y, width, height);
}

#else /* !SIMULATOR */

/* Clear the greyscale display (sets all pixels to white) */
void gray_ub_clear_display(void)
{
    _gray_rb->memset(_gray_info.plane_data, 0, _GRAY_MULUQ(_gray_info.depth,
                     _gray_info.plane_size));
}

#if LCD_PIXELFORMAT == HORIZONTAL_PACKING

/* Write a pixel block, defined by their brightnesses in a greymap.
   Address is the byte in the first bitplane, src is the greymap start address,
   mask determines which pixels of the destination block are changed. */
static void _writearray(unsigned char *address, const unsigned char *src,
                        unsigned mask)
{
    unsigned long pat_stack[8];
    unsigned long *pat_ptr = &pat_stack[8];
    unsigned char *addr;
#ifdef CPU_ARM
    const unsigned char *_src;
    unsigned _mask, depth, trash;

    _mask = mask;
    _src = src;

    /* precalculate the bit patterns with random shifts 
       for all 8 pixels and put them on an extra "stack" */
    asm volatile 
    (
        "mov     %[mask], %[mask], lsl #24   \n"  /* shift mask to upper byte */
        "mov     r3, #8                      \n"  /* loop count */
        
    ".wa_loop:                               \n"  /** load pattern for pixel **/
        "mov     r2, #0                      \n"  /* pattern for skipped pixel must be 0 */
        "movs    %[mask], %[mask], lsl #1    \n"  /* shift out msb of mask */
        "bcc     .wa_skip                    \n"  /* skip this pixel */
        
        "ldrb    r0, [%[src]]                \n"  /* load src byte */
        "ldrb    r0, [%[trns], r0]           \n"  /* idxtable into pattern index */
        "ldr     r2, [%[bpat], r0, lsl #2]   \n"  /* r2 = bitpattern[byte]; */

        "add     %[rnd], %[rnd], %[rnd], lsl #2  \n"  /* multiply by 75 */
        "rsb     %[rnd], %[rnd], %[rnd], lsl #4  \n"
        "add     %[rnd], %[rnd], #74         \n"  /* add another 74 */
        /* Since the lower bits are not very random:   get bits 8..15 (need max. 5) */
        "and     r1, %[rmsk], %[rnd], lsr #8 \n"  /* ..and mask out unneeded bits */

        "cmp     r1, %[dpth]                 \n"  /* random >= depth ? */
        "subhs   r1, r1, %[dpth]             \n"  /* yes: random -= depth */

        "mov     r0, r2, lsl r1              \n"  /** rotate pattern **/
        "sub     r1, %[dpth], r1             \n"
        "orr     r2, r0, r2, lsr r1          \n"

    ".wa_skip:                               \n"
        "str     r2, [%[patp], #-4]!         \n"  /* push on pattern stack */

        "add     %[src], %[src], #1          \n"  /* src++; */
        "subs    r3, r3, #1                  \n"  /* loop 8 times (pixel block) */
        "bne     .wa_loop                    \n"
        : /* outputs */
        [src] "+r"(_src),
        [patp]"+r"(pat_ptr),
        [rnd] "+r"(_gray_random_buffer),
        [mask]"+r"(_mask)
        : /* inputs */
        [bpat]"r"(_gray_info.bitpattern),
        [trns]"r"(_gray_info.idxtable),
        [dpth]"r"(_gray_info.depth),
        [rmsk]"r"(_gray_info.randmask)
        : /* clobbers */
        "r0", "r1", "r2", "r3"
    );
    
    addr = address;
    _mask = mask;
    depth = _gray_info.depth;

    /* set the bits for all 8 pixels in all bytes according to the
     * precalculated patterns on the pattern stack */
    asm volatile 
    (
        "ldmia   %[patp], {r1 - r8}          \n"  /* pop all 8 patterns */

        /** Rotate the four 8x8 bit "blocks" within r1..r8 **/

        "mov     %[rx], #0xF0                \n"  /** Stage 1: 4 bit "comb" **/
        "orr     %[rx], %[rx], %[rx], lsl #8 \n"
        "orr     %[rx], %[rx], %[rx], lsl #16\n"  /* bitmask = ...11110000 */
        "eor     r0, r1, r5, lsl #4          \n"
        "and     r0, r0, %[rx]               \n"
        "eor     r1, r1, r0                  \n"  /* r1 = ...e3e2e1e0a3a2a1a0 */
        "eor     r5, r5, r0, lsr #4          \n"  /* r5 = ...e7e6e5e4a7a6a5a4 */
        "eor     r0, r2, r6, lsl #4          \n"
        "and     r0, r0, %[rx]               \n"
        "eor     r2, r2, r0                  \n"  /* r2 = ...f3f2f1f0b3b2b1b0 */
        "eor     r6, r6, r0, lsr #4          \n"  /* r6 = ...f7f6f5f4f7f6f5f4 */
        "eor     r0, r3, r7, lsl #4          \n"
        "and     r0, r0, %[rx]               \n"
        "eor     r3, r3, r0                  \n"  /* r3 = ...g3g2g1g0c3c2c1c0 */
        "eor     r7, r7, r0, lsr #4          \n"  /* r7 = ...g7g6g5g4c7c6c5c4 */
        "eor     r0, r4, r8, lsl #4          \n"
        "and     r0, r0, %[rx]               \n"
        "eor     r4, r4, r0                  \n"  /* r4 = ...h3h2h1h0d3d2d1d0 */
        "eor     r8, r8, r0, lsr #4          \n"  /* r8 = ...h7h6h5h4d7d6d5d4 */

        "mov     %[rx], #0xCC                \n"  /** Stage 2: 2 bit "comb" **/
        "orr     %[rx], %[rx], %[rx], lsl #8 \n"
        "orr     %[rx], %[rx], %[rx], lsl #16\n"  /* bitmask = ...11001100 */
        "eor     r0, r1, r3, lsl #2          \n"
        "and     r0, r0, %[rx]               \n"
        "eor     r1, r1, r0                  \n"  /* r1 = ...g1g0e1e0c1c0a1a0 */
        "eor     r3, r3, r0, lsr #2          \n"  /* r3 = ...g3g2e3e2c3c2a3a2 */
        "eor     r0, r2, r4, lsl #2          \n"
        "and     r0, r0, %[rx]               \n"
        "eor     r2, r2, r0                  \n"  /* r2 = ...h1h0f1f0d1d0b1b0 */
        "eor     r4, r4, r0, lsr #2          \n"  /* r4 = ...h3h2f3f2d3d2b3b2 */
        "eor     r0, r5, r7, lsl #2          \n"
        "and     r0, r0, %[rx]               \n"
        "eor     r5, r5, r0                  \n"  /* r5 = ...g5g4e5e4c5c4a5a4 */
        "eor     r7, r7, r0, lsr #2          \n"  /* r7 = ...g7g6e7e6c7c6a7a6 */
        "eor     r0, r6, r8, lsl #2          \n"
        "and     r0, r0, %[rx]               \n"
        "eor     r6, r6, r0                  \n"  /* r6 = ...h5h4f5f4d5d4b5b4 */
        "eor     r8, r8, r0, lsr #2          \n"  /* r8 = ...h7h6f7f6d7d6b7b6 */

        "mov     %[rx], #0xAA                \n"  /** Stage 3: 1 bit "comb" **/
        "orr     %[rx], %[rx], %[rx], lsl #8 \n"
        "orr     %[rx], %[rx], %[rx], lsl #16\n"  /* bitmask = ...10101010 */
        "eor     r0, r1, r2, lsl #1          \n"
        "and     r0, r0, %[rx]               \n"
        "eor     r1, r1, r0                  \n"  /* r1 = ...h0g0f0e0d0c0b0a0 */
        "eor     r2, r2, r0, lsr #1          \n"  /* r2 = ...h1g1f1e1d1c1b1a1 */
        "eor     r0, r3, r4, lsl #1          \n"
        "and     r0, r0, %[rx]               \n"
        "eor     r3, r3, r0                  \n"  /* r3 = ...h2g2f2e2d2c2b2a2 */
        "eor     r4, r4, r0, lsr #1          \n"  /* r4 = ...h3g3f3e3d3c3b3a3 */
        "eor     r0, r5, r6, lsl #1          \n"
        "and     r0, r0, %[rx]               \n"
        "eor     r5, r5, r0                  \n"  /* r5 = ...h4g4f4e4d4c4b4a4 */
        "eor     r6, r6, r0, lsr #1          \n"  /* r6 = ...h5g5f5e5d5c5b5a5 */
        "eor     r0, r7, r8, lsl #1          \n"
        "and     r0, r0, %[rx]               \n"
        "eor     r7, r7, r0                  \n"  /* r7 = ...h6g6f6e6d6c6b6a6 */
        "eor     r8, r8, r0, lsr #1          \n"  /* r8 = ...h7g7f7e7d7c7b7a7 */
        
        "sub     r0, %[dpth], #1             \n"  /** shift out unused low bytes **/
        "and     r0, r0, #7                  \n"
        "add     pc, pc, r0, lsl #2          \n"  /* jump into shift streak */
        "mov     r8, r8, lsr #8              \n"  /* r8: never reached */
        "mov     r7, r7, lsr #8              \n"
        "mov     r6, r6, lsr #8              \n"
        "mov     r5, r5, lsr #8              \n"
        "mov     r4, r4, lsr #8              \n"
        "mov     r3, r3, lsr #8              \n"
        "mov     r2, r2, lsr #8              \n"
        "mov     r1, r1, lsr #8              \n"

        "mvn     %[mask], %[mask]            \n"  /* "set" mask -> "keep" mask */
        "ands    %[mask], %[mask], #0xff     \n"
        "beq     .wa_sstart                  \n"  /* short loop if no bits to keep */

        "ldrb    r0, [pc, r0]                \n"  /* jump into full loop */
        "add     pc, pc, r0                  \n"
    ".wa_ftable:                             \n"
        ".byte   .wa_f1 - .wa_ftable - 4     \n"  /* [jump tables are tricky] */
        ".byte   .wa_f2 - .wa_ftable - 4     \n"
        ".byte   .wa_f3 - .wa_ftable - 4     \n"
        ".byte   .wa_f4 - .wa_ftable - 4     \n"
        ".byte   .wa_f5 - .wa_ftable - 4     \n"
        ".byte   .wa_f6 - .wa_ftable - 4     \n"
        ".byte   .wa_f7 - .wa_ftable - 4     \n"
        ".byte   .wa_f8 - .wa_ftable - 4     \n"

    ".wa_floop:                              \n"  /** full loop (bits to keep)**/
    ".wa_f8:                                 \n"
        "ldrb    r0, [%[addr]]               \n"  /* load old byte */
        "and     r0, r0, %[mask]             \n"  /* mask out replaced bits */
        "orr     r0, r0, r1                  \n"  /* set new bits */
        "strb    r0, [%[addr]], %[psiz]      \n"  /* store byte */
        "mov     r1, r1, lsr #8              \n"  /* shift out used-up byte */
    ".wa_f7:                                 \n"
        "ldrb    r0, [%[addr]]               \n"
        "and     r0, r0, %[mask]             \n"
        "orr     r0, r0, r2                  \n"
        "strb    r0, [%[addr]], %[psiz]      \n"
        "mov     r2, r2, lsr #8              \n"
    ".wa_f6:                                 \n"
        "ldrb    r0, [%[addr]]               \n"
        "and     r0, r0, %[mask]             \n"
        "orr     r0, r0, r3                  \n"
        "strb    r0, [%[addr]], %[psiz]      \n"
        "mov     r3, r3, lsr #8              \n"
    ".wa_f5:                                 \n"
        "ldrb    r0, [%[addr]]               \n"
        "and     r0, r0, %[mask]             \n"
        "orr     r0, r0, r4                  \n"
        "strb    r0, [%[addr]], %[psiz]      \n"
        "mov     r4, r4, lsr #8              \n"
    ".wa_f4:                                 \n"
        "ldrb    r0, [%[addr]]               \n"
        "and     r0, r0, %[mask]             \n"
        "orr     r0, r0, r5                  \n"
        "strb    r0, [%[addr]], %[psiz]      \n"
        "mov     r5, r5, lsr #8              \n"
    ".wa_f3:                                 \n"
        "ldrb    r0, [%[addr]]               \n"
        "and     r0, r0, %[mask]             \n"
        "orr     r0, r0, r6                  \n"
        "strb    r0, [%[addr]], %[psiz]      \n"
        "mov     r6, r6, lsr #8              \n"
    ".wa_f2:                                 \n"
        "ldrb    r0, [%[addr]]               \n"
        "and     r0, r0, %[mask]             \n"
        "orr     r0, r0, r7                  \n"
        "strb    r0, [%[addr]], %[psiz]      \n"
        "mov     r7, r7, lsr #8              \n"
    ".wa_f1:                                 \n"
        "ldrb    r0, [%[addr]]               \n"
        "and     r0, r0, %[mask]             \n"
        "orr     r0, r0, r8                  \n"
        "strb    r0, [%[addr]], %[psiz]      \n"
        "mov     r8, r8, lsr #8              \n"

        "subs    %[dpth], %[dpth], #8        \n"  /* next round if anything left */
        "bhi     .wa_floop                   \n"

        "b       .wa_end                     \n"

    ".wa_sstart:                             \n"
        "ldrb    r0, [pc, r0]                \n"  /* jump into short loop*/
        "add     pc, pc, r0                  \n"
    ".wa_stable:                             \n"
        ".byte   .wa_s1 - .wa_stable - 4     \n"
        ".byte   .wa_s2 - .wa_stable - 4     \n"
        ".byte   .wa_s3 - .wa_stable - 4     \n"
        ".byte   .wa_s4 - .wa_stable - 4     \n"
        ".byte   .wa_s5 - .wa_stable - 4     \n"
        ".byte   .wa_s6 - .wa_stable - 4     \n"
        ".byte   .wa_s7 - .wa_stable - 4     \n"
        ".byte   .wa_s8 - .wa_stable - 4     \n"

    ".wa_sloop:                              \n"  /** short loop (nothing to keep) **/
    ".wa_s8:                                 \n"
        "strb    r1, [%[addr]], %[psiz]      \n"  /* store byte */
        "mov     r1, r1, lsr #8              \n"  /* shift out used-up byte */
    ".wa_s7:                                 \n"
        "strb    r2, [%[addr]], %[psiz]      \n"
        "mov     r2, r2, lsr #8              \n"
    ".wa_s6:                                 \n"
        "strb    r3, [%[addr]], %[psiz]      \n"
        "mov     r3, r3, lsr #8              \n"
    ".wa_s5:                                 \n"
        "strb    r4, [%[addr]], %[psiz]      \n"
        "mov     r4, r4, lsr #8              \n"
    ".wa_s4:                                 \n"
        "strb    r5, [%[addr]], %[psiz]      \n"
        "mov     r5, r5, lsr #8              \n"
    ".wa_s3:                                 \n"
        "strb    r6, [%[addr]], %[psiz]      \n"
        "mov     r6, r6, lsr #8              \n"
    ".wa_s2:                                 \n"
        "strb    r7, [%[addr]], %[psiz]      \n"
        "mov     r7, r7, lsr #8              \n"
    ".wa_s1:                                 \n"
        "strb    r8, [%[addr]], %[psiz]      \n"
        "mov     r8, r8, lsr #8              \n"

        "subs    %[dpth], %[dpth], #8        \n"  /* next round if anything left */
        "bhi     .wa_sloop                   \n"

    ".wa_end:                                \n"
        : /* outputs */
        [addr]"+r"(addr),
        [mask]"+r"(_mask),
        [dpth]"+r"(depth),
        [rx]  "=&r"(trash)
        : /* inputs */
        [psiz]"r"(_gray_info.plane_size),
        [patp]"[rx]"(pat_ptr)
        : /* clobbers */
        "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8"
    );
#else /* C version, for reference*/
#warning C version of _writearray() used
    unsigned char *end;
    unsigned test = 0x80;
    int i;

    /* precalculate the bit patterns with random shifts
     * for all 8 pixels and put them on an extra "stack" */
    for (i = 7; i >= 0; i--)
    {
        unsigned pat = 0;

        if (mask & test)
        {
            int shift;

            pat = _gray_info.bitpattern[_gray_info.idxtable[*src]];

            /* shift pattern pseudo-random, simple & fast PRNG */
            _gray_random_buffer = 75 * _gray_random_buffer + 74;
            shift = (_gray_random_buffer >> 8) & _gray_info.randmask;
            if (shift >= _gray_info.depth)
                shift -= _gray_info.depth;

            pat = (pat << shift) | (pat >> (_gray_info.depth - shift));
        }
        *(--pat_ptr) = pat;
        src++;
        test >>= 1;
    }

    addr = address;
    end = addr + _GRAY_MULUQ(_gray_info.depth, _gray_info.plane_size);

    /* set the bits for all 8 pixels in all bytes according to the
     * precalculated patterns on the pattern stack */
    test = 1 << ((-_gray_info.depth) & 7);
    mask = (~mask & 0xff);
    if (mask == 0)
    {
        do
        {
            unsigned data = 0;

            for (i = 7; i >= 0; i--)
                data = (data << 1) | ((pat_stack[i] & test) ? 1 : 0);

            *addr = data;
            addr += _gray_info.plane_size;
            test <<= 1;
        }
        while (addr < end);
    }
    else
    {
        do
        {
            unsigned data = 0;

            for (i = 7; i >= 0; i--)
                data = (data << 1) | ((pat_stack[i] & test) ? 1 : 0);

            *addr = (*addr & mask) | data;
            addr += _gray_info.plane_size;
            test <<= 1;
        }
        while (addr < end);
    }
#endif
}

/* Draw a partial greyscale bitmap, canonical format */
void gray_ub_gray_bitmap_part(const unsigned char *src, int src_x, int src_y,
                              int stride, int x, int y, int width, int height)
{
    int shift, nx;
    unsigned char *dst, *dst_end;
    unsigned mask, mask_right;

    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= _gray_info.width)
        || (y >= _gray_info.height) || (x + width <= 0) || (y + height <= 0))
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
    if (x + width > _gray_info.width)
        width = _gray_info.width - x;
    if (y + height > _gray_info.height)
        height = _gray_info.height - y;

    shift = x & 7;
    src += _GRAY_MULUQ(stride, src_y) + src_x - shift;
    dst  = _gray_info.plane_data + (x >> 3) + _GRAY_MULUQ(_gray_info.bwidth, y);
    nx   = width - 1 + shift;

    mask = 0xFFu >> shift;
    mask_right = 0xFFu << (~nx & 7);
    
    dst_end = dst + _GRAY_MULUQ(_gray_info.bwidth, height);
    do
    {
        const unsigned char *src_row = src;
        unsigned char *dst_row = dst;
        unsigned mask_row = mask;
        
        for (x = nx; x >= 8; x -= 8)
        {
            _writearray(dst_row++, src_row, mask_row);
            src_row += 8;
            mask_row = 0xFFu;
        }
        _writearray(dst_row, src_row, mask_row & mask_right);

        src += stride;
        dst += _gray_info.bwidth;
    }
    while (dst < dst_end);
}
#else /* LCD_PIXELFORMAT == VERTICAL_PACKING */

/* Write a pixel block, defined by their brightnesses in a greymap.
   Address is the byte in the first bitplane, src is the greymap start address,
   stride is the increment for the greymap to get to the next pixel, mask
   determines which pixels of the destination block are changed. */
static void _writearray(unsigned char *address, const unsigned char *src,
                        int stride, unsigned mask) __attribute__((noinline));
static void _writearray(unsigned char *address, const unsigned char *src,
                        int stride, unsigned mask)
{
    unsigned long pat_stack[8];
    unsigned long *pat_ptr = &pat_stack[8];
    unsigned char *addr;
#if CONFIG_CPU == SH7034
    const unsigned char *_src;
    unsigned _mask, depth, trash;

    _mask = mask;
    _src = src;

    /* precalculate the bit patterns with random shifts
       for all 8 pixels and put them on an extra "stack" */
    asm volatile 
    (
        "mov     #8, r3              \n"  /* loop count */

    ".wa_loop:                       \n"  /** load pattern for pixel **/
        "mov     #0, r0              \n"  /* pattern for skipped pixel must be 0 */
        "shlr    %[mask]             \n"  /* shift out lsb of mask */
        "bf      .wa_skip            \n"  /* skip this pixel */

        "mov.b   @%[src], r0         \n"  /* load src byte */
        "mov     #75, r1             \n"
        "extu.b  r0, r0              \n"  /* extend unsigned */
        "mov.b   @(r0,%[trns]), r0   \n"  /* idxtable into pattern index */
        "mulu    r1, %[rnd]          \n"  /* multiply by 75 */
        "extu.b  r0, r0              \n"  /* extend unsigned */
        "shll2   r0                  \n"
        "mov.l   @(r0,%[bpat]), r4   \n"  /* r4 = bitpattern[byte]; */
        "sts     macl, %[rnd]        \n"
        "add     #74, %[rnd]         \n"  /* add another 74 */
        /* Since the lower bits are not very random: */
        "swap.b  %[rnd], r1          \n"  /* get bits 8..15 (need max. 5) */
        "and     %[rmsk], r1         \n"  /* mask out unneeded bits */

        "cmp/hs  %[dpth], r1         \n"  /* random >= depth ? */
        "bf      .wa_ntrim           \n"
        "sub     %[dpth], r1         \n"  /* yes: random -= depth; */
    ".wa_ntrim:                      \n"

        "mov.l   .ashlsi3, r0        \n"  /** rotate pattern **/
        "jsr     @r0                 \n"  /* r4 -> r0, shift left by r5 */
        "mov     r1, r5              \n"

        "mov     %[dpth], r5         \n"
        "sub     r1, r5              \n"  /* r5 = depth - r1 */
        "mov.l   .lshrsi3, r1        \n"
        "jsr     @r1                 \n"  /* r4 -> r0, shift right by r5 */
        "mov     r0, r1              \n"  /* store previous result in r1 */

        "or      r1, r0              \n"  /* rotated_pattern = r0 | r1 */

    ".wa_skip:                       \n"
        "mov.l   r0, @-%[patp]       \n"  /* push on pattern stack */

        "add     %[stri], %[src]     \n"  /* src += stride; */
        "add     #-1, r3             \n"  /* loop 8 times (pixel block) */
        "cmp/pl  r3                  \n"
        "bt      .wa_loop            \n"
        : /* outputs */
        [src] "+r"(_src),
        [rnd] "+r"(_gray_random_buffer),
        [patp]"+r"(pat_ptr),
        [mask]"+r"(_mask)
        : /* inputs */
        [stri]"r"(stride),
        [dpth]"r"(_gray_info.depth),
        [bpat]"r"(_gray_info.bitpattern),
        [rmsk]"r"(_gray_info.randmask),
        [trns]"r"(_gray_info.idxtable)
        : /* clobbers */
        "r0", "r1", "r3", "r4", "r5", "macl", "pr"
    );

    addr = address;
    _mask = mask;
    depth = _gray_info.depth;

    /* set the bits for all 8 pixels in all bytes according to the
     * precalculated patterns on the pattern stack */
    asm volatile 
    (
        "mov.l   @%[patp]+, r8       \n"  /* pop all 8 patterns */
        "mov.l   @%[patp]+, r7       \n"
        "mov.l   @%[patp]+, r6       \n"
        "mov.l   @%[patp]+, r5       \n"
        "mov.l   @%[patp]+, r4       \n"
        "mov.l   @%[patp]+, r3       \n"
        "mov.l   @%[patp]+, r2       \n"
        "mov.l   @%[patp], r1        \n"

        /** Rotate the four 8x8 bit "blocks" within r1..r8 **/
                                          
        "mov.l   .wa_mask4, %[rx]    \n"  /* bitmask = ...11110000 */
        "mov     r5, r0              \n"  /** Stage 1: 4 bit "comb" **/
        "shll2   r0                  \n"
        "shll2   r0                  \n"
        "xor     r1, r0              \n"
        "and     %[rx], r0           \n"
        "xor     r0, r1              \n"  /* r1 = ...e3e2e1e0a3a2a1a0 */
        "shlr2   r0                  \n"
        "shlr2   r0                  \n"
        "xor     r0, r5              \n"  /* r5 = ...e7e6e5e4a7a6a5a4 */
        "mov     r6, r0              \n"
        "shll2   r0                  \n"
        "shll2   r0                  \n"
        "xor     r2, r0              \n"
        "and     %[rx], r0           \n"
        "xor     r0, r2              \n"  /* r2 = ...f3f2f1f0b3b2b1b0 */
        "shlr2   r0                  \n"
        "shlr2   r0                  \n"
        "xor     r0, r6              \n"  /* r6 = ...f7f6f5f4f7f6f5f4 */
        "mov     r7, r0              \n"
        "shll2   r0                  \n"
        "shll2   r0                  \n"
        "xor     r3, r0              \n"
        "and     %[rx], r0           \n"
        "xor     r0, r3              \n"  /* r3 = ...g3g2g1g0c3c2c1c0 */
        "shlr2   r0                  \n"
        "shlr2   r0                  \n"
        "xor     r0, r7              \n"  /* r7 = ...g7g6g5g4c7c6c5c4 */
        "mov     r8, r0              \n"
        "shll2   r0                  \n"
        "shll2   r0                  \n"
        "xor     r4, r0              \n"
        "and     %[rx], r0           \n"
        "xor     r0, r4              \n"  /* r4 = ...h3h2h1h0d3d2d1d0 */
        "shlr2   r0                  \n"
        "shlr2   r0                  \n"
        "xor     r0, r8              \n"  /* r8 = ...h7h6h5h4d7d6d5d4 */

        "mov.l   .wa_mask2, %[rx]    \n"  /* bitmask = ...11001100 */
        "mov     r3, r0              \n"  /** Stage 2: 2 bit "comb" **/
        "shll2   r0                  \n"
        "xor     r1, r0              \n"
        "and     %[rx], r0           \n"
        "xor     r0, r1              \n"  /* r1 = ...g1g0e1e0c1c0a1a0 */
        "shlr2   r0                  \n"
        "xor     r0, r3              \n"  /* r3 = ...g3g2e3e2c3c2a3a2 */
        "mov     r4, r0              \n"
        "shll2   r0                  \n"
        "xor     r2, r0              \n"
        "and     %[rx], r0           \n"
        "xor     r0, r2              \n"  /* r2 = ...h1h0f1f0d1d0b1b0 */
        "shlr2   r0                  \n"
        "xor     r0, r4              \n"  /* r4 = ...h3h2f3f2d3d2b3b2 */
        "mov     r7, r0              \n"
        "shll2   r0                  \n"
        "xor     r5, r0              \n"
        "and     %[rx], r0           \n"
        "xor     r0, r5              \n"  /* r5 = ...g5g4e5e4c5c4a5a4 */
        "shlr2   r0                  \n"
        "xor     r0, r7              \n"  /* r7 = ...g7g6e7e6c7c6a7a6 */
        "mov     r8, r0              \n"
        "shll2   r0                  \n"
        "xor     r6, r0              \n"
        "and     %[rx], r0           \n"
        "xor     r0, r6              \n"  /* r6 = ...h5h4f5f4d5d4b5b4 */
        "shlr2   r0                  \n"
        "xor     r0, r8              \n"  /* r8 = ...h7h6f7f6d7d6b7b6 */

        "mov.l   .wa_mask1, %[rx]    \n"  /* bitmask = ...10101010 */
        "mov     r2, r0              \n"  /** Stage 3: 1 bit "comb" **/
        "shll    r0                  \n"
        "xor     r1, r0              \n"
        "and     %[rx], r0           \n"
        "xor     r0, r1              \n"  /* r1 = ...h0g0f0e0d0c0b0a0 */
        "shlr    r0                  \n"
        "xor     r0, r2              \n"  /* r2 = ...h1g1f1e1d1c1b1a1 */
        "mov     r4, r0              \n"
        "shll    r0                  \n"
        "xor     r3, r0              \n"
        "and     %[rx], r0           \n"
        "xor     r0, r3              \n"  /* r3 = ...h2g2f2e2d2c2b2a2 */
        "shlr    r0                  \n"
        "xor     r0, r4              \n"  /* r4 = ...h3g3f3e3d3c3b3a3 */
        "mov     r6, r0              \n"
        "shll    r0                  \n"
        "xor     r5, r0              \n"
        "and     %[rx], r0           \n"
        "xor     r0, r5              \n"  /* r5 = ...h4g4f4e4d4c4b4a4 */
        "shlr    r0                  \n"
        "xor     r0, r6              \n"  /* r6 = ...h5g5f5e5d5c5b5a5 */
        "mov     r8, r0              \n"
        "shll    r0                  \n"
        "xor     r7, r0              \n"
        "and     %[rx], r0           \n"
        "xor     r0, r7              \n"  /* r7 = ...h6g6f6e6d6c6b6a6 */
        "shlr    r0                  \n"
        "xor     r0, r8              \n"  /* r8 = ...h7g7f7e7d7c7b7a7 */
        
        "mov     %[dpth], %[rx]      \n"  /** shift out unused low bytes **/
        "add     #-1, %[rx]          \n"
        "mov     #7, r0              \n"
        "and     r0, %[rx]           \n"
        "mova    .wa_pshift, r0      \n"
        "add     %[rx], r0           \n"
        "add     %[rx], r0           \n"
        "jmp     @r0                 \n"  /* jump into shift streak */
        "nop                         \n"

        ".align  2                   \n"
    ".wa_pshift:                     \n"
        "shlr8   r7                  \n"
        "shlr8   r6                  \n"
        "shlr8   r5                  \n"
        "shlr8   r4                  \n"
        "shlr8   r3                  \n"
        "shlr8   r2                  \n"
        "shlr8   r1                  \n"

        "not     %[mask], %[mask]    \n"  /* "set" mask -> "keep" mask */
        "extu.b  %[mask], %[mask]    \n"  /* mask out high bits */
        "tst     %[mask], %[mask]    \n"
        "bt      .wa_sstart          \n"  /* short loop if nothing to keep */
        
        "mova    .wa_ftable, r0      \n"  /* jump into full loop */
        "mov.b   @(r0, %[rx]), %[rx] \n"
        "add     %[rx], r0           \n"
        "jmp     @r0                 \n"
        "nop                         \n"

        ".align  2                   \n"
    ".wa_ftable:                     \n"
        ".byte   .wa_f1 - .wa_ftable \n"
        ".byte   .wa_f2 - .wa_ftable \n"
        ".byte   .wa_f3 - .wa_ftable \n"
        ".byte   .wa_f4 - .wa_ftable \n"
        ".byte   .wa_f5 - .wa_ftable \n"
        ".byte   .wa_f6 - .wa_ftable \n"
        ".byte   .wa_f7 - .wa_ftable \n"
        ".byte   .wa_f8 - .wa_ftable \n"

    ".wa_floop:                      \n"  /** full loop (there are bits to keep)**/
    ".wa_f8:                         \n"
        "mov.b   @%[addr], r0        \n"  /* load old byte */
        "and     %[mask], r0         \n"  /* mask out replaced bits */
        "or      r1, r0              \n"  /* set new bits */
        "mov.b   r0, @%[addr]        \n"  /* store byte */
        "add     %[psiz], %[addr]    \n"
        "shlr8   r1                  \n"  /* shift out used-up byte */
    ".wa_f7:                         \n"
        "mov.b   @%[addr], r0        \n"
        "and     %[mask], r0         \n"
        "or      r2, r0              \n"
        "mov.b   r0, @%[addr]        \n"
        "add     %[psiz], %[addr]    \n"
        "shlr8   r2                  \n"
    ".wa_f6:                         \n"
        "mov.b   @%[addr], r0        \n"
        "and     %[mask], r0         \n"
        "or      r3, r0              \n"
        "mov.b   r0, @%[addr]        \n"
        "add     %[psiz], %[addr]    \n"
        "shlr8   r3                  \n"
    ".wa_f5:                         \n"
        "mov.b   @%[addr], r0        \n"
        "and     %[mask], r0         \n"
        "or      r4, r0              \n"
        "mov.b   r0, @%[addr]        \n"
        "add     %[psiz], %[addr]    \n"
        "shlr8   r4                  \n"
    ".wa_f4:                         \n"
        "mov.b   @%[addr], r0        \n"
        "and     %[mask], r0         \n"
        "or      r5, r0              \n"
        "mov.b   r0, @%[addr]        \n"
        "add     %[psiz], %[addr]    \n"
        "shlr8   r5                  \n"
    ".wa_f3:                         \n"
        "mov.b   @%[addr], r0        \n"
        "and     %[mask], r0         \n"
        "or      r6, r0              \n"
        "mov.b   r0, @%[addr]        \n"
        "add     %[psiz], %[addr]    \n"
        "shlr8   r6                  \n"
    ".wa_f2:                         \n"
        "mov.b   @%[addr], r0        \n"
        "and     %[mask], r0         \n"
        "or      r7, r0              \n"
        "mov.b   r0, @%[addr]        \n"
        "add     %[psiz], %[addr]    \n"
        "shlr8   r7                  \n"
    ".wa_f1:                         \n"
        "mov.b   @%[addr], r0        \n"
        "and     %[mask], r0         \n"
        "or      r8, r0              \n"
        "mov.b   r0, @%[addr]        \n"
        "add     %[psiz], %[addr]    \n"
        "shlr8   r8                  \n"

        "add     #-8, %[dpth]        \n"
        "cmp/pl  %[dpth]             \n"  /* next round if anything left */
        "bt      .wa_floop           \n"

        "bra     .wa_end             \n"
        "nop                         \n"

        /* References to C library routines used in the precalc block */
        ".align  2                   \n"
    ".ashlsi3:                       \n"  /* C library routine: */
        ".long   ___ashlsi3          \n"  /* shift r4 left by r5, result in r0 */
    ".lshrsi3:                       \n"  /* C library routine: */
        ".long   ___lshrsi3          \n"  /* shift r4 right by r5, result in r0 */
        /* both routines preserve r4, destroy r5 and take ~16 cycles */

        /* Bitmasks for the bit block rotation */
    ".wa_mask4:                      \n"
        ".long   0xF0F0F0F0          \n"
    ".wa_mask2:                      \n"
        ".long   0xCCCCCCCC          \n"
    ".wa_mask1:                      \n"
        ".long   0xAAAAAAAA          \n"

    ".wa_sstart:                     \n"
        "mova    .wa_stable, r0      \n"  /* jump into short loop */
        "mov.b   @(r0, %[rx]), %[rx] \n"
        "add     %[rx], r0           \n"
        "jmp     @r0                 \n"
        "nop                         \n"

        ".align  2                   \n"
    ".wa_stable:                     \n"
        ".byte   .wa_s1 - .wa_stable \n"
        ".byte   .wa_s2 - .wa_stable \n"
        ".byte   .wa_s3 - .wa_stable \n"
        ".byte   .wa_s4 - .wa_stable \n"
        ".byte   .wa_s5 - .wa_stable \n"
        ".byte   .wa_s6 - .wa_stable \n"
        ".byte   .wa_s7 - .wa_stable \n"
        ".byte   .wa_s8 - .wa_stable \n"

    ".wa_sloop:                      \n"  /** short loop (nothing to keep) **/
    ".wa_s8:                         \n"
        "mov.b   r1, @%[addr]        \n"  /* store byte */
        "add     %[psiz], %[addr]    \n"
        "shlr8   r1                  \n"  /* shift out used-up byte */
    ".wa_s7:                         \n"
        "mov.b   r2, @%[addr]        \n"
        "add     %[psiz], %[addr]    \n"
        "shlr8   r2                  \n"
    ".wa_s6:                         \n"
        "mov.b   r3, @%[addr]        \n"
        "add     %[psiz], %[addr]    \n"
        "shlr8   r3                  \n"
    ".wa_s5:                         \n"
        "mov.b   r4, @%[addr]        \n"
        "add     %[psiz], %[addr]    \n"
        "shlr8   r4                  \n"
    ".wa_s4:                         \n"
        "mov.b   r5, @%[addr]        \n"
        "add     %[psiz], %[addr]    \n"
        "shlr8   r5                  \n"
    ".wa_s3:                         \n"
        "mov.b   r6, @%[addr]        \n"
        "add     %[psiz], %[addr]    \n"
        "shlr8   r6                  \n"
    ".wa_s2:                         \n"
        "mov.b   r7, @%[addr]        \n"
        "add     %[psiz], %[addr]    \n"
        "shlr8   r7                  \n"
    ".wa_s1:                         \n"
        "mov.b   r8, @%[addr]        \n"
        "add     %[psiz], %[addr]    \n"
        "shlr8   r8                  \n"

        "add     #-8, %[dpth]        \n"
        "cmp/pl  %[dpth]             \n"  /* next round if anything left */
        "bt      .wa_sloop           \n"

    ".wa_end:                        \n"
        : /* outputs */
        [addr]"+r"(addr),
        [mask]"+r"(_mask),
        [dpth]"+r"(depth),
        [rx]  "=&r"(trash)
        : /* inputs */
        [psiz]"r"(_gray_info.plane_size),
        [patp]"[rx]"(pat_ptr)
        : /* clobbers */
        "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "macl"
    );
#elif defined(CPU_COLDFIRE)
    const unsigned char *_src;
    unsigned _mask, depth, trash;

    _mask = mask;
    _src = src;

    /* precalculate the bit patterns with random shifts 
       for all 8 pixels and put them on an extra "stack" */
    asm volatile 
    (
        "moveq.l #8, %%d3            \n"  /* loop count */

    ".wa_loop:                       \n"  /** load pattern for pixel **/
        "clr.l   %%d2                \n"  /* pattern for skipped pixel must be 0 */
        "lsr.l   #1, %[mask]         \n"  /* shift out lsb of mask */
        "bcc.b   .wa_skip            \n"  /* skip this pixel */

        "clr.l   %%d0                \n"
        "move.b  (%[src]), %%d0      \n"  /* load src byte */
        "move.b  (%%d0:l:1, %[trns]), %%d0   \n" /* idxtable into pattern index */
        "move.l  (%%d0:l:4, %[bpat]), %%d2   \n" /* d2 = bitpattern[byte]; */

        "mulu.w  #75, %[rnd]         \n"  /* multiply by 75 */
        "add.l   #74, %[rnd]         \n"  /* add another 74 */
        /* Since the lower bits are not very random: */
        "move.l  %[rnd], %%d1        \n"
        "lsr.l   #8, %%d1            \n"  /* get bits 8..15 (need max. 5) */
        "and.l   %[rmsk], %%d1       \n"  /* mask out unneeded bits */

        "cmp.l   %[dpth], %%d1       \n"  /* random >= depth ? */
        "blo.b   .wa_ntrim           \n"
        "sub.l   %[dpth], %%d1       \n"  /* yes: random -= depth; */
    ".wa_ntrim:                      \n"

        "move.l  %%d2, %%d0          \n"  /** rotate pattern **/
        "lsl.l   %%d1, %%d0          \n"
        "sub.l   %[dpth], %%d1       \n"
        "neg.l   %%d1                \n"  /* d1 = depth - d1 */
        "lsr.l   %%d1, %%d2          \n"
        "or.l    %%d0, %%d2          \n"

    ".wa_skip:                       \n"
        "move.l  %%d2, -(%[patp])    \n"  /* push on pattern stack */

        "add.l   %[stri], %[src]     \n"  /* src += stride; */
        "subq.l  #1, %%d3            \n"  /* loop 8 times (pixel block) */
        "bne.b   .wa_loop            \n"
        : /* outputs */
        [src] "+a"(_src),
        [patp]"+a"(pat_ptr),
        [rnd] "+d"(_gray_random_buffer),
        [mask]"+d"(_mask)
        : /* inputs */
        [stri]"r"(stride),
        [bpat]"a"(_gray_info.bitpattern),
        [trns]"a"(_gray_info.idxtable),
        [dpth]"d"(_gray_info.depth),
        [rmsk]"d"(_gray_info.randmask)
        : /* clobbers */
        "d0", "d1", "d2", "d3"
    );

    addr = address;
    _mask = ~mask & 0xff;
    depth = _gray_info.depth;

    /* set the bits for all 8 pixels in all bytes according to the
     * precalculated patterns on the pattern stack */
    asm volatile
    (
        "movem.l (%[patp]), %%d1-%%d7/%%a0   \n"  /* pop all 8 patterns */
        /* move.l  %%d5, %[ax]        */  /* need %%d5 as workspace, but not yet */

        /** Rotate the four 8x8 bit "blocks" within r1..r8 **/

        "move.l  %%d1, %%d0          \n"  /** Stage 1: 4 bit "comb" **/
        "lsl.l   #4, %%d0            \n"
        /* move.l  %[ax], %%d5        */  /* already in d5 */
        "eor.l   %%d5, %%d0          \n"
        "and.l   #0xF0F0F0F0, %%d0   \n"  /* bitmask = ...11110000 */
        "eor.l   %%d0, %%d5          \n"
        "move.l  %%d5, %[ax]         \n"  /* ax = ...h3h2h1h0d3d2d1d0 */
        "lsr.l   #4, %%d0            \n"
        "eor.l   %%d0, %%d1          \n"  /* d1 = ...h7h6h5h4d7d6d5d4 */
        "move.l  %%d2, %%d0          \n"
        "lsl.l   #4, %%d0            \n"
        "eor.l   %%d6, %%d0          \n"
        "and.l   #0xF0F0F0F0, %%d0   \n"
        "eor.l   %%d0, %%d6          \n"  /* d6 = ...g3g2g1g0c3c2c1c0 */
        "lsr.l   #4, %%d0            \n"
        "eor.l   %%d0, %%d2          \n"  /* d2 = ...g7g6g5g4c7c6c5c4 */
        "move.l  %%d3, %%d0          \n"
        "lsl.l   #4, %%d0            \n"
        "eor.l   %%d7, %%d0          \n"
        "and.l   #0xF0F0F0F0, %%d0   \n"
        "eor.l   %%d0, %%d7          \n"  /* d7 = ...f3f2f1f0b3b2b1b0 */
        "lsr.l   #4, %%d0            \n"
        "eor.l   %%d0, %%d3          \n"  /* d3 = ...f7f6f5f4f7f6f5f4 */
        "move.l  %%d4, %%d0          \n"
        "lsl.l   #4, %%d0            \n"
        "move.l  %%a0, %%d5          \n"
        "eor.l   %%d5, %%d0          \n"
        "and.l   #0xF0F0F0F0, %%d0   \n"
        "eor.l   %%d0, %%d5          \n"  /* (a0 = ...e3e2e1e0a3a2a1a0) */
        /* move.l  %%d5, %%a0         */  /* but d5 is kept until next usage */
        "lsr.l   #4, %%d0            \n"
        "eor.l   %%d0, %%d4          \n"  /* d4 = ...e7e6e5e4a7a6a5a4 */
        
        "move.l  %%d6, %%d0          \n"  /** Stage 2: 2 bit "comb" **/
        "lsl.l   #2, %%d0            \n"
        /* move.l  %%a0, %%d5         */  /* still in d5 */
        "eor.l   %%d5, %%d0          \n"
        "and.l   #0xCCCCCCCC, %%d0   \n"  /* bitmask = ...11001100 */
        "eor.l   %%d0, %%d5          \n"
        "move.l  %%d5, %%a0          \n"  /* a0 = ...g1g0e1e0c1c0a1a0 */
        "lsr.l   #2, %%d0            \n"
        "eor.l   %%d0, %%d6          \n"  /* d6 = ...g3g2e3e2c3c2a3a2 */
        "move.l  %[ax], %%d5         \n"
        "move.l  %%d5, %%d0          \n"
        "lsl.l   #2, %%d0            \n"
        "eor.l   %%d7, %%d0          \n"
        "and.l   #0xCCCCCCCC, %%d0   \n"
        "eor.l   %%d0, %%d7          \n"  /* r2 = ...h1h0f1f0d1d0b1b0 */
        "lsr.l   #2, %%d0            \n"
        "eor.l   %%d0, %%d5          \n"  /* (ax = ...h3h2f3f2d3d2b3b2) */
        /* move.l  %%d5, %[ax]        */  /* but d5 is kept until next usage */
        "move.l  %%d2, %%d0          \n"
        "lsl.l   #2, %%d0            \n"
        "eor.l   %%d4, %%d0          \n"
        "and.l   #0xCCCCCCCC, %%d0   \n"
        "eor.l   %%d0, %%d4          \n"  /* d4 = ...g5g4e5e4c5c4a5a4 */
        "lsr.l   #2, %%d0            \n"
        "eor.l   %%d0, %%d2          \n"  /* d2 = ...g7g6e7e6c7c6a7a6 */
        "move.l  %%d1, %%d0          \n"
        "lsl.l   #2, %%d0            \n"
        "eor.l   %%d3, %%d0          \n"
        "and.l   #0xCCCCCCCC, %%d0   \n"
        "eor.l   %%d0, %%d3          \n"  /* d3 = ...h5h4f5f4d5d4b5b4 */
        "lsr.l   #2, %%d0            \n"
        "eor.l   %%d0, %%d1          \n"  /* d1 = ...h7h6f7f6d7d6b7b6 */
        
        "move.l  %%d1, %%d0          \n"  /** Stage 3: 1 bit "comb" **/
        "lsl.l   #1, %%d0            \n"
        "eor.l   %%d2, %%d0          \n"
        "and.l   #0xAAAAAAAA, %%d0   \n"  /* bitmask = ...10101010 */
        "eor.l   %%d0, %%d2          \n"  /* d2 = ...h6g6f6e6d6c6b6a6 */
        "lsr.l   #1, %%d0            \n"
        "eor.l   %%d0, %%d1          \n"  /* d1 = ...h7g7f7e7d7c7b7a7 */
        "move.l  %%d3, %%d0          \n"
        "lsl.l   #1, %%d0            \n"
        "eor.l   %%d4, %%d0          \n"
        "and.l   #0xAAAAAAAA, %%d0   \n"
        "eor.l   %%d0, %%d4          \n"  /* d4 = ...h4g4f4e4d4c4b4a4 */
        "lsr.l   #1, %%d0            \n"
        "eor.l   %%d0, %%d3          \n"  /* d3 = ...h5g5f5e5d5c5b5a5 */
        /* move.l  %[ax], %%d5        */  /* still in d5 */
        "move.l  %%d5, %%d0          \n"
        "lsl.l   #1, %%d0            \n"
        "eor.l   %%d6, %%d0          \n"
        "and.l   #0xAAAAAAAA, %%d0   \n"
        "eor.l   %%d0, %%d6          \n"  /* d6 = ...h2g2f2e2d2c2b2a2 */
        "lsr.l   #1, %%d0            \n"
        "eor.l   %%d0, %%d5          \n"
        "move.l  %%d5, %[ax]         \n"  /* ax = ...h3g3f3e3d3c3b3a3 */
        "move.l  %%d7, %%d0          \n"
        "lsl.l   #1, %%d0            \n"
        "move.l  %%a0, %%d5          \n"
        "eor.l   %%d5, %%d0          \n"
        "and.l   #0xAAAAAAAA, %%d0   \n"
        "eor.l   %%d0, %%d5          \n"  /* (a0 = ...h0g0f0e0d0c0b0a0) */
        /* move.l  %%d5, %%a0         */  /*  but keep in d5 for shift streak */
        "lsr.l   #1, %%d0            \n"
        "eor.l   %%d0, %%d7          \n"  /* d7 = ...h1g1f1e1d1c1b1a1 */
        
        "move.l  %[dpth], %%d0       \n"  /** shift out unused low bytes **/
        "subq.l  #1, %%d0            \n"
        "and.l   #7, %%d0            \n"
        "move.l  %%d0, %%a0          \n"
        "move.l  %[ax], %%d0         \n"  /* all data in D registers */
        "jmp     (2, %%pc, %%a0:l:2) \n"  /* jump into shift streak */
        "lsr.l   #8, %%d2            \n"
        "lsr.l   #8, %%d3            \n"
        "lsr.l   #8, %%d4            \n"
        "lsr.l   #8, %%d0            \n"
        "lsr.l   #8, %%d6            \n"
        "lsr.l   #8, %%d7            \n"
        "lsr.l   #8, %%d5            \n"
        "move.l  %%d0, %[ax]         \n"  /* put the 2 extra words back.. */
        "move.l  %%a0, %%d0          \n"  /* keep the value for later */
        "move.l  %%d5, %%a0          \n"  /* ..into their A registers */

        "tst.l   %[mask]             \n"
        "jeq     .wa_sstart          \n"  /* short loop if nothing to keep */

        "move.l  %[mask], %%d5       \n"  /* need mask in data reg. */
        "move.l  %%d1, %[mask]       \n"  /* free d1 as working reg. */

        "jmp     (2, %%pc, %%d0:l:2) \n"  /* jump into full loop */
        "bra.s   .wa_f1              \n"
        "bra.s   .wa_f2              \n"
        "bra.s   .wa_f3              \n"
        "bra.s   .wa_f4              \n"
        "bra.s   .wa_f5              \n"
        "bra.s   .wa_f6              \n"
        "bra.s   .wa_f7              \n"
        /* bra.s   .wa_f8             */  /* identical with target */

    ".wa_floop:                      \n"  /** full loop (there are bits to keep)**/
    ".wa_f8:                         \n"
        "move.b  (%[addr]), %%d0     \n"  /* load old byte */
        "and.l   %%d5, %%d0          \n"  /* mask out replaced bits */
        "move.l  %%a0, %%d1          \n"
        "or.l    %%d1, %%d0          \n"  /* set new bits */
        "move.b  %%d0, (%[addr])     \n"  /* store byte */
        "add.l   %[psiz], %[addr]    \n"
        "lsr.l   #8, %%d1            \n"  /* shift out used-up byte */
        "move.l  %%d1, %%a0          \n"
    ".wa_f7:                         \n"
        "move.b  (%[addr]), %%d0     \n"
        "and.l   %%d5, %%d0          \n"
        "or.l    %%d7, %%d0          \n"
        "move.b  %%d0, (%[addr])     \n"
        "add.l   %[psiz], %[addr]    \n"
        "lsr.l   #8, %%d7            \n"
    ".wa_f6:                         \n"
        "move.b  (%[addr]), %%d0     \n"
        "and.l   %%d5, %%d0          \n"
        "or.l    %%d6, %%d0          \n"
        "move.b  %%d0, (%[addr])     \n"
        "add.l   %[psiz], %[addr]    \n"
        "lsr.l   #8, %%d6            \n"
    ".wa_f5:                         \n"
        "move.b  (%[addr]), %%d0     \n"
        "and.l   %%d5, %%d0          \n"
        "move.l  %[ax], %%d1         \n"
        "or.l    %%d1, %%d0          \n"
        "move.b  %%d0, (%[addr])     \n"
        "add.l   %[psiz], %[addr]    \n"
        "lsr.l   #8, %%d1            \n"
        "move.l  %%d1, %[ax]         \n"
    ".wa_f4:                         \n"
        "move.b  (%[addr]), %%d0     \n"
        "and.l   %%d5, %%d0          \n"
        "or.l    %%d4, %%d0          \n"
        "move.b  %%d0, (%[addr])     \n"
        "add.l   %[psiz], %[addr]    \n"
        "lsr.l   #8, %%d4            \n"
    ".wa_f3:                         \n"
        "move.b  (%[addr]), %%d0     \n"
        "and.l   %%d5, %%d0          \n"
        "or.l    %%d3, %%d0          \n"
        "move.b  %%d0, (%[addr])     \n"
        "add.l   %[psiz], %[addr]    \n"
        "lsr.l   #8, %%d3            \n"
    ".wa_f2:                         \n"
        "move.b  (%[addr]), %%d0     \n"
        "and.l   %%d5, %%d0          \n"
        "or.l    %%d2, %%d0          \n"
        "move.b  %%d0, (%[addr])     \n"
        "add.l   %[psiz], %[addr]    \n"
        "lsr.l   #8, %%d2            \n"
    ".wa_f1:                         \n"
        "move.b  (%[addr]), %%d0     \n"
        "and.l   %%d5, %%d0          \n"
        "move.l  %[mask], %%d1       \n"
        "or.l    %%d1, %%d0          \n"
        "move.b  %%d0, (%[addr])     \n"
        "add.l   %[psiz], %[addr]    \n"
        "lsr.l   #8, %%d1            \n"
        "move.l  %%d1, %[mask]       \n"

        "subq.l  #8, %[dpth]         \n"
        "tst.l   %[dpth]             \n"  /* subq doesn't set flags for A reg */
        "jgt     .wa_floop           \n"  /* next round if anything left */

        "jra     .wa_end             \n"

    ".wa_sstart:                     \n"
        "jmp     (2, %%pc, %%d0:l:2) \n"  /* jump into short loop */
        "bra.s   .wa_s1              \n"
        "bra.s   .wa_s2              \n"
        "bra.s   .wa_s3              \n"
        "bra.s   .wa_s4              \n"
        "bra.s   .wa_s5              \n"
        "bra.s   .wa_s6              \n"
        "bra.s   .wa_s7              \n"
        /* bra.s   .wa_s8             */  /* identical with target */

    ".wa_sloop:                      \n"  /** short loop (nothing to keep) **/
    ".wa_s8:                         \n"
        "move.l  %%a0, %%d5          \n"
        "move.b  %%d5, (%[addr])     \n"  /* store byte */
        "add.l   %[psiz], %[addr]    \n"
        "lsr.l   #8, %%d5            \n"  /* shift out used-up byte */
        "move.l  %%d5, %%a0          \n"
    ".wa_s7:                         \n"
        "move.b  %%d7, (%[addr])     \n"
        "add.l   %[psiz], %[addr]    \n"
        "lsr.l   #8, %%d7            \n"
    ".wa_s6:                         \n"
        "move.b  %%d6, (%[addr])     \n"
        "add.l   %[psiz], %[addr]    \n"
        "lsr.l   #8, %%d6            \n"
    ".wa_s5:                         \n"
        "move.l  %[ax], %%d5         \n"
        "move.b  %%d5, (%[addr])     \n"
        "add.l   %[psiz], %[addr]    \n"
        "lsr.l   #8, %%d5            \n"
        "move.l  %%d5, %[ax]         \n"
    ".wa_s4:                         \n"
        "move.b  %%d4, (%[addr])     \n"
        "add.l   %[psiz], %[addr]    \n"
        "lsr.l   #8, %%d4            \n"
    ".wa_s3:                         \n"
        "move.b  %%d3, (%[addr])     \n"
        "add.l   %[psiz], %[addr]    \n"
        "lsr.l   #8, %%d3            \n"
    ".wa_s2:                         \n"
        "move.b  %%d2, (%[addr])     \n"
        "add.l   %[psiz], %[addr]    \n"
        "lsr.l   #8, %%d2            \n"
    ".wa_s1:                         \n"
        "move.b  %%d1, (%[addr])     \n"
        "add.l   %[psiz], %[addr]    \n"
        "lsr.l   #8, %%d1            \n"

        "subq.l  #8, %[dpth]         \n"
        "tst.l   %[dpth]             \n"  /* subq doesn't set flags for A reg */
        "jgt     .wa_sloop           \n"  /* next round if anything left */

    ".wa_end:                        \n"
        : /* outputs */
        [addr]"+a"(addr),
        [dpth]"+a"(depth),
        [mask]"+a"(_mask),
        [ax]  "=&a"(trash)
        : /* inputs */
        [psiz]"a"(_gray_info.plane_size),
        [patp]"[ax]"(pat_ptr)
        : /* clobbers */
        "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "a0"
    );
#else /* C version, for reference*/
#warning C version of _writearray() used
    unsigned char *end;
    unsigned test = 1;
    int i;

    /* precalculate the bit patterns with random shifts
     * for all 8 pixels and put them on an extra "stack" */
    for (i = 7; i >= 0; i--)
    {
        unsigned pat = 0;

        if (mask & test)
        {
            int shift;

            pat = _gray_info.bitpattern[_gray_info.idxtable[*src]];

            /* shift pattern pseudo-random, simple & fast PRNG */
            _gray_random_buffer = 75 * _gray_random_buffer + 74;
            shift = (_gray_random_buffer >> 8) & _gray_info.randmask;
            if (shift >= _gray_info.depth)
                shift -= _gray_info.depth;

            pat = (pat << shift) | (pat >> (_gray_info.depth - shift));
        }
        *(--pat_ptr) = pat;
        src += stride;
        test <<= 1;
    }

    addr = address;
    end = addr + _GRAY_MULUQ(_gray_info.depth, _gray_info.plane_size);

    /* set the bits for all 8 pixels in all bytes according to the
     * precalculated patterns on the pattern stack */
    test = 1 << ((-_gray_info.depth) & 7);
    mask = (~mask & 0xff);
    if (mask == 0)
    {
        do
        {
            unsigned data = 0;

            for (i = 0; i < 8; i++)
                data = (data << 1) | ((pat_stack[i] & test) ? 1 : 0);

            *addr = data;
            addr += _gray_info.plane_size;
            test <<= 1;
        }
        while (addr < end);
    }
    else
    {
        do
        {
            unsigned data = 0;

            for (i = 0; i < 8; i++)
                data = (data << 1) | ((pat_stack[i] & test) ? 1 : 0);

            *addr = (*addr & mask) | data;
            addr += _gray_info.plane_size;
            test <<= 1;
        }
        while (addr < end);
    }
#endif
}

/* Draw a partial greyscale bitmap, canonical format */
void gray_ub_gray_bitmap_part(const unsigned char *src, int src_x, int src_y,
                              int stride, int x, int y, int width, int height)
{
    int shift, ny;
    unsigned char *dst, *dst_end;
    unsigned mask, mask_bottom;

    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= _gray_info.width)
        || (y >= _gray_info.height) || (x + width <= 0) || (y + height <= 0))
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
    if (x + width > _gray_info.width)
        width = _gray_info.width - x;
    if (y + height > _gray_info.height)
        height = _gray_info.height - y;

    shift = y & 7;
    src += _GRAY_MULUQ(stride, src_y) + src_x - _GRAY_MULUQ(stride, shift);
    dst  = _gray_info.plane_data + x
           + _GRAY_MULUQ(_gray_info.width, y >> 3);
    ny   = height - 1 + shift;

    mask = 0xFFu << shift;
    mask_bottom = 0xFFu >> (~ny & 7);

    for (; ny >= 8; ny -= 8)
    {
        const unsigned char *src_row = src;
        unsigned char *dst_row = dst;

        dst_end = dst_row + width;
        do
            _writearray(dst_row++, src_row++, stride, mask);
        while (dst_row < dst_end);

        src += stride << 3;
        dst += _gray_info.width;
        mask = 0xFFu;
    }
    mask &= mask_bottom;
    dst_end = dst + width;
    do
        _writearray(dst++, src++, stride, mask);
    while (dst < dst_end);
}
#endif /* LCD_PIXELFORMAT */

#endif /* !SIMULATOR */

/* Draw a full greyscale bitmap, canonical format */
void gray_ub_gray_bitmap(const unsigned char *src, int x, int y, int width,
                         int height)
{
    gray_ub_gray_bitmap_part(src, 0, 0, width, x, y, width, height);
}

#endif /* HAVE_LCD_BITMAP */
