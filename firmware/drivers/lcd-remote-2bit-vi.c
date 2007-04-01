/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Jens Arnold
 *
 * Rockbox driver for 2bit vertically interleaved remote LCDs
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
#include "lcd-remote.h"
#include "kernel.h"
#include "thread.h"
#include <string.h>
#include <stdlib.h>
#include "memory.h"
#include "file.h"
#include "debug.h"
#include "system.h"
#include "font.h"
#include "rbunicode.h"
#include "bidi.h"
#include "lcd-remote-target.h"

#define SCROLLABLE_LINES (((LCD_REMOTE_HEIGHT+4)/5 < 32) ? (LCD_REMOTE_HEIGHT+4)/5 : 32)

/*** globals ***/

fb_remote_data lcd_remote_framebuffer[LCD_REMOTE_FBHEIGHT][LCD_REMOTE_FBWIDTH]
                                      IBSS_ATTR;

static const fb_remote_data patterns[4] = {0xFFFF, 0xFF00, 0x00FF, 0x0000};

static fb_remote_data* remote_backdrop = NULL;
static long remote_backdrop_offset IDATA_ATTR = 0;

static unsigned fg_pattern IDATA_ATTR = 0xFFFF; /* initially black */
static unsigned bg_pattern IDATA_ATTR = 0x0000; /* initially white */
static int drawmode = DRMODE_SOLID;
static int xmargin = 0;
static int ymargin = 0;
static int curfont = FONT_SYSFIXED;

/* scrolling */
static volatile int scrolling_lines=0; /* Bitpattern of which lines are scrolling */
static void scroll_thread(void);
static long scroll_stack[DEFAULT_STACK_SIZE/sizeof(long)];
static const char scroll_name[] = "remote_scroll";
static int scroll_ticks = 12; /* # of ticks between updates*/
static int scroll_delay = HZ/2; /* ticks delay before start */
static int scroll_step = 6;  /* pixels per scroll step */
static int bidir_limit = 50;  /* percent */
static struct scrollinfo scroll[SCROLLABLE_LINES];

static const char scroll_tick_table[16] = {
 /* Hz values:
    1, 1.25, 1.55, 2, 2.5, 3.12, 4, 5, 6.25, 8.33, 10, 12.5, 16.7, 20, 25, 33 */
    100, 80, 64, 50, 40, 32, 25, 20, 16, 12, 10, 8, 6, 5, 4, 3
};

/* remote hotplug */
#ifndef SIMULATOR
struct event_queue remote_scroll_queue;
#endif

/*** parameter handling ***/
unsigned lcd_remote_color_to_native(unsigned color)
{
    unsigned r = (color & 0xf800) >> 10;
    unsigned g = (color & 0x07e0) >> 5;
    unsigned b = (color & 0x001f) << 2;
    /*
     *                                     |R|
     * |Y'| = |0.299000 0.587000 0.114000| |G|
     *                                     |B|
     */
    return (5*r + 9*g + b) >> 8;
}

void lcd_remote_set_drawmode(int mode)
{
    drawmode = mode & (DRMODE_SOLID|DRMODE_INVERSEVID);
}

int lcd_remote_get_drawmode(void)
{
    return drawmode;
}

void lcd_remote_set_foreground(unsigned brightness)
{
    fg_pattern = patterns[brightness & 3];
}

unsigned lcd_remote_get_foreground(void)
{
    return (~fg_pattern >> 7) & 3;
}

void lcd_remote_set_background(unsigned brightness)
{
    bg_pattern = patterns[brightness & 3];
}

unsigned lcd_remote_get_background(void)
{
    return (~bg_pattern >> 7) & 3;
}

void lcd_remote_set_drawinfo(int mode, unsigned fg_brightness,
                             unsigned bg_brightness)
{
    lcd_remote_set_drawmode(mode);
    lcd_remote_set_foreground(fg_brightness);
    lcd_remote_set_background(bg_brightness);
}

void lcd_remote_setmargins(int x, int y)
{
    xmargin = x;
    ymargin = y;
}

int lcd_remote_getxmargin(void)
{
    return xmargin;
}

int lcd_remote_getymargin(void)
{
    return ymargin;
}

void lcd_remote_setfont(int newfont)
{
    curfont = newfont;
}

int lcd_remote_getstringsize(const unsigned char *str, int *w, int *h)
{
    return font_getstringsize(str, w, h, curfont);
}

/*** low-level drawing functions ***/

static void setpixel(int x, int y)
{
    unsigned mask = 0x0101 << (y & 7);
    fb_remote_data *address = &lcd_remote_framebuffer[y>>3][x];
    unsigned data = *address;

    *address = data ^ ((data ^ fg_pattern) & mask);
}

static void clearpixel(int x, int y)
{
    unsigned mask = 0x0101 << (y & 7);
    fb_remote_data *address = &lcd_remote_framebuffer[y>>3][x];
    unsigned data = *address;

    *address = data ^ ((data ^ bg_pattern) & mask);
}

static void clearimgpixel(int x, int y)
{
    unsigned mask = 0x0101 << (y & 7);
    fb_remote_data *address = &lcd_remote_framebuffer[y>>3][x];
    unsigned data = *address;

    *address = data ^ ((data ^ *(fb_remote_data *)((long)address 
               + remote_backdrop_offset)) & mask);
}

static void flippixel(int x, int y)
{
    unsigned mask = 0x0101 << (y & 7);
    fb_remote_data *address = &lcd_remote_framebuffer[y>>3][x];

    *address ^= mask;
}

static void nopixel(int x, int y)
{
    (void)x;
    (void)y;
}

lcd_remote_pixelfunc_type* const lcd_remote_pixelfuncs_bgcolor[8] = {
    flippixel, nopixel, setpixel, setpixel,
    nopixel, clearpixel, nopixel, clearpixel
};

lcd_remote_pixelfunc_type* const lcd_remote_pixelfuncs_backdrop[8] = {
    flippixel, nopixel, setpixel, setpixel,
    nopixel, clearimgpixel, nopixel, clearimgpixel
};

lcd_remote_pixelfunc_type* const *lcd_remote_pixelfuncs = lcd_remote_pixelfuncs_bgcolor;

/* 'mask' and 'bits' contain 2 bits per pixel */
static void flipblock(fb_remote_data *address, unsigned mask, unsigned bits)
                      ICODE_ATTR;
static void flipblock(fb_remote_data *address, unsigned mask, unsigned bits)
{
    *address ^= bits & mask;
}

static void bgblock(fb_remote_data *address, unsigned mask, unsigned bits)
                    ICODE_ATTR;
static void bgblock(fb_remote_data *address, unsigned mask, unsigned bits)
{
    unsigned data = *address;

    *address = data ^ ((data ^ bg_pattern) & mask & ~bits);
}

static void bgimgblock(fb_remote_data *address, unsigned mask, unsigned bits)
                       ICODE_ATTR;
static void bgimgblock(fb_remote_data *address, unsigned mask, unsigned bits)
{
    unsigned data = *address;

    *address = data ^ ((data ^ *(fb_remote_data *)((long)address
               + remote_backdrop_offset)) & mask & ~bits);
}

static void fgblock(fb_remote_data *address, unsigned mask, unsigned bits)
                    ICODE_ATTR;
static void fgblock(fb_remote_data *address, unsigned mask, unsigned bits)
{
    unsigned data = *address;

    *address = data ^ ((data ^ fg_pattern) & mask & bits);
}

static void solidblock(fb_remote_data *address, unsigned mask, unsigned bits)
                       ICODE_ATTR;
static void solidblock(fb_remote_data *address, unsigned mask, unsigned bits)
{
    unsigned data = *address;
    unsigned bgp  = bg_pattern;

    bits     = bgp  ^ ((bgp ^ fg_pattern) & bits);
    *address = data ^ ((data ^ bits) & mask);
}

static void solidimgblock(fb_remote_data *address, unsigned mask, unsigned bits)
                          ICODE_ATTR;
static void solidimgblock(fb_remote_data *address, unsigned mask, unsigned bits)
{
    unsigned data = *address;
    unsigned bgp  = *(fb_remote_data *)((long)address + remote_backdrop_offset);

    bits     = bgp  ^ ((bgp ^ fg_pattern) & bits);
    *address = data ^ ((data ^ bits) & mask);
}

static void flipinvblock(fb_remote_data *address, unsigned mask, unsigned bits)
                         ICODE_ATTR;
static void flipinvblock(fb_remote_data *address, unsigned mask, unsigned bits)
{
    *address ^= ~bits & mask;
}

static void bginvblock(fb_remote_data *address, unsigned mask, unsigned bits)
                       ICODE_ATTR;
static void bginvblock(fb_remote_data *address, unsigned mask, unsigned bits)
{
    unsigned data = *address;

    *address = data ^ ((data ^ bg_pattern) & mask & bits);
}

static void bgimginvblock(fb_remote_data *address, unsigned mask, unsigned bits)
                          ICODE_ATTR;
static void bgimginvblock(fb_remote_data *address, unsigned mask, unsigned bits)
{
    unsigned data = *address;

    *address = data ^ ((data ^ *(fb_remote_data *)((long)address 
               + remote_backdrop_offset)) & mask & bits);
}

static void fginvblock(fb_remote_data *address, unsigned mask, unsigned bits)
                       ICODE_ATTR;
static void fginvblock(fb_remote_data *address, unsigned mask, unsigned bits)
{
    unsigned data = *address;

    *address = data ^ ((data ^ fg_pattern) & mask & ~bits);
}

static void solidinvblock(fb_remote_data *address, unsigned mask, unsigned bits)
                          ICODE_ATTR;
static void solidinvblock(fb_remote_data *address, unsigned mask, unsigned bits)
{
    unsigned data = *address;
    unsigned fgp  = fg_pattern;

    bits     = fgp  ^ ((fgp ^ bg_pattern) & bits);
    *address = data ^ ((data ^ bits) & mask);
}

static void solidimginvblock(fb_remote_data *address, unsigned mask, unsigned bits)
                             ICODE_ATTR;
static void solidimginvblock(fb_remote_data *address, unsigned mask, unsigned bits)
{
    unsigned data = *address;
    unsigned fgp  = fg_pattern;

    bits     = fgp  ^ ((fgp ^ *(fb_remote_data *)((long)address
               + remote_backdrop_offset)) & bits);
    *address = data ^ ((data ^ bits) & mask);
}

lcd_remote_blockfunc_type* const lcd_remote_blockfuncs_bgcolor[8] = {
    flipblock, bgblock, fgblock, solidblock,
    flipinvblock, bginvblock, fginvblock, solidinvblock
};

lcd_remote_blockfunc_type* const lcd_remote_blockfuncs_backdrop[8] = {
    flipblock, bgimgblock, fgblock, solidimgblock,
    flipinvblock, bgimginvblock, fginvblock, solidimginvblock
};

lcd_remote_blockfunc_type* const *lcd_remote_blockfuncs = lcd_remote_blockfuncs_bgcolor;


void lcd_remote_set_backdrop(fb_remote_data* backdrop)
{
    remote_backdrop = backdrop;
    if (backdrop)
    {
        remote_backdrop_offset = (long)backdrop - (long)lcd_remote_framebuffer;
        lcd_remote_pixelfuncs = lcd_remote_pixelfuncs_backdrop;
        lcd_remote_blockfuncs = lcd_remote_blockfuncs_backdrop;
    }
    else
    {
        remote_backdrop_offset = 0;
        lcd_remote_pixelfuncs = lcd_remote_pixelfuncs_bgcolor;
        lcd_remote_blockfuncs = lcd_remote_blockfuncs_bgcolor;
    }
}

fb_remote_data* lcd_remote_get_backdrop(void)
{
    return remote_backdrop;
}

static inline void setblock(fb_remote_data *address, unsigned mask, unsigned bits)
{
    unsigned data = *address;

    bits    ^= data;
    *address = data ^ (bits & mask);
}

/*** drawing functions ***/

/* Clear the whole display */
void lcd_remote_clear_display(void)
{
    if (drawmode & DRMODE_INVERSEVID)
    {
        memset(lcd_remote_framebuffer, fg_pattern,
               sizeof lcd_remote_framebuffer);
    }
    else
    {
        if (remote_backdrop)
            memcpy(lcd_remote_framebuffer, remote_backdrop,
                   sizeof lcd_remote_framebuffer);
        else
            memset(lcd_remote_framebuffer, bg_pattern,
                   sizeof lcd_remote_framebuffer);
    }
    scrolling_lines = 0;
}

/* Set a single pixel */
void lcd_remote_drawpixel(int x, int y)
{
    if (((unsigned)x < LCD_REMOTE_WIDTH) && ((unsigned)y < LCD_REMOTE_HEIGHT))
        lcd_remote_pixelfuncs[drawmode](x, y);
}

/* Draw a line */
void lcd_remote_drawline(int x1, int y1, int x2, int y2)
{
    int numpixels;
    int i;
    int deltax, deltay;
    int d, dinc1, dinc2;
    int x, xinc1, xinc2;
    int y, yinc1, yinc2;
    lcd_remote_pixelfunc_type *pfunc = lcd_remote_pixelfuncs[drawmode];

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
        if (((unsigned)x < LCD_REMOTE_WIDTH) && ((unsigned)y < LCD_REMOTE_HEIGHT))
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
void lcd_remote_hline(int x1, int x2, int y)
{
    int x;
    fb_remote_data *dst, *dst_end;
    unsigned mask;
    lcd_remote_blockfunc_type *bfunc;

    /* direction flip */
    if (x2 < x1)
    {
        x = x1;
        x1 = x2;
        x2 = x;
    }

    /* nothing to draw? */
    if (((unsigned)y >= LCD_REMOTE_HEIGHT) || (x1 >= LCD_REMOTE_WIDTH)
        || (x2 < 0))
        return;

    /* clipping */
    if (x1 < 0)
        x1 = 0;
    if (x2 >= LCD_REMOTE_WIDTH)
        x2 = LCD_REMOTE_WIDTH-1;

    bfunc = lcd_remote_blockfuncs[drawmode];
    dst   = &lcd_remote_framebuffer[y>>3][x1];
    mask  = 0x0101 << (y & 7);

    dst_end = dst + x2 - x1;
    do
        bfunc(dst++, mask, 0xFFFFu);
    while (dst <= dst_end);
}

/* Draw a vertical line (optimised) */
void lcd_remote_vline(int x, int y1, int y2)
{
    int ny;
    fb_remote_data *dst;
    unsigned mask, mask_bottom;
    lcd_remote_blockfunc_type *bfunc;

    /* direction flip */
    if (y2 < y1)
    {
        ny = y1;
        y1 = y2;
        y2 = ny;
    }

    /* nothing to draw? */
    if (((unsigned)x >= LCD_REMOTE_WIDTH) || (y1 >= LCD_REMOTE_HEIGHT)
        || (y2 < 0))
        return;

    /* clipping */
    if (y1 < 0)
        y1 = 0;
    if (y2 >= LCD_REMOTE_HEIGHT)
        y2 = LCD_REMOTE_HEIGHT-1;

    bfunc = lcd_remote_blockfuncs[drawmode];
    dst   = &lcd_remote_framebuffer[y1>>3][x];
    ny    = y2 - (y1 & ~7);
    mask  = (0xFFu << (y1 & 7)) & 0xFFu;
    mask |= mask << 8;
    mask_bottom  = 0xFFu >> (~ny & 7);
    mask_bottom |= mask_bottom << 8;

    for (; ny >= 8; ny -= 8)
    {
        bfunc(dst, mask, 0xFFFFu);
        dst += LCD_REMOTE_WIDTH;
        mask = 0xFFFFu;
    }
    mask &= mask_bottom;
    bfunc(dst, mask, 0xFFFFu);
}

/* Draw a rectangular box */
void lcd_remote_drawrect(int x, int y, int width, int height)
{
    if ((width <= 0) || (height <= 0))
        return;

    int x2 = x + width - 1;
    int y2 = y + height - 1;

    lcd_remote_vline(x, y, y2);
    lcd_remote_vline(x2, y, y2);
    lcd_remote_hline(x, x2, y);
    lcd_remote_hline(x, x2, y2);
}

/* Fill a rectangular area */
void lcd_remote_fillrect(int x, int y, int width, int height)
{
    int ny;
    fb_remote_data *dst, *dst_end;
    unsigned mask, mask_bottom;
    unsigned bits = 0;
    lcd_remote_blockfunc_type *bfunc;
    bool fillopt = false;

    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= LCD_REMOTE_WIDTH)
        || (y >= LCD_REMOTE_HEIGHT) || (x + width <= 0) || (y + height <= 0))
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
    if (x + width > LCD_REMOTE_WIDTH)
        width = LCD_REMOTE_WIDTH - x;
    if (y + height > LCD_REMOTE_HEIGHT)
        height = LCD_REMOTE_HEIGHT - y;

    if (drawmode & DRMODE_INVERSEVID)
    {
        if ((drawmode & DRMODE_BG) && !remote_backdrop)
        {
            fillopt = true;
            bits = bg_pattern;
        }
    }
    else
    {
        if (drawmode & DRMODE_FG)
        {
            fillopt = true;
            bits = fg_pattern;
        }
    }
    bfunc = lcd_remote_blockfuncs[drawmode];
    dst   = &lcd_remote_framebuffer[y>>3][x];
    ny    = height - 1 + (y & 7);
    mask  = (0xFFu << (y & 7)) & 0xFFu;
    mask |= mask << 8;
    mask_bottom  = 0xFFu >> (~ny & 7);
    mask_bottom |= mask_bottom << 8;

    for (; ny >= 8; ny -= 8)
    {
        if (fillopt && (mask == 0xFFFFu))
            memset16(dst, bits, width);
        else
        {
            fb_remote_data *dst_row = dst;

            dst_end = dst_row + width;
            do
                bfunc(dst_row++, mask, 0xFFFFu);
            while (dst_row < dst_end);
        }

        dst += LCD_REMOTE_WIDTH;
        mask = 0xFFFFu;
    }
    mask &= mask_bottom;

    if (fillopt && (mask == 0xFFFFu))
        memset16(dst, bits, width);
    else
    {
        dst_end = dst + width;
        do
            bfunc(dst++, mask, 0xFFFFu);
        while (dst < dst_end);
    }
}

/* About Rockbox' internal monochrome bitmap format:
 *
 * A bitmap contains one bit for every pixel that defines if that pixel is
 * black (1) or white (0). Bits within a byte are arranged vertically, LSB
 * at top.
 * The bytes are stored in row-major order, with byte 0 being top left,
 * byte 1 2nd from left etc. The first row of bytes defines pixel rows
 * 0..7, the second row defines pixel row 8..15 etc.
 *
 * This is similar to the internal lcd hw format. */

/* Draw a partial monochrome bitmap */
void lcd_remote_mono_bitmap_part(const unsigned char *src, int src_x, int src_y,
                                 int stride, int x, int y, int width, int height)
                                 ICODE_ATTR;
void lcd_remote_mono_bitmap_part(const unsigned char *src, int src_x, int src_y,
                                 int stride, int x, int y, int width, int height)
{
    int shift, ny;
    fb_remote_data *dst, *dst_end;
    unsigned data, mask, mask_bottom;
    lcd_remote_blockfunc_type *bfunc;

    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= LCD_REMOTE_WIDTH)
        || (y >= LCD_REMOTE_HEIGHT) || (x + width <= 0) || (y + height <= 0))
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
    if (x + width > LCD_REMOTE_WIDTH)
        width = LCD_REMOTE_WIDTH - x;
    if (y + height > LCD_REMOTE_HEIGHT)
        height = LCD_REMOTE_HEIGHT - y;

    src    += stride * (src_y >> 3) + src_x; /* move starting point */
    src_y  &= 7;
    y      -= src_y;
    dst    = &lcd_remote_framebuffer[y>>3][x];
    shift  = y & 7;
    ny     = height - 1 + shift + src_y;

    bfunc  = lcd_remote_blockfuncs[drawmode];
    mask   = 0xFFu << (shift + src_y);
             /* not byte-doubled here because shift+src_y can be > 7 */
    mask_bottom  = 0xFFu >> (~ny & 7);
    mask_bottom |= mask_bottom << 8;

    if (shift == 0)
    {
        mask &= 0xFFu;
        mask |= mask << 8;

        for (; ny >= 8; ny -= 8)
        {
            const unsigned char *src_row = src;
            fb_remote_data *dst_row = dst;

            dst_end = dst_row + width;
            do
            {
                data = *src_row++;
                bfunc(dst_row++, mask, data | (data << 8));
            }
            while (dst_row < dst_end);

            src += stride;
            dst += LCD_REMOTE_WIDTH;
            mask = 0xFFFFu;
        }
        mask  &= mask_bottom;

        dst_end = dst + width;
        do
        {
            data = *src++;
            bfunc(dst++, mask, data | (data << 8));
        }
        while (dst < dst_end);
    }
    else
    {
        unsigned ddata;

        dst_end = dst + width;
        do
        {
            const unsigned char *src_col = src++;
            fb_remote_data *dst_col = dst++;
            unsigned mask_col = mask & 0xFFu;

            mask_col |= mask_col << 8;
            data = 0;

            for (y = ny; y >= 8; y -= 8)
            {
                data |= *src_col << shift;

                if (mask_col)
                {
                    ddata = data & 0xFFu;
                    bfunc(dst_col, mask_col, ddata | (ddata << 8));
                    mask_col = 0xFFFFu;
                }
                else
                {
                    mask_col  = (mask >> 8) & 0xFFu;
                    mask_col |= mask_col << 8;
                }

                src_col += stride;
                dst_col += LCD_REMOTE_WIDTH;
                data >>= 8;
            }
            data |= *src_col << shift;
            mask_col &= mask_bottom;
            ddata = data & 0xFFu;
            bfunc(dst_col, mask_col, ddata | (ddata << 8));
        }
        while (dst < dst_end);
    }
}

/* Draw a full monochrome bitmap */
void lcd_remote_mono_bitmap(const unsigned char *src, int x, int y, int width,
                            int height)
{
    lcd_remote_mono_bitmap_part(src, 0, 0, width, x, y, width, height);
}

/* About Rockbox' internal native bitmap format:
 *
 * A bitmap contains one bit in each byte of a pair of bytes for every pixel.
 * 00 = white, 01 = light grey, 10 = dark grey, 11 = black. Bits within a byte
 * are arranged vertically, LSB at top.
 * The pairs of bytes are stored as shorts, in row-major order, with word 0
 * being top left, word 1 2nd from left etc. The first row of words defines
 * pixel rows 0..7, the second row defines pixel row 8..15 etc.
 *
 * This is the same as the internal lcd hw format. */

/* Draw a partial native bitmap */
void lcd_remote_bitmap_part(const fb_remote_data *src, int src_x, int src_y,
                            int stride, int x, int y, int width, int height)
                            ICODE_ATTR;
void lcd_remote_bitmap_part(const fb_remote_data *src, int src_x, int src_y,
                            int stride, int x, int y, int width, int height)
{
    int shift, ny;
    fb_remote_data *dst, *dst_end;
    unsigned mask, mask_bottom;

    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= LCD_REMOTE_WIDTH)
        || (y >= LCD_REMOTE_HEIGHT) || (x + width <= 0) || (y + height <= 0))
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
    if (x + width > LCD_REMOTE_WIDTH)
        width = LCD_REMOTE_WIDTH - x;
    if (y + height > LCD_REMOTE_HEIGHT)
        height = LCD_REMOTE_HEIGHT - y;

    src   += stride * (src_y >> 3) + src_x; /* move starting point */
    src_y &= 7;
    y     -= src_y;
    dst    = &lcd_remote_framebuffer[y>>3][x];
    shift  = y & 7;
    ny     = height - 1 + shift + src_y;

    mask   = 0xFFu << (shift + src_y);
             /* not byte-doubled here because shift+src_y can be > 7 */
    mask_bottom  = 0xFFu >> (~ny & 7);
    mask_bottom |= mask_bottom << 8;

    if (shift == 0)
    {
        mask &= 0xFFu;
        mask |= mask << 8;

        for (; ny >= 8; ny -= 8)
        {
            if (mask == 0xFFFFu)
                memcpy(dst, src, width * sizeof(fb_remote_data));
            else
            {
                const fb_remote_data *src_row = src;
                fb_remote_data *dst_row = dst;

                dst_end = dst_row + width;
                do
                    setblock(dst_row++, mask, *src_row++);
                while (dst_row < dst_end);
            }
            src += stride;
            dst += LCD_REMOTE_WIDTH;
            mask = 0xFFFFu;
        }
        mask &= mask_bottom;

        if (mask == 0xFFFFu)
            memcpy(dst, src, width * sizeof(fb_remote_data));
        else
        {
            dst_end = dst + width;
            do
                setblock(dst++, mask, *src++);
            while (dst < dst_end);
        }
    }
    else
    {
        unsigned datamask = (0xFFu << shift) & 0xFFu;

        datamask |= datamask << 8;

        dst_end = dst + width;
        do
        {
            const fb_remote_data *src_col = src++;
            fb_remote_data *dst_col = dst++;
            unsigned mask_col = mask & 0xFFu;
            unsigned data, olddata = 0;

            mask_col |= mask_col << 8;

            for (y = ny; y >= 8; y -= 8)
            {
                data = *src_col << shift;

                if (mask_col)
                {
                    setblock(dst_col, mask_col,
                             olddata ^((olddata ^ data) & datamask));
                    mask_col = 0xFFFFu;
                }
                else
                {
                    mask_col  = (mask << 8) & 0xFFu;
                    mask_col |= mask_col << 8;
                }
                src_col += stride;
                dst_col += LCD_REMOTE_WIDTH;
                olddata = data >> 8;
            }
            data = *src_col << shift;
            setblock(dst_col, mask_col & mask_bottom,
                     olddata ^((olddata ^ data) & datamask));
        }
        while (dst < dst_end);
    }
}

/* Draw a full native bitmap */
void lcd_remote_bitmap(const fb_remote_data *src, int x, int y, int width,
                       int height)
{
    lcd_remote_bitmap_part(src, 0, 0, width, x, y, width, height);
}

/* put a string at a given pixel position, skipping first ofs pixel columns */
static void lcd_remote_putsxyofs(int x, int y, int ofs, const unsigned char *str)
{
    unsigned short ch;
    unsigned short *ucs;
    struct font* pf = font_get(curfont);

    ucs = bidi_l2v(str, 1);

    while ((ch = *ucs++) != 0 && x < LCD_REMOTE_WIDTH)
    {
        int width;
        const unsigned char *bits;

        /* get proportional width and glyph bits */
        width = font_get_width(pf, ch);

        if (ofs > width)
        {
            ofs -= width;
            continue;
        }

        bits = font_get_bits(pf, ch);

        lcd_remote_mono_bitmap_part(bits, ofs, 0, width, x, y, width - ofs,
                                    pf->height);

        x += width - ofs;
        ofs = 0;
    }
}

/* put a string at a given pixel position */
void lcd_remote_putsxy(int x, int y, const unsigned char *str)
{
    lcd_remote_putsxyofs(x, y, 0, str);
}

/*** line oriented text output ***/

/* put a string at a given char position */
void lcd_remote_puts(int x, int y, const unsigned char *str)
{
    lcd_remote_puts_style_offset(x, y, str, STYLE_DEFAULT, 0);
}

void lcd_remote_puts_style(int x, int y, const unsigned char *str, int style)
{
    lcd_remote_puts_style_offset(x, y, str, style, 0);
}

void lcd_remote_puts_offset(int x, int y, const unsigned char *str, int offset)
{
    lcd_remote_puts_style_offset(x, y, str, STYLE_DEFAULT, offset);
}

/* put a string at a given char position, style, and pixel position,
 * skipping first offset pixel columns */
void lcd_remote_puts_style_offset(int x, int y, const unsigned char *str,
                                  int style, int offset)
{
    int xpos,ypos,w,h,xrect;
    int lastmode = drawmode;

    /* make sure scrolling is turned off on the line we are updating */
    scrolling_lines &= ~(1 << y);

    if(!str || !str[0])
        return;

    lcd_remote_getstringsize(str, &w, &h);
    xpos = xmargin + x*w / utf8length((char *)str);
    ypos = ymargin + y*h;
    drawmode = (style & STYLE_INVERT) ?
               (DRMODE_SOLID|DRMODE_INVERSEVID) : DRMODE_SOLID;
    lcd_remote_putsxyofs(xpos, ypos, offset, str);
    drawmode ^= DRMODE_INVERSEVID;
    xrect = xpos + MAX(w - offset, 0);
    lcd_remote_fillrect(xrect, ypos, LCD_REMOTE_WIDTH - xrect, h);
    drawmode = lastmode;
}

/*** scrolling ***/

/* Reverse the invert setting of the scrolling line (if any) at given char
   position.  Setting will go into affect next time line scrolls. */
void lcd_remote_invertscroll(int x, int y)
{
    struct scrollinfo* s;

    (void)x;

    if(y>=SCROLLABLE_LINES) return;

    s = &scroll[y];
    s->invert = !s->invert;
}

void lcd_remote_stop_scroll(void)
{
    scrolling_lines=0;
}

void lcd_remote_scroll_speed(int speed)
{
    scroll_ticks = scroll_tick_table[speed];
}

void lcd_remote_scroll_step(int step)
{
    scroll_step = step;
}

void lcd_remote_scroll_delay(int ms)
{
    scroll_delay = ms / (HZ / 10);
}

void lcd_remote_bidir_scroll(int percent)
{
    bidir_limit = percent;
}

void lcd_remote_puts_scroll(int x, int y, const unsigned char *string)
{
    lcd_remote_puts_scroll_style(x, y, string, STYLE_DEFAULT);
}

void lcd_remote_puts_scroll_style(int x, int y, const unsigned char *string, int style)
{
     lcd_remote_puts_scroll_style_offset(x, y, string, style, 0);
}

void lcd_remote_puts_scroll_offset(int x, int y, const unsigned char *string, int offset)
{
     lcd_remote_puts_scroll_style_offset(x, y, string, STYLE_DEFAULT, offset);
}

void lcd_remote_puts_scroll_style_offset(int x, int y, const unsigned char *string,
                                         int style, int offset)
{
    struct scrollinfo* s;
    int w, h;

    if(y>=SCROLLABLE_LINES) return;

    s = &scroll[y];

    s->start_tick = current_tick + scroll_delay;
    s->invert = false;
    if (style & STYLE_INVERT) {
        s->invert = true;
        lcd_remote_puts_style_offset(x,y,string,STYLE_INVERT,offset);
    }
    else
        lcd_remote_puts_offset(x,y,string,offset);

    lcd_remote_getstringsize(string, &w, &h);

    if (LCD_REMOTE_WIDTH - x * 8 - xmargin < w) {
        /* prepare scroll line */
        char *end;

        memset(s->line, 0, sizeof s->line);
        strcpy(s->line, (char *)string);

        /* get width */
        s->width = lcd_remote_getstringsize((unsigned char *)s->line, &w, &h);

        /* scroll bidirectional or forward only depending on the string
           width */
        if ( bidir_limit ) {
            s->bidir = s->width < (LCD_REMOTE_WIDTH - xmargin) *
                (100 + bidir_limit) / 100;
        }
        else
            s->bidir = false;

        if (!s->bidir) { /* add spaces if scrolling in the round */
            strcat(s->line, "   ");
            /* get new width incl. spaces */
            s->width = lcd_remote_getstringsize((unsigned char *)s->line, &w, &h);
        }

        end = strchr(s->line, '\0');
        strncpy(end, (char *)string, LCD_REMOTE_WIDTH/2);

        s->len = utf8length((char *)string);
        s->offset = offset;
        s->startx = xmargin + x * s->width / s->len;;
        s->backward = false;
        scrolling_lines |= (1<<y);
    }
    else
        /* force a bit switch-off since it doesn't scroll */
        scrolling_lines &= ~(1<<y);
}

static void scroll_thread(void)
{
    struct font* pf;
    struct scrollinfo* s;
    int index;
    int xpos, ypos;
    int lastmode;
    long delay = 0;
    long next_tick = current_tick;
#ifndef SIMULATOR
    struct event ev;
#endif

    /* initialize scroll struct array */
    scrolling_lines = 0;

    while ( 1 ) {

#ifdef SIMULATOR
        sleep(delay);
#else
        if (remote_initialized)
            queue_wait_w_tmo(&remote_scroll_queue, &ev, delay);
        else
            queue_wait(&remote_scroll_queue, &ev);

        switch (ev.id)
        {
            case REMOTE_INIT_LCD:
                lcd_remote_on();
                lcd_remote_update();
                break;

            case REMOTE_DEINIT_LCD:
                lcd_remote_off();
                break;
        }

        delay = next_tick - current_tick - 1;
        if (delay >= 0)
            continue;
#endif

        for ( index = 0; index < SCROLLABLE_LINES; index++ ) {
            /* really scroll? */
            if ( !(scrolling_lines&(1<<index)) )
                continue;

            s = &scroll[index];

            /* check pause */
            if (TIME_BEFORE(current_tick, s->start_tick))
                continue;

            if (s->backward)
                s->offset -= scroll_step;
            else
                s->offset += scroll_step;

            pf = font_get(curfont);
            xpos = s->startx;
            ypos = ymargin + index * pf->height;

            if (s->bidir) { /* scroll bidirectional */
                if (s->offset <= 0) {
                    /* at beginning of line */
                    s->offset = 0;
                    s->backward = false;
                    s->start_tick = current_tick + scroll_delay * 2;
                }
                if (s->offset >= s->width - (LCD_REMOTE_WIDTH - xpos)) {
                    /* at end of line */
                    s->offset = s->width - (LCD_REMOTE_WIDTH - xpos);
                    s->backward = true;
                    s->start_tick = current_tick + scroll_delay * 2;
                }
            }
            else {
                /* scroll forward the whole time */
                if (s->offset >= s->width)
                    s->offset %= s->width;
            }

            lastmode = drawmode;
            drawmode = s->invert ?
                       (DRMODE_SOLID|DRMODE_INVERSEVID) : DRMODE_SOLID;
            lcd_remote_putsxyofs(xpos, ypos, s->offset, s->line);
            drawmode = lastmode;
            lcd_remote_update_rect(xpos, ypos, LCD_REMOTE_WIDTH - xpos, pf->height);
        }


        next_tick += scroll_ticks;
        delay = next_tick - current_tick - 1;
        if (delay < 0)
        {
            next_tick = current_tick + 1;
            delay = 0;
        }
    }
}

/* LCD init */
void lcd_remote_init(void)
{
#ifndef SIMULATOR
    /* Call device specific init */
    lcd_remote_init_device();
    /* private queue */
    queue_init(&remote_scroll_queue, false);
#endif
    create_thread(scroll_thread, scroll_stack,
                  sizeof(scroll_stack), scroll_name IF_PRIO(, PRIORITY_USER_INTERFACE)
		  IF_COP(, CPU, false));
}
