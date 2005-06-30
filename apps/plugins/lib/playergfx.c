/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Bitmap graphics on player LCD!
*
* Copyright (C) 2005 Jens Arnold
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#include "plugin.h"

#ifdef HAVE_LCD_CHARCELLS /* Player only :) */
#include "playergfx.h"

/*** globals ***/

static struct plugin_api *pgfx_rb = NULL; /* global api struct pointer */
static int char_width;
static int char_height;
static int pixel_height;
static int pixel_width;
static unsigned char gfx_chars[8];
static unsigned char gfx_buffer[56];
static int drawmode = DRMODE_SOLID;

/*** Special functions ***/

/* library init */
bool pgfx_init(struct plugin_api* newrb, int cwidth, int cheight)
{
    int i;

    if (((unsigned) cwidth * (unsigned) cheight) > 8 || (unsigned) cheight > 2)
        return false;

    pgfx_rb = newrb;
    char_width = cwidth;
    char_height = cheight;
    pixel_height = 7 * char_height;
    pixel_width = 5 * char_width;

    for (i = 0; i < cwidth * cheight; i++)
    {
        if ((gfx_chars[i] = pgfx_rb->lcd_get_locked_pattern()) == 0)
        {
            pgfx_release();
            return false;
        }
    }

    return true;
}

/* library deinit */
void pgfx_release(void)
{
    int i;
    
    for (i = 0; i < 8; i++)
        if (gfx_chars[i])
            pgfx_rb->lcd_unlock_pattern(gfx_chars[i]);
}

/* place the display */
void pgfx_display(int cx, int cy)
{
    int i, j;
    int width = MIN(char_width, 11 - cx);
    int height = MIN(char_height, 2 - cy);

    for (i = 0; i < width; i++)
        for (j = 0; j < height; j++)
            pgfx_rb->lcd_putc(cx + i, cy + j, gfx_chars[char_height * i + j]);
}

/*** Update functions ***/

void pgfx_update(void)
{
    int i;

    for (i = 0; i < char_width * char_height; i++)
        pgfx_rb->lcd_define_pattern(gfx_chars[i], gfx_buffer + 7 * i);
}

/*** Parameter handling ***/

void pgfx_set_drawmode(int mode)
{
    drawmode = mode & (DRMODE_SOLID|DRMODE_INVERSEVID);
}

int pgfx_get_drawmode(void)
{
    return drawmode;
}

/*** Low-level drawing functions ***/

static void setpixel(int x, int y)
{
    gfx_buffer[pixel_height * (x/5) + y] |= 0x10 >> (x%5);
}

static void clearpixel(int x, int y)
{
    gfx_buffer[pixel_height * (x/5) + y] &= ~(0x10 >> (x%5));
}

static void flippixel(int x, int y)
{
    gfx_buffer[pixel_height * (x/5) + y] ^= 0x10 >> (x%5);
}

static void nopixel(int x, int y)
{
    (void)x;
    (void)y;
}

lcd_pixelfunc_type* pgfx_pixelfuncs[8] = {
    flippixel, nopixel, setpixel, setpixel,
    nopixel, clearpixel, nopixel, clearpixel
};
                               
static void flipblock(unsigned char *address, unsigned mask, unsigned bits)
{
    *address ^= (bits & mask);
}

static void bgblock(unsigned char *address, unsigned mask, unsigned bits)
{
    *address &= (bits | ~mask);
}

static void fgblock(unsigned char *address, unsigned mask, unsigned bits)
{
    *address |= (bits & mask);
}

static void solidblock(unsigned char *address, unsigned mask, unsigned bits)
{
    *address = (*address & ~mask) | (bits & mask);
}

static void flipinvblock(unsigned char *address, unsigned mask, unsigned bits)
{
    *address ^= (~bits & mask);
}

static void bginvblock(unsigned char *address, unsigned mask, unsigned bits)
{
    *address &= ~(bits & mask);
}

static void fginvblock(unsigned char *address, unsigned mask, unsigned bits)
{
    *address |= (~bits & mask);
}

static void solidinvblock(unsigned char *address, unsigned mask, unsigned bits)
{
    *address = (*address & ~mask) | (~bits & mask);
}

lcd_blockfunc_type* pgfx_blockfuncs[8] = {
    flipblock, bgblock, fgblock, solidblock,
    flipinvblock, bginvblock, fginvblock, solidinvblock
};

/*** Drawing functions ***/

/* Clear the whole display */
void pgfx_clear_display(void)
{
    unsigned bits = (drawmode & DRMODE_INVERSEVID) ? 0x1F : 0;

    pgfx_rb->memset(gfx_buffer, bits, char_width * pixel_height);
}

/* Set a single pixel */
void pgfx_drawpixel(int x, int y)
{
    if (((unsigned)x < (unsigned)pixel_width) 
        && ((unsigned)y < (unsigned)pixel_height))
        pgfx_pixelfuncs[drawmode](x, y);
}

/* Draw a line */
void pgfx_drawline(int x1, int y1, int x2, int y2)
{
    int numpixels;
    int i;
    int deltax, deltay;
    int d, dinc1, dinc2;
    int x, xinc1, xinc2;
    int y, yinc1, yinc2;
    lcd_pixelfunc_type *pfunc = pgfx_pixelfuncs[drawmode];

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
        if (((unsigned)x < (unsigned)pixel_width)
            && ((unsigned)y < (unsigned)pixel_height))
            pfunc(x, y);

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
void pgfx_hline(int x1, int x2, int y)
{
    int nx;
    unsigned char *dst;
    unsigned mask, mask_right;
    lcd_blockfunc_type *bfunc;

    /* direction flip */
    if (x2 < x1)
    {
        nx = x1;
        x1 = x2;
        x2 = nx;
    }

    /* nothing to draw? */
    if (((unsigned)y >= (unsigned)pixel_height) || (x1 >= pixel_width) 
        || (x2 < 0))
        return;  
    
    /* clipping */
    if (x1 < 0)
        x1 = 0;
    if (x2 >= pixel_width)
        x2 = pixel_width - 1;
        
    bfunc = pgfx_blockfuncs[drawmode];
    dst   = &gfx_buffer[pixel_height * (x1/5) + y];
    nx    = x2 - (x1 - (x1 % 5));
    mask  = 0x1F >> (x1 % 5);
    mask_right = 0x1F0 >> (nx % 5);

    for (; nx >= 5; nx -= 5)
    {
        bfunc(dst, mask, 0xFFu);
        dst += pixel_height;
        mask = 0x1F;
    }
    mask &= mask_right;
    bfunc(dst, mask, 0x1F);
}

/* Draw a vertical line (optimised) */
void pgfx_vline(int x, int y1, int y2)
{
    int y;
    unsigned char *dst;
    unsigned mask;
    lcd_blockfunc_type *bfunc;

    /* direction flip */
    if (y2 < y1)
    {
        y = y1;
        y1 = y2;
        y2 = y;
    }
    
    /* nothing to draw? */
    if (((unsigned)x >= (unsigned)pixel_width) || (y1 >= pixel_height) 
        || (y2 < 0))
        return;  
    
    /* clipping */
    if (y1 < 0)
        y1 = 0;
    if (y2 >= pixel_height)
        y2 = pixel_height - 1;
        
    bfunc = pgfx_blockfuncs[drawmode];
    dst   = &gfx_buffer[pixel_height * (x/5) + y1];
    mask  = 0x10 >> (x % 5);

    for (y = y1; y <= y2; y++)
        bfunc(dst++, mask, 0x1F);
}

/* Draw a rectangular box */
void pgfx_drawrect(int x, int y, int width, int height)
{
    if ((width <= 0) || (height <= 0))
        return;

    int x2 = x + width - 1;
    int y2 = y + height - 1;

    pgfx_vline(x, y, y2);
    pgfx_vline(x2, y, y2);
    pgfx_hline(x, x2, y);
    pgfx_hline(x, x2, y2);
}

/* Fill a rectangular area */
void pgfx_fillrect(int x, int y, int width, int height)
{
    int nx, i;
    unsigned char *dst;
    unsigned mask, mask_right;
    lcd_blockfunc_type *bfunc;

    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= pixel_width)
        || (y >= pixel_height) || (x + width <= 0) || (y + height <= 0))
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
    if (x + width > pixel_width)
        width = pixel_width - x;
    if (y + height > pixel_height)
        height = pixel_height - y;
    
    bfunc = pgfx_blockfuncs[drawmode];
    dst   = &gfx_buffer[pixel_height * (x/5) + y];
    nx    = width - 1 + (x % 5);
    mask  = 0x1F >> (x % 5);
    mask_right = 0x1F0 >> (nx % 5);

    for (; nx >= 5; nx -= 5)
    {
        unsigned char *dst_col = dst;

        for (i = height; i > 0; i--)
            bfunc(dst_col++, mask, 0x1F);

        dst += pixel_height;
        mask = 0x1F;
    }
    mask &= mask_right;

    for (i = height; i > 0; i--)
        bfunc(dst++, mask, 0x1F);
}

/* About PlayerGFX internal bitmap format:
 *
 * A bitmap contains one bit for every pixel that defines if that pixel is
 * black (1) or white (0). Bits within a byte are arranged horizontally, 
 * MSB at the left.
 * The bytes are stored in row-major order, with byte 0 being top left,
 * byte 1 2nd from left etc. Each row of bytes defines one pixel row.
 *
 * This approximates the (even more strange) internal hardware format. */

/* Draw a partial bitmap. Note that stride is given in bytes */
void pgfx_bitmap_part(const unsigned char *src, int src_x, int src_y,
                      int stride, int x, int y, int width, int height)
{
    int nx, shift;
    unsigned char *dst;
    unsigned mask, mask_right;
    lcd_blockfunc_type *bfunc;

    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= pixel_width) 
        || (y >= pixel_height) || (x + width <= 0) || (y + height <= 0))
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
    if (x + width > pixel_width)
        width = pixel_width - x;
    if (y + height > pixel_height)
        height = pixel_height - y;

    src   += stride * src_y + (src_x >> 3); /* move starting point */  
    dst   = &gfx_buffer[pixel_height * (x/5) + y];
    shift = 3 + (x % 5) - (src_x & 7);
    nx    = width - 1 + (x % 5);

    bfunc  = pgfx_blockfuncs[drawmode];
    mask  = 0x1F >> (x % 5);
    mask_right = 0x1F0 >> (nx % 5);
    
    for (y = 0; y < height; y++)
    {
        const unsigned char *src_row = src;
        unsigned char *dst_row = dst;
        unsigned mask_row = mask;
        unsigned data = *src_row++;
        int extrabits = shift;

        for (x = nx; x >= 5; x -= 5)
        {
            if (extrabits < 0)
            {
                data = (data << 8) | *src_row++;
                extrabits += 8;
            }
            bfunc(dst_row, mask_row, data >> extrabits);
            extrabits -= 5;
            dst_row += pixel_height;
            mask_row = 0x1F;
        }
        if (extrabits < 0)
        {
            data = (data << 8) | *src_row;
            extrabits += 8;
        }
        bfunc(dst_row, mask_row & mask_right, data >> extrabits);

        src += stride;
        dst++;
    }
}

/* Draw a full bitmap */
void pgfx_bitmap(const unsigned char *src, int x, int y, int width, int height)
{
    pgfx_bitmap_part(src, 0, 0, (width + 7) >> 3, x, y, width, height);
}

#endif /* HAVE_LCD_CHARCELLS */
