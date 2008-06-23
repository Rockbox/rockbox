/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * LCD driver for horizontally-packed 2bpp greyscale display
 * 
 * Based on code from the rockbox lcd's driver
 *
 * Copyright (c) 2006 Seven Le Mesle (sevlm@free.fr)
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "cpu.h"
#include "lcd.h"
#include "kernel.h"
#include "thread.h"
#include <string.h>
#include <stdlib.h>
#include "file.h"
#include "debug.h"
#include "system.h"
#include "font.h"
#include "rbunicode.h"
#include "bidi.h"
#include "scroll_engine.h"

/*** globals ***/

unsigned char lcd_framebuffer[LCD_FBHEIGHT][LCD_FBWIDTH] IBSS_ATTR;

static const unsigned char pixmask[4] ICONST_ATTR = {
    0xC0, 0x30, 0x0C, 0x03
};

static fb_data* lcd_backdrop = NULL;
static long lcd_backdrop_offset IDATA_ATTR = 0;

static struct viewport default_vp =
{
    .x        = 0,
    .y        = 0,
    .width    = LCD_WIDTH,
    .height   = LCD_HEIGHT,
    .font     = FONT_SYSFIXED,
    .drawmode = DRMODE_SOLID,
    .fg_pattern = LCD_DEFAULT_FG,
    .bg_pattern = LCD_DEFAULT_BG
};

static struct viewport* current_vp IBSS_ATTR;
static unsigned fg_pattern IBSS_ATTR;
static unsigned bg_pattern IBSS_ATTR;

/* LCD init */
void lcd_init(void)
{
    /* Initialise the viewport */
    lcd_set_viewport(NULL);

    lcd_clear_display();
    /* Call device specific init */
    lcd_init_device();
    scroll_init();
}

/*** Viewports ***/

void lcd_set_viewport(struct viewport* vp)
{
    if (vp == NULL)
        current_vp = &default_vp;
    else
        current_vp = vp;

    fg_pattern = 0x55 * (~current_vp->fg_pattern & 3);
    bg_pattern = 0x55 * (~current_vp->bg_pattern & 3);
}

void lcd_update_viewport(void)
{
    lcd_update_rect(current_vp->x, current_vp->y,
                    current_vp->width, current_vp->height);
}

void lcd_update_viewport_rect(int x, int y, int width, int height)
{
    lcd_update_rect(current_vp->x + x, current_vp->y + y, width, height);
}

/*** parameter handling ***/

void lcd_set_drawmode(int mode)
{
    current_vp->drawmode = mode & (DRMODE_SOLID|DRMODE_INVERSEVID);
}

int lcd_get_drawmode(void)
{
    return current_vp->drawmode;
}

void lcd_set_foreground(unsigned brightness)
{
    current_vp->fg_pattern = brightness;
    fg_pattern = 0x55 * (~brightness & 3);
}

unsigned lcd_get_foreground(void)
{
    return current_vp->fg_pattern;
}

void lcd_set_background(unsigned brightness)
{
    current_vp->bg_pattern = brightness;
    bg_pattern = 0x55 * (~brightness & 3);
}

unsigned lcd_get_background(void)
{
    return current_vp->bg_pattern;
}

void lcd_set_drawinfo(int mode, unsigned fg_brightness, unsigned bg_brightness)
{
    lcd_set_drawmode(mode);
    lcd_set_foreground(fg_brightness);
    lcd_set_background(bg_brightness);
}

int lcd_getwidth(void)
{
    return current_vp->width;
}

int lcd_getheight(void)
{
    return current_vp->height;
}

void lcd_setfont(int newfont)
{
    current_vp->font = newfont;
}

int lcd_getfont(void)
{
    return current_vp->font;
}

int lcd_getstringsize(const unsigned char *str, int *w, int *h)
{
    return font_getstringsize(str, w, h, current_vp->font);
}

/*** low-level drawing functions ***/

static void setpixel(int x, int y)
{
    unsigned mask = pixmask[x & 3];
    fb_data *address = &lcd_framebuffer[y][x>>2];
    unsigned data = *address;

    *address = data ^ ((data ^ fg_pattern) & mask);
}

static void clearpixel(int x, int y)
{
    unsigned mask = pixmask[x & 3];
    fb_data *address = &lcd_framebuffer[y][x>>2];
    unsigned data = *address;

    *address = data ^ ((data ^ bg_pattern) & mask);
}

static void clearimgpixel(int x, int y)
{
    unsigned mask = pixmask[x & 3];
    fb_data *address = &lcd_framebuffer[y][x>>2];
    unsigned data = *address;

    *address = data ^ ((data ^ *(address + lcd_backdrop_offset)) & mask);
}

static void flippixel(int x, int y)
{
    unsigned mask =  pixmask[x & 3];
    fb_data *address = &lcd_framebuffer[y][x>>2];
    
    *address ^= mask;
}

static void nopixel(int x, int y)
{
    (void)x;
    (void)y;
}

lcd_pixelfunc_type* const lcd_pixelfuncs_bgcolor[8] = {
    flippixel, nopixel, setpixel, setpixel,
    nopixel, clearpixel, nopixel, clearpixel
};

lcd_pixelfunc_type* const lcd_pixelfuncs_backdrop[8] = {
    flippixel, nopixel, setpixel, setpixel,
    nopixel, clearimgpixel, nopixel, clearimgpixel
};

lcd_pixelfunc_type* const * lcd_pixelfuncs = lcd_pixelfuncs_bgcolor;


/* 'mask' and 'bits' contain 2 bits per pixel */
static void ICODE_ATTR flipblock(fb_data *address, unsigned mask,
                                 unsigned bits)
{
    *address ^= bits & mask;
}

static void ICODE_ATTR bgblock(fb_data *address, unsigned mask,
                               unsigned bits)
{
    unsigned data = *address;

    *address = data ^ ((data ^ bg_pattern) & mask & ~bits);
}

static void ICODE_ATTR bgimgblock(fb_data *address, unsigned mask,
                                  unsigned bits)
{
    unsigned data = *address;

    *address = data ^ ((data ^ *(address + lcd_backdrop_offset)) & mask & ~bits);
}

static void ICODE_ATTR fgblock(fb_data *address, unsigned mask,
                               unsigned bits)
{
    unsigned data = *address;

    *address = data ^ ((data ^ fg_pattern) & mask & bits);
}

static void ICODE_ATTR solidblock(fb_data *address, unsigned mask,
                                  unsigned bits)
{
    unsigned data = *address;
    unsigned bgp  = bg_pattern;

    bits     = bgp  ^ ((bgp ^ fg_pattern) & bits);
    *address = data ^ ((data ^ bits) & mask);
}

static void ICODE_ATTR solidimgblock(fb_data *address, unsigned mask,
                                     unsigned bits)
{
    unsigned data = *address;
    unsigned bgp  = *(address + lcd_backdrop_offset);

    bits     = bgp  ^ ((bgp ^ fg_pattern) & bits);
    *address = data ^ ((data ^ bits) & mask);
}

static void ICODE_ATTR flipinvblock(fb_data *address, unsigned mask,
                                    unsigned bits)
{
    *address ^= ~bits & mask;
}

static void ICODE_ATTR bginvblock(fb_data *address, unsigned mask,
                                  unsigned bits)
{
    unsigned data = *address;

    *address = data ^ ((data ^ bg_pattern) & mask & bits);
}

static void ICODE_ATTR bgimginvblock(fb_data *address, unsigned mask,
                                     unsigned bits)
{
    unsigned data = *address;

    *address = data ^ ((data ^ *(address + lcd_backdrop_offset)) & mask & bits);
}

static void ICODE_ATTR fginvblock(fb_data *address, unsigned mask,
                                  unsigned bits)
{
    unsigned data = *address;

    *address = data ^ ((data ^ fg_pattern) & mask & ~bits);
}

static void ICODE_ATTR solidinvblock(fb_data *address, unsigned mask,
                                     unsigned bits)
{
    unsigned data = *address;
    unsigned fgp  = fg_pattern;

    bits     = fgp  ^ ((fgp ^ bg_pattern) & bits);
    *address = data ^ ((data ^ bits) & mask);
}

static void ICODE_ATTR solidimginvblock(fb_data *address, unsigned mask,
                                        unsigned bits)
{
    unsigned data = *address;
    unsigned fgp  = fg_pattern;

    bits     = fgp  ^ ((fgp ^ *(address + lcd_backdrop_offset)) & bits);
    *address = data ^ ((data ^ bits) & mask);
}

lcd_blockfunc_type* const lcd_blockfuncs_bgcolor[8] = {
    flipblock, bgblock, fgblock, solidblock,
    flipinvblock, bginvblock, fginvblock, solidinvblock
};

lcd_blockfunc_type* const lcd_blockfuncs_backdrop[8] = {
    flipblock, bgimgblock, fgblock, solidimgblock,
    flipinvblock, bgimginvblock, fginvblock, solidimginvblock
};

lcd_blockfunc_type* const * lcd_blockfuncs = lcd_blockfuncs_bgcolor;


void lcd_set_backdrop(fb_data* backdrop)
{
    lcd_backdrop = backdrop;
    if (backdrop)
    {
        lcd_backdrop_offset = (long)backdrop - (long)lcd_framebuffer;
        lcd_pixelfuncs = lcd_pixelfuncs_backdrop;
        lcd_blockfuncs = lcd_blockfuncs_backdrop;
    }
    else
    {
        lcd_backdrop_offset = 0;
        lcd_pixelfuncs = lcd_pixelfuncs_bgcolor;
        lcd_blockfuncs = lcd_blockfuncs_bgcolor;
    }
}

fb_data* lcd_get_backdrop(void)
{
    return lcd_backdrop;
}


static inline void setblock(fb_data *address, unsigned mask, unsigned bits)
{
    unsigned data = *address;
    
    bits    ^= data;
    *address = data ^ (bits & mask);
}

/*** drawing functions ***/

/* Clear the whole display */
void lcd_clear_display(void)
{
    if (current_vp->drawmode & DRMODE_INVERSEVID)
    {
        memset(lcd_framebuffer, fg_pattern, sizeof lcd_framebuffer);
    }
    else
    {
        if (lcd_backdrop)
            memcpy(lcd_framebuffer, lcd_backdrop, sizeof lcd_framebuffer);
        else
            memset(lcd_framebuffer, bg_pattern, sizeof lcd_framebuffer);
    }

    lcd_scroll_info.lines = 0;
}

/* Clear the current viewport */
void lcd_clear_viewport(void)
{
    int lastmode;

    if (current_vp == &default_vp)
    {
        lcd_clear_display();
    }
    else
    {
        lastmode = current_vp->drawmode;

        /* Invert the INVERSEVID bit and set basic mode to SOLID */
        current_vp->drawmode = (~lastmode & DRMODE_INVERSEVID) | 
                               DRMODE_SOLID;

        lcd_fillrect(0, 0, current_vp->width, current_vp->height);

        current_vp->drawmode = lastmode;

        lcd_scroll_stop(current_vp);
    }
}

/* Set a single pixel */
void lcd_drawpixel(int x, int y)
{
    if (((unsigned)x < (unsigned)current_vp->width) &&
        ((unsigned)y < (unsigned)current_vp->height))
        lcd_pixelfuncs[current_vp->drawmode](current_vp->x + x, current_vp->y + y);
}

/* Draw a line */
void lcd_drawline(int x1, int y1, int x2, int y2)
{
    int numpixels;
    int i;
    int deltax, deltay;
    int d, dinc1, dinc2;
    int x, xinc1, xinc2;
    int y, yinc1, yinc2;
    lcd_pixelfunc_type *pfunc = lcd_pixelfuncs[current_vp->drawmode];

    deltay = abs(y2 - y1);
    if (deltay == 0)
    {
        DEBUGF("lcd_drawline() called for horizontal line - optimisation.\n");
        lcd_hline(x1, x2, y1);
        return;
    }
    deltax = abs(x2 - x1);
    if (deltax == 0)
    {
        DEBUGF("lcd_drawline() called for vertical line - optimisation.\n");
        lcd_vline(x1, y1, y2);
        return;
    }
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
        if (((unsigned)x < (unsigned)current_vp->width) &&
            ((unsigned)y < (unsigned)current_vp->height))
            pfunc(current_vp->x + x, current_vp->y + y);

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
void lcd_hline(int x1, int x2, int y)
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
    if (((unsigned)y >= (unsigned)current_vp->height) || (x1 >= current_vp->width) 
        || (x2 < 0))
        return;  
    
    /* clipping */
    if (x1 < 0)
        x1 = 0;
    if (x2 >= current_vp->width)
        x2 = current_vp->width-1;

    /* adjust to viewport */
    x1 += current_vp->x;
    x2 += current_vp->x;
    y += current_vp->y;

    bfunc = lcd_blockfuncs[current_vp->drawmode];
    dst   = &lcd_framebuffer[y][x1>>2];
    nx    = x2 - (x1 & ~3);
    mask  = 0xFFu >> (2 * (x1 & 3));
    mask_right = 0xFFu << (2 * (~nx & 3));
    
    for (; nx >= 4; nx -= 4)
    {
        bfunc(dst++, mask, 0xFFu);
        mask = 0xFFu;
    }
    mask &= mask_right;
    bfunc(dst, mask, 0xFFu);
}

/* Draw a vertical line (optimised) */
void lcd_vline(int x, int y1, int y2)
{
    int y;
    unsigned char *dst, *dst_end;
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
    if (((unsigned)x >= (unsigned)current_vp->width) || (y1 >= current_vp->height)
        || (y2 < 0))
        return;  
    
    /* clipping */
    if (y1 < 0)
        y1 = 0;
    if (y2 >= current_vp->height)
        y2 = current_vp->height-1;

    /* adjust for viewport */
    y1 += current_vp->y;
    y2 += current_vp->y;
    x += current_vp->x;
        
    bfunc = lcd_blockfuncs[current_vp->drawmode];
    dst   = &lcd_framebuffer[y1][x>>2];
    mask  = pixmask[x & 3];
    
    dst_end = dst + (y2 - y1) * LCD_FBWIDTH;
    do
    {
        bfunc(dst, mask, 0xFFu);
        dst += LCD_FBWIDTH;
    }
    while (dst <= dst_end);
}

/* Draw a rectangular box */
void lcd_drawrect(int x, int y, int width, int height)
{
    if ((width <= 0) || (height <= 0))
        return;

    int x2 = x + width - 1;
    int y2 = y + height - 1;

    lcd_vline(x, y, y2);
    lcd_vline(x2, y, y2);
    lcd_hline(x, x2, y);
    lcd_hline(x, x2, y2);
}

/* Fill a rectangular area */
void lcd_fillrect(int x, int y, int width, int height)
{
    int nx;
    unsigned char *dst, *dst_end;
    unsigned mask, mask_right;
    lcd_blockfunc_type *bfunc;

    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= current_vp->width) || (y >= current_vp->height)
        || (x + width <= 0) || (y + height <= 0))
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
    if (x + width > current_vp->width)
        width = current_vp->width - x;
    if (y + height > current_vp->height)
        height = current_vp->height - y;

    /* adjust for viewport */
    x += current_vp->x;
    y += current_vp->y;

    bfunc = lcd_blockfuncs[current_vp->drawmode];
    dst   = &lcd_framebuffer[y][x>>2];
    nx    = width - 1 + (x & 3);
    mask  = 0xFFu >> (2 * (x & 3));
    mask_right = 0xFFu << (2 * (~nx & 3));
    
    for (; nx >= 4; nx -= 4)
    {
        unsigned char *dst_col = dst;

        dst_end = dst_col + height * LCD_FBWIDTH;
        do
        {
            bfunc(dst_col, mask, 0xFFu);
            dst_col += LCD_FBWIDTH;
        }
        while (dst_col < dst_end);

        dst++;
        mask = 0xFFu;
    }
    mask &= mask_right;

    dst_end = dst + height * LCD_FBWIDTH;
    do
    {
        bfunc(dst, mask, 0xFFu);
        dst += LCD_FBWIDTH;
    }
    while (dst < dst_end);
}

/* About Rockbox' internal monochrome bitmap format:
 *
 * A bitmap contains one bit for every pixel that defines if that pixel is
 * black (1) or white (0). Bits within a byte are arranged vertically, LSB
 * at top.
 * The bytes are stored in row-major order, with byte 0 being top left,
 * byte 1 2nd from left etc. The first row of bytes defines pixel rows
 * 0..7, the second row defines pixel row 8..15 etc. */

/* Draw a partial monochrome bitmap */
void ICODE_ATTR lcd_mono_bitmap_part(const unsigned char *src, int src_x,
                                     int src_y, int stride, int x, int y,
                                     int width, int height)
{
    int ny, nx, ymax;
    const unsigned char * src_end;
    lcd_pixelfunc_type* fgfunc;
    lcd_pixelfunc_type* bgfunc;

    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= current_vp->width) ||
        (y >= current_vp->height) || (x + width <= 0) || (y + height <= 0))
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
    if (x + width > current_vp->width)
        width = current_vp->width - x;
    if (y + height > current_vp->height)
        height = current_vp->height - y;

    /* adjust for viewport */
    x += current_vp->x;
    y += current_vp->y;

    src += stride * (src_y >> 3) + src_x; /* move starting point */
    src_y  &= 7;
    src_end = src + width;

    fgfunc = lcd_pixelfuncs[current_vp->drawmode];    
    bgfunc = lcd_pixelfuncs[current_vp->drawmode ^ DRMODE_INVERSEVID];    
    nx = x;
    do
    {
        const unsigned char *src_col = src++;
        unsigned data = *src_col >> src_y;
        int numbits = 8 - ((int)src_y);
        
        ymax = y + height;
        ny = y;
        do
        {
            if (data & 0x01)
                fgfunc(nx,ny);
            else
                bgfunc(nx,ny);

            ny++;

            data >>= 1;
            if (--numbits == 0)
            {
                src_col += stride;
                data = *src_col;
                numbits = 8;
            }
        }
        while (ny < ymax);
        nx++;
    }
    while (src < src_end);
}

/* Draw a full monochrome bitmap */
void lcd_mono_bitmap(const unsigned char *src, int x, int y, int width, int height)
{
    lcd_mono_bitmap_part(src, 0, 0, width, x, y, width, height);
}

/* About Rockbox' internal native bitmap format:
 *
 * A bitmap contains two bits for every pixel. 00 = white, 01 = light grey,
 * 10 = dark grey, 11 = black. Bits within a byte are arranged horizontally,
 * MSB at the left.
 * The bytes are stored in row-major order, with byte 0 being top left,
 * byte 1 2nd from left etc. Each row of bytes defines one pixel row.
 *
 * This is the same as the internal lcd hw format. */

/* Draw a partial native bitmap */
void ICODE_ATTR lcd_bitmap_part(const unsigned char *src, int src_x,
                                int src_y, int stride, int x, int y,
                                int width, int height)
{
    int  shift, nx;
    unsigned char *dst, *dst_end;
    unsigned mask, mask_right;

    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= current_vp->width) || 
        (y >= current_vp->height) || (x + width <= 0) || (y + height <= 0))
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
    if (x + width > current_vp->width)
        width = current_vp->width - x;
    if (y + height > current_vp->height)
        height = current_vp->height - y;

    /* adjust for viewport */
    x += current_vp->x;
    y += current_vp->y;

    stride = (stride + 3) >> 2; /* convert to no. of bytes */

    src   += stride * src_y + (src_x >> 2); /* move starting point */
    src_x &= 3;
    x     -= src_x;
    dst   = &lcd_framebuffer[y][x>>2];
    shift = x & 3;
    nx    = width - 1 + shift + src_x;

    mask   = 0xFF00u >> (2 * (shift + src_x));
    mask_right = 0xFFu << (2 * (~nx & 3));
    
    shift *= 2;
    dst_end = dst + height * LCD_FBWIDTH;
    do
    {
        const unsigned char *src_row = src;
        unsigned char *dst_row = dst;
        unsigned mask_row = mask >> 8;
        unsigned data = 0;
        
        for (x = nx; x >= 4; x -= 4)
        {
            data = (data << 8) | *src_row++;
            
            if (mask_row & 0xFF)
            {
                setblock(dst_row, mask_row, data >> shift);
                mask_row = 0xFF;
            }
            else
                mask_row = mask;

            dst_row++;
        }
        data = (data << 8) | *src_row;
        setblock(dst_row, mask_row & mask_right, data >> shift);

        src += stride;
        dst += LCD_FBWIDTH;
    }
    while (dst < dst_end);
}

/* Draw a full native bitmap */
void lcd_bitmap(const unsigned char *src, int x, int y, int width, int height)
{
    lcd_bitmap_part(src, 0, 0, width, x, y, width, height);
}

/* put a string at a given pixel position, skipping first ofs pixel columns */
static void lcd_putsxyofs(int x, int y, int ofs, const unsigned char *str)
{
    unsigned short ch;
    unsigned short *ucs;
    struct font* pf = font_get(current_vp->font);

    ucs = bidi_l2v(str, 1);

    while ((ch = *ucs++) != 0 && x < current_vp->width)
    {
        int width;
        const unsigned char *bits;

        /* get proportional width and glyph bits */
        width = font_get_width(pf,ch);

        if (ofs > width)
        {
            ofs -= width;
            continue;
        }

        bits = font_get_bits(pf, ch);

        lcd_mono_bitmap_part(bits, ofs, 0, width, x, y, width - ofs,
                             pf->height);
        
        x += width - ofs;
        ofs = 0;
    }
}

/* put a string at a given pixel position */
void lcd_putsxy(int x, int y, const unsigned char *str)
{
    lcd_putsxyofs(x, y, 0, str);
}

/*** line oriented text output ***/

/* put a string at a given char position */
void lcd_puts(int x, int y, const unsigned char *str)
{
    lcd_puts_style_offset(x, y, str, STYLE_DEFAULT, 0);
}

void lcd_puts_style(int x, int y, const unsigned char *str, int style)
{
    lcd_puts_style_offset(x, y, str, style, 0);
}

void lcd_puts_offset(int x, int y, const unsigned char *str, int offset)
{
    lcd_puts_style_offset(x, y, str, STYLE_DEFAULT, offset);
}

/* put a string at a given char position, style, and pixel position,
 * skipping first offset pixel columns */
void lcd_puts_style_offset(int x, int y, const unsigned char *str,
                           int style, int offset)
{
    int xpos,ypos,w,h,xrect;
    int lastmode = current_vp->drawmode;

    /* make sure scrolling is turned off on the line we are updating */
    lcd_scroll_stop_line(current_vp, y);

    if(!str || !str[0])
        return;

    lcd_getstringsize(str, &w, &h);
    xpos = x*w / utf8length((char *)str);
    ypos = y*h;
    current_vp->drawmode = (style & STYLE_INVERT) ?
                           (DRMODE_SOLID|DRMODE_INVERSEVID) : DRMODE_SOLID;
    lcd_putsxyofs(xpos, ypos, offset, str);
    current_vp->drawmode ^= DRMODE_INVERSEVID;
    xrect = xpos + MAX(w - offset, 0);
    lcd_fillrect(xrect, ypos, current_vp->width - xrect, h);
    current_vp->drawmode = lastmode;
}

/*** scrolling ***/
void lcd_puts_scroll(int x, int y, const unsigned char *string)
{
    lcd_puts_scroll_style(x, y, string, STYLE_DEFAULT);
}

void lcd_puts_scroll_style(int x, int y, const unsigned char *string, int style)
{
     lcd_puts_scroll_style_offset(x, y, string, style, 0);
}

void lcd_puts_scroll_offset(int x, int y, const unsigned char *string, int offset)
{
     lcd_puts_scroll_style_offset(x, y, string, STYLE_DEFAULT, offset);
}          
   
void lcd_puts_scroll_style_offset(int x, int y, const unsigned char *string,
                                         int style, int offset)
{
    struct scrollinfo* s;
    int w, h;

    if ((unsigned)y >= (unsigned)current_vp->height)
        return;

    /* remove any previously scrolling line at the same location */
    lcd_scroll_stop_line(current_vp, y);

    if (lcd_scroll_info.lines >= LCD_SCROLLABLE_LINES) return;

    s = &lcd_scroll_info.scroll[lcd_scroll_info.lines];

    s->start_tick = current_tick + lcd_scroll_info.delay;
    s->style = style;
    if (style & STYLE_INVERT) {
        lcd_puts_style_offset(x,y,string,STYLE_INVERT,offset);
    }
    else
        lcd_puts_offset(x,y,string,offset);

    lcd_getstringsize(string, &w, &h);

    if (current_vp->width - x * 8 < w) {
        /* prepare scroll line */
        char *end;

        memset(s->line, 0, sizeof s->line);
        strcpy(s->line, (char *)string);

        /* get width */
        s->width = lcd_getstringsize((unsigned char *)s->line, &w, &h);

        /* scroll bidirectional or forward only depending on the string
           width */
        if ( lcd_scroll_info.bidir_limit ) {
            s->bidir = s->width < (current_vp->width) *
                (100 + lcd_scroll_info.bidir_limit) / 100;
        }
        else
            s->bidir = false;

        if (!s->bidir) { /* add spaces if scrolling in the round */
            strcat(s->line, "   ");
            /* get new width incl. spaces */
            s->width = lcd_getstringsize((unsigned char *)s->line, &w, &h);
        }

        end = strchr(s->line, '\0');
        strncpy(end, (char *)string, current_vp->width/2);

        s->vp = current_vp;
        s->y = y;
        s->len = utf8length((char *)string);
        s->offset = offset;
        s->startx = x * s->width / s->len;;
        s->backward = false;
        lcd_scroll_info.lines++;
    }
}

void lcd_scroll_fn(void)
{
    struct font* pf;
    struct scrollinfo* s;
    int index;
    int xpos, ypos;
    int lastmode;
    struct viewport* old_vp = current_vp;

    for ( index = 0; index < lcd_scroll_info.lines; index++ ) {
        s = &lcd_scroll_info.scroll[index];

        /* check pause */
        if (TIME_BEFORE(current_tick, s->start_tick))
            continue;

        lcd_set_viewport(s->vp);

        if (s->backward)
            s->offset -= lcd_scroll_info.step;
        else
            s->offset += lcd_scroll_info.step;

        pf = font_get(current_vp->font);
        xpos = s->startx;
        ypos = s->y * pf->height;

        if (s->bidir) { /* scroll bidirectional */
            if (s->offset <= 0) {
                /* at beginning of line */
                s->offset = 0;
                s->backward = false;
                s->start_tick = current_tick + lcd_scroll_info.delay * 2;
            }
            if (s->offset >= s->width - (current_vp->width - xpos)) {
                /* at end of line */
                s->offset = s->width - (current_vp->width - xpos);
                s->backward = true;
                s->start_tick = current_tick + lcd_scroll_info.delay * 2;
            }
        }
        else {
            /* scroll forward the whole time */
            if (s->offset >= s->width)
                s->offset %= s->width;
        }

        lastmode = current_vp->drawmode;
        current_vp->drawmode = (s->style&STYLE_INVERT) ?
                               (DRMODE_SOLID|DRMODE_INVERSEVID) : DRMODE_SOLID;
        lcd_putsxyofs(xpos, ypos, s->offset, s->line);
        current_vp->drawmode = lastmode;
        lcd_update_viewport_rect(xpos, ypos, current_vp->width - xpos, pf->height);
    }

    lcd_set_viewport(old_vp);
}
