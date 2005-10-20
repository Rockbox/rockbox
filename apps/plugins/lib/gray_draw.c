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
* This is a generic framework to use grayscale display within Rockbox
* plugins. It obviously does not work for the player.
*
* Copyright (C) 2004-2005 Jens Arnold
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#ifndef SIMULATOR /* not for simulator by now */
#include "plugin.h"

#ifdef HAVE_LCD_BITMAP /* and also not for the Player */
#include "gray.h"

/*** low-level drawing functions ***/

static void setpixel(unsigned char *address)
{
    *address = _gray_info.fg_brightness;
}

static void clearpixel(unsigned char *address)
{
    *address = _gray_info.bg_brightness;
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
                     _gray_info.fg_brightness : _gray_info.bg_brightness;

    _gray_rb->memset(_gray_info.cur_buffer, brightness,
                     MULU16(_gray_info.width, _gray_info.height));
}

/* Set a single pixel */
void gray_drawpixel(int x, int y)
{
    if (((unsigned)x < (unsigned)_gray_info.width)
        && ((unsigned)y < (unsigned)_gray_info.height))
        _gray_pixelfuncs[_gray_info.drawmode](&_gray_info.cur_buffer[MULU16(x,
                                               _gray_info.height) + y]);
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
            pfunc(&_gray_info.cur_buffer[MULU16(x, _gray_info.height) + y]);

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
    dst   = &_gray_info.cur_buffer[MULU16(x1, _gray_info.height) + y];

    dst_end = dst + MULU16(x2 - x1, _gray_info.height);
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
    int y, bits;
    unsigned char *dst;
    bool fillopt;
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

    bits = _gray_info.fg_brightness;
    fillopt = (_gray_info.drawmode & DRMODE_INVERSEVID) ?
              (_gray_info.drawmode & DRMODE_BG) :
              (_gray_info.drawmode & DRMODE_FG);
    if (fillopt &&(_gray_info.drawmode & DRMODE_INVERSEVID))
        bits = _gray_info.bg_brightness;

    pfunc = _gray_pixelfuncs[_gray_info.drawmode];
    dst   = &_gray_info.cur_buffer[MULU16(x, _gray_info.height) + y1];

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
    int bits;
    unsigned char *dst, *dst_end;
    bool fillopt;
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
    
    bits = _gray_info.fg_brightness;
    fillopt = (_gray_info.drawmode & DRMODE_INVERSEVID) ?
              (_gray_info.drawmode & DRMODE_BG) :
              (_gray_info.drawmode & DRMODE_FG);
    if (fillopt &&(_gray_info.drawmode & DRMODE_INVERSEVID))
        bits = _gray_info.bg_brightness;

    pfunc = _gray_pixelfuncs[_gray_info.drawmode];
    dst   = &_gray_info.cur_buffer[MULU16(x, _gray_info.height) + y];
    dst_end = dst + MULU16(width, _gray_info.height);

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

/* About Rockbox' internal monochrome bitmap format:
 *
 * A bitmap contains one bit for every pixel that defines if that pixel is
 * foreground (1) or background (0). Bits within a byte are arranged
 * vertically, LSB at top.
 * The bytes are stored in row-major order, with byte 0 being top left,
 * byte 1 2nd from left etc. The first row of bytes defines pixel rows
 * 0..7, the second row defines pixel row 8..15 etc.
 *
 * This is similar to the internal lcd hw format. */

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

    src    += MULU16(stride, src_y >> 3) + src_x; /* move starting point */
    src_y  &= 7;
    src_end = src + width;

    dst    = &_gray_info.cur_buffer[MULU16(x, _gray_info.height) + y];
    fgfunc = _gray_pixelfuncs[_gray_info.drawmode];
    bgfunc = _gray_pixelfuncs[_gray_info.drawmode ^ DRMODE_INVERSEVID];
    
    do
    {
        const unsigned char *src_col = src++;
        unsigned char *dst_col = dst;
        unsigned char data = *src_col >> src_y;
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
    const unsigned char *src_end;
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

    src    += MULU16(stride, src_y) + src_x; /* move starting point */
    src_end = src + width;
    dst     = &_gray_info.cur_buffer[MULU16(x, _gray_info.height) + y];

    do
    {
        const unsigned char *src_col = src++;
        unsigned char *dst_col = dst;

        dst_end = dst_col + height;
        do
        {
            unsigned data = MULU16(_gray_info.depth, *src_col) + 127;
            *dst_col++ = (data + (data >> 8)) >> 8; /* approx. data / 255 */
            src_col += stride;
        }
        while (dst_col < dst_end);

        dst += _gray_info.height;
    }
    while (src < src_end);
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
    struct font* pf = font_get(_gray_info.curfont);

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

/* Clear the greyscale display (sets all pixels to white) */
void gray_ub_clear_display(void)
{
    _gray_rb->memset(_gray_info.plane_data, 0, MULU16(_gray_info.depth,
                     _gray_info.plane_size));
}

/* Write a pixel block, defined by their brightnesses in a greymap.
   Address is the byte in the first bitplane, src is the greymap start address,
   stride is the increment for the greymap to get to the next pixel, mask
   determines which pixels of the destination block are changed. */
static void _writearray(unsigned char *address, const unsigned char *src,
                        int stride, unsigned mask)
{
#if (CONFIG_CPU == SH7034) && (LCD_DEPTH == 1)
    unsigned long pat_stack[8];
    unsigned long *pat_ptr = &pat_stack[8];
    const unsigned char *_src;
    unsigned char *addr, *end;
    unsigned _mask, trash;

    _mask = mask;
    _src = src;

    /* precalculate the bit patterns with random shifts 
       for all 8 pixels and put them on an extra "stack" */
    asm volatile (
        "mov     #8,r3       \n"  /* loop count in r3: 8 pixels */

    ".wa_loop:               \n"  /** load pattern for pixel **/
        "mov     #0,r0       \n"  /* pattern for skipped pixel must be 0 */
        "shlr    %[mask]     \n"  /* shift out lsb of mask */
        "bf      .wa_skip    \n"  /* skip this pixel */

        "mov.b   @%[src],r0  \n"  /* load src byte */
        "extu.b  r0,r0       \n"  /* extend unsigned */
        "mulu    %[dpth],r0  \n"  /* macl = byte * depth; */
        "sts     macl,r1     \n"  /* r1 = macl; */
        "add     #127,r1     \n"  /* byte += 127; */
        "mov     r1,r0       \n"
        "shlr8   r1          \n"
        "add     r1,r0       \n"  /* byte += byte >> 8; */
        "shlr8   r0          \n"  /* byte >>= 8; */
        "shll2   r0          \n"
        "mov.l   @(r0,%[bpat]),r4\n"  /* r4 = bitpattern[byte]; */

        "mov     #75,r0      \n"
        "mulu    r0,%[rnd]   \n"  /* multiply by 75 */
        "sts     macl,%[rnd] \n"
        "add     #74,%[rnd]  \n"  /* add another 74 */
        /* Since the lower bits are not very random: */
        "swap.b  %[rnd],r1   \n"  /* get bits 8..15 (need max. 5) */
        "and     %[rmsk],r1  \n"  /* mask out unneeded bits */

        "cmp/hs  %[dpth],r1  \n"  /* random >= depth ? */
        "bf      .wa_ntrim   \n"
        "sub     %[dpth],r1  \n"  /* yes: random -= depth; */
    ".wa_ntrim:              \n"

        "mov.l   .ashlsi3,r0 \n"  /** rotate pattern **/
        "jsr     @r0         \n"  /* r4 -> r0, shift left by r5 */
        "mov     r1,r5       \n"

        "mov     %[dpth],r5  \n"
        "sub     r1,r5       \n"  /* r5 = depth - r1 */
        "mov.l   .lshrsi3,r1 \n"
        "jsr     @r1         \n"  /* r4 -> r0, shift right by r5 */
        "mov     r0,r1       \n"  /* store previous result in r1 */

        "or      r1,r0       \n"  /* rotated_pattern = r0 | r1 */

    ".wa_skip:               \n"
        "mov.l   r0,@-%[patp]\n"  /* push on pattern stack */

        "add     %[stri],%[src]  \n"  /* src += stride; */
        "add     #-1,r3      \n"  /* decrease loop count */
        "cmp/pl  r3          \n"  /* loop count > 0? */
        "bt      .wa_loop    \n"  /* yes: loop */
        : /* outputs */
        [src] "+r"(_src),
        [rnd] "+r"(_gray_random_buffer),
        [patp]"+r"(pat_ptr),
        [mask]"+r"(_mask)
        : /* inputs */
        [stri]"r"(stride),
        [dpth]"r"(_gray_info.depth),
        [bpat]"r"(_gray_info.bitpattern),
        [rmsk]"r"(_gray_info.randmask)
        : /* clobbers */
        "r0", "r1", "r3", "r4", "r5", "macl", "pr"
    );

    addr = address;
    end = addr + MULU16(_gray_info.depth, _gray_info.plane_size);
    _mask = mask;

    /* set the bits for all 8 pixels in all bytes according to the
     * precalculated patterns on the pattern stack */
    asm volatile (
        "mov.l   @%[patp]+,r1\n"  /* pop all 8 patterns */
        "mov.l   @%[patp]+,r2\n"
        "mov.l   @%[patp]+,r3\n"
        "mov.l   @%[patp]+,r6\n"
        "mov.l   @%[patp]+,r7\n"
        "mov.l   @%[patp]+,r8\n"
        "mov.l   @%[patp]+,r9\n"
        "mov.l   @%[patp],r10\n"

        "not     %[mask],%[mask] \n"  /* "set" mask -> "keep" mask */
        "extu.b  %[mask],%[mask] \n"  /* mask out high bits */
        "tst     %[mask],%[mask] \n"  /* nothing to keep? */
        "bt      .wa_sloop   \n"  /* yes: jump to short loop */

    ".wa_floop:              \n"  /** full loop (there are bits to keep)**/
        "shlr    r1          \n"  /* rotate lsb of pattern 1 to t bit */
        "rotcl   r0          \n"  /* rotate t bit into r0 */
        "shlr    r2          \n"
        "rotcl   r0          \n"
        "shlr    r3          \n"
        "rotcl   r0          \n"
        "shlr    r6          \n"
        "rotcl   r0          \n"
        "shlr    r7          \n"
        "rotcl   r0          \n"
        "shlr    r8          \n"
        "rotcl   r0          \n"
        "shlr    r9          \n"
        "rotcl   r0          \n"
        "shlr    r10         \n"
        "mov.b   @%[addr],%[rx]  \n"  /* read old value */
        "rotcl   r0          \n"
        "and     %[mask],%[rx]   \n"  /* mask out unneeded bits */
        "or      %[rx],r0    \n"  /* set new bits */
        "mov.b   r0,@%[addr] \n"  /* store value to bitplane */
        "add     %[psiz],%[addr] \n"  /* advance to next bitplane */
        "cmp/hi  %[addr],%[end]  \n"  /* last bitplane done? */
        "bt      .wa_floop   \n"  /* no: loop */

        "bra     .wa_end     \n"
        "nop                 \n"

    ".wa_sloop:              \n"  /** short loop (nothing to keep) **/
        "shlr    r1          \n"  /* rotate lsb of pattern 1 to t bit */
        "rotcl   r0          \n"  /* rotate t bit into r0 */
        "shlr    r2          \n"
        "rotcl   r0          \n"
        "shlr    r3          \n"
        "rotcl   r0          \n"
        "shlr    r6          \n"
        "rotcl   r0          \n"
        "shlr    r7          \n"
        "rotcl   r0          \n"
        "shlr    r8          \n"
        "rotcl   r0          \n"
        "shlr    r9          \n"
        "rotcl   r0          \n"
        "shlr    r10         \n"
        "rotcl   r0          \n"
        "mov.b   r0,@%[addr] \n"  /* store byte to bitplane */
        "add     %[psiz],%[addr] \n"  /* advance to next bitplane */
        "cmp/hi  %[addr],%[end]  \n"  /* last bitplane done? */
        "bt      .wa_sloop   \n"  /* no: loop */

    ".wa_end:                \n"
        : /* outputs */
        [addr]"+r"(addr),
        [mask]"+r"(_mask),
        [rx]  "=&r"(trash)
        : /* inputs */
        [psiz]"r"(_gray_info.plane_size),
        [end] "r"(end),
        [patp]"[rx]"(pat_ptr)
        : /* clobbers */
        "r0", "r1", "r2", "r3", "r6", "r7", "r8", "r9", "r10"
    );
#elif defined(CPU_COLDFIRE) && (LCD_DEPTH == 2)
    unsigned long pat_stack[8];
    unsigned long *pat_ptr = &pat_stack[8];
    const unsigned char *_src;
    unsigned char *addr, *end;
    unsigned _mask, trash;

    _mask = mask;
    _src = src;

    /* precalculate the bit patterns with random shifts 
       for all 8 pixels and put them on an extra "stack" */
    asm volatile (
        "moveq.l #8,%%d3     \n"  /* loop count in d3: 8 pixels */

    ".wa_loop:               \n"  /** load pattern for pixel **/
        "clr.l   %%d2        \n"  /* pattern for skipped pixel must be 0 */
        "lsr.l   #1,%[mask]  \n"  /* shift out lsb of mask */
        "bcc.b   .wa_skip    \n"  /* skip this pixel */

        "clr.l   %%d0        \n"
        "move.b  (%[src]),%%d0   \n"  /* load src byte */
        "mulu.w  %[dpth],%%d0\n"  /* byte = byte * depth; */
        "add.l   #127,%%d0   \n"  /* byte += 127; */
        "move.l  %%d0,%%d1   \n"
        "lsr.l   #8,%%d1     \n"
        "add.l   %%d1,%%d0   \n"  /* byte += byte >> 8; */
        "lsr.l   #8,%%d0     \n"  /* byte >>= 8; */
        "move.l  (%%d0:l:4,%[bpat]),%%d2\n" /* d2 = bitpattern[byte]; */

        "mulu.w  #75,%[rnd]  \n"  /* multiply by 75 */
        "add.l   #74,%[rnd]  \n"  /* add another 74 */
        /* Since the lower bits are not very random: */
        "move.l  %[rnd],%%d1 \n"
        "lsr.l   #8,%%d1     \n"  /* get bits 8..15 (need max. 5) */
        "and.l   %[rmsk],%%d1\n"  /* mask out unneeded bits */

        "cmp.l   %[dpth],%%d1\n"  /* random >= depth ? */
        "blo.b   .wa_ntrim   \n"
        "sub.l   %[dpth],%%d1\n"  /* yes: random -= depth; */
    ".wa_ntrim:              \n"

        "move.l  %%d2,%%d0   \n"
        "lsl.l   %%d1,%%d0   \n"
        "sub.l   %[dpth],%%d1\n"
        "neg.l   %%d1        \n"  /* d1 = depth - d1 */
        "lsr.l   %%d1,%%d2   \n"
        "or.l    %%d0,%%d2   \n"

    ".wa_skip:               \n"
        "move.l  %%d2,-(%[patp]) \n"  /* push on pattern stack */

        "add.l   %[stri],%[src]  \n"  /* src += stride; */
        "subq.l  #1,%%d3     \n"  /* decrease loop count */
        "bne.b   .wa_loop    \n"  /* yes: loop */
        : /* outputs */
        [src] "+a"(_src),
        [patp]"+a"(pat_ptr),
        [rnd] "+d"(_gray_random_buffer),
        [mask]"+d"(_mask)
        : /* inputs */
        [stri]"r"(stride),
        [bpat]"a"(_gray_info.bitpattern),
        [dpth]"d"(_gray_info.depth),
        [rmsk]"d"(_gray_info.randmask)
        : /* clobbers */
        "d0", "d1", "d2", "d3"
    );

    addr = address;
    end = addr + MULU16(_gray_info.depth, _gray_info.plane_size);
    _mask = mask;

    /* set the bits for all 8 pixels in all bytes according to the
     * precalculated patterns on the pattern stack */
    asm volatile (
        "movem.l (%[patp]),%%d2-%%d6/%%a0-%%a1/%[ax] \n"  
                                  /* pop all 8 patterns */
        "not.l   %[mask]     \n"  /* "set" mask -> "keep" mask */
        "and.l   #0xFF,%[mask]   \n"
        "beq.b   .wa_sstart  \n"  /* yes: jump to short loop */

    ".wa_floop:              \n"  /** full loop (there are bits to keep)**/
        "clr.l   %%d0        \n"
        "lsr.l   #1,%%d2     \n"  /* shift out mask bit */
        "addx.l  %%d0,%%d0   \n"  /* puts bit into LSB, shifts left by 1 */
        "lsr.l   #1,%%d3     \n"
        "addx.l  %%d0,%%d0   \n"
        "lsr.l   #1,%%d4     \n"
        "addx.l  %%d0,%%d0   \n"
        "lsr.l   #1,%%d5     \n"
        "addx.l  %%d0,%%d0   \n"
        "lsr.l   #1,%%d6     \n"
        "addx.l  %%d0,%%d0   \n"
        "move.l  %%a0,%%d1   \n"
        "lsr.l   #1,%%d1     \n"
        "addx.l  %%d0,%%d0   \n"
        "move.l  %%d1,%%a0   \n"
        "move.l  %%a1,%%d1   \n"
        "lsr.l   #1,%%d1     \n"
        "addx.l  %%d0,%%d0   \n"
        "move.l  %%d1,%%a1   \n"
        "move.l  %[ax],%%d1  \n"
        "lsr.l   #1,%%d1     \n"
        "addx.l  %%d0,%%d0   \n"
        "move.l  %%d1,%[ax]  \n"

        "move.b  (%[addr]),%%d1  \n"  /* read old value */
        "and.l   %[mask],%%d1    \n"  /* mask out unneeded bits */
        "or.l    %%d0,%%d1       \n"  /* set new bits */
        "move.b  %%d1,(%[addr])  \n"  /* store value to bitplane */

        "add.l   %[psiz],%[addr] \n"  /* advance to next bitplane */
        "cmp.l   %[addr],%[end]  \n"  /* last bitplane done? */
        "bhi.b   .wa_floop   \n"  /* no: loop */

        "bra.b   .wa_end     \n"
        
    ".wa_sstart:             \n"
        "move.l  %%a0,%[mask]\n"  /* mask isn't needed here, reuse reg */

    ".wa_sloop:              \n"  /** short loop (nothing to keep) **/
        "clr.l   %%d0        \n"
        "lsr.l   #1,%%d2     \n"  /* shift out mask bit */
        "addx.l  %%d0,%%d0   \n"  /* puts bit into LSB, shifts left by 1 */
        "lsr.l   #1,%%d3     \n"
        "addx.l  %%d0,%%d0   \n"
        "lsr.l   #1,%%d4     \n"
        "addx.l  %%d0,%%d0   \n"
        "lsr.l   #1,%%d5     \n"
        "addx.l  %%d0,%%d0   \n"
        "lsr.l   #1,%%d6     \n"
        "addx.l  %%d0,%%d0   \n"
        "lsr.l   #1,%[mask]  \n"
        "addx.l  %%d0,%%d0   \n"
        "move.l  %%a1,%%d1   \n"
        "lsr.l   #1,%%d1     \n"
        "addx.l  %%d0,%%d0   \n"
        "move.l  %%d1,%%a1   \n"
        "move.l  %[ax],%%d1  \n"
        "lsr.l   #1,%%d1     \n"
        "addx.l  %%d0,%%d0   \n"
        "move.l  %%d1,%[ax]  \n"

        "move.b  %%d0,(%[addr])  \n"  /* store byte to bitplane */
        "add.l   %[psiz],%[addr] \n"  /* advance to next bitplane */
        "cmp.l   %[addr],%[end]  \n"  /* last bitplane done? */
        "bhi.b   .wa_sloop   \n"  /* no: loop */

    ".wa_end:                \n"
        : /* outputs */
        [addr]"+a"(addr),
        [mask]"+d"(_mask),
        [ax]  "=&a"(trash)
        : /* inputs */
        [psiz]"a"(_gray_info.plane_size),
        [end] "a"(end),
        [patp]"[ax]"(pat_ptr)
        : /* clobbers */
        "d0", "d1", "d2", "d3", "d4", "d5", "d6", "a0", "a1"
    );
#endif
}

#if CONFIG_CPU == SH7034
/* References to C library routines used in _writearray() */
asm (
    ".align  2           \n"
".ashlsi3:               \n"  /* C library routine: */
    ".long   ___ashlsi3  \n"  /* shift r4 left by r5, return in r0 */
".lshrsi3:               \n"  /* C library routine: */
    ".long   ___lshrsi3  \n"  /* shift r4 right by r5, return in r0 */
    /* both routines preserve r4, destroy r5 and take ~16 cycles */
);
#endif

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

    shift = y & (_PBLOCK-1);
    src += MULU16(stride, src_y) + src_x - MULU16(stride, shift);
    dst  = _gray_info.plane_data + x
           + MULU16(_gray_info.width, y >> _PBLOCK_EXP);
    ny   = height - 1 + shift;

    mask = 0xFFu << shift;
    mask_bottom = 0xFFu >> (~ny & (_PBLOCK-1));

    for (; ny >= _PBLOCK; ny -= _PBLOCK)
    {
        const unsigned char *src_row = src;
        unsigned char *dst_row = dst;

        dst_end = dst_row + width;
        do
            _writearray(dst_row++, src_row++, stride, mask);
        while (dst_row < dst_end);

        src += stride << _PBLOCK_EXP;
        dst += _gray_info.width;
        mask = 0xFFu;
    }
    mask &= mask_bottom;
    dst_end = dst + width;
    do
        _writearray(dst++, src++, stride, mask);
    while (dst < dst_end);
}

/* Draw a full greyscale bitmap, canonical format */
void gray_ub_gray_bitmap(const unsigned char *src, int x, int y, int width,
                         int height)
{
    gray_ub_gray_bitmap_part(src, 0, 0, width, x, y, width, height);
}


#endif /* HAVE_LCD_BITMAP */
#endif /* !SIMULATOR */

