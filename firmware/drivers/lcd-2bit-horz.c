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

#define SCROLLABLE_LINES 26

/*** globals ***/

unsigned char lcd_framebuffer[LCD_HEIGHT][LCD_FBWIDTH] IBSS_ATTR;

static const unsigned char dibits[16] ICONST_ATTR = {
    0x00, 0x03, 0x0C, 0x0F, 0x30, 0x33, 0x3C, 0x3F,
    0xC0, 0xC3, 0xCC, 0xCF, 0xF0, 0xF3, 0xFC, 0xFF
};

static const unsigned char pixmask[4] ICONST_ATTR = {
    0x03, 0x0C, 0x30, 0xC0
};

static unsigned fg_pattern IDATA_ATTR = 0xFF; /* initially black */
static unsigned bg_pattern IDATA_ATTR = 0x00; /* initially white */
static int drawmode = DRMODE_SOLID;
static int xmargin = 0;
static int ymargin = 0;
static int curfont = FONT_SYSFIXED;

/* scrolling */
static volatile int scrolling_lines=0; /* Bitpattern of which lines are scrolling */
static void scroll_thread(void);
static long scroll_stack[DEFAULT_STACK_SIZE/sizeof(long)];
static const char scroll_name[] = "scroll";
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


/* LCD init */
void lcd_init(void)
{
    lcd_clear_display();
    /* Call device specific init */
    lcd_init_device();
    create_thread(scroll_thread, scroll_stack,
                  sizeof(scroll_stack), scroll_name);
}

int lcd_default_contrast(void)
{
#if defined(IPOD_MINI) || defined(IPOD_MINI2G)
    return 42;
#else
    return 35;
#endif
}

/*** parameter handling ***/

void lcd_set_drawmode(int mode)
{
    drawmode = mode & (DRMODE_SOLID|DRMODE_INVERSEVID);
}

int lcd_get_drawmode(void)
{
    return drawmode;
}

void lcd_set_foreground(unsigned brightness)
{
    fg_pattern = 0x55 * (~brightness & 3);
}

unsigned lcd_get_foreground(void)
{
    return ~fg_pattern & 3;
}

void lcd_set_background(unsigned brightness)
{
    bg_pattern = 0x55 * (~brightness & 3);
}

unsigned lcd_get_background(void)
{
    return ~bg_pattern & 3;
}

void lcd_set_drawinfo(int mode, unsigned fg_brightness, unsigned bg_brightness)
{
    lcd_set_drawmode(mode);
    lcd_set_foreground(fg_brightness);
    lcd_set_background(bg_brightness);
}

void lcd_setmargins(int x, int y)
{
    xmargin = x;
    ymargin = y;
}

int lcd_getxmargin(void)
{
    return xmargin;
}

int lcd_getymargin(void)
{
    return ymargin;
}

void lcd_setfont(int newfont)
{
    curfont = newfont;
}

int lcd_getstringsize(const unsigned char *str, int *w, int *h)
{
    return font_getstringsize(str, w, h, curfont);
}

/*** low-level drawing functions ***/

static void setpixel(int x, int y)
{
    unsigned char *data = &lcd_framebuffer[y][x>>2];
    unsigned mask = pixmask[x & 3];
    *data = (*data & ~mask) | (fg_pattern & mask);
}

static void clearpixel(int x, int y)
{
    unsigned char *data = &lcd_framebuffer[y][x>>2];
    unsigned mask = pixmask[x & 3];
    *data = (*data & ~mask) | (bg_pattern & mask);
}

static void flippixel(int x, int y)
{
    lcd_framebuffer[y][x>>2] ^=  pixmask[x & 3];
}

static void nopixel(int x, int y)
{
    (void)x;
    (void)y;
}

lcd_pixelfunc_type* const lcd_pixelfuncs[8] = {
    flippixel, nopixel, setpixel, setpixel,
    nopixel, clearpixel, nopixel, clearpixel
};

/* 'mask' and 'bits' contain 2 bits per pixel */
static void flipblock(unsigned char *address, unsigned mask, unsigned bits)
                      ICODE_ATTR;
static void flipblock(unsigned char *address, unsigned mask, unsigned bits)
{
    *address ^= bits & mask;
}

static void bgblock(unsigned char *address, unsigned mask, unsigned bits)
                    ICODE_ATTR;
static void bgblock(unsigned char *address, unsigned mask, unsigned bits)
{
    mask &= ~bits;
    *address = (*address & ~mask) | (bg_pattern & mask);
}

static void fgblock(unsigned char *address, unsigned mask, unsigned bits)
                    ICODE_ATTR;
static void fgblock(unsigned char *address, unsigned mask, unsigned bits)
{
    mask &= bits;
    *address = (*address & ~mask) | (fg_pattern & mask);
}

static void solidblock(unsigned char *address, unsigned mask, unsigned bits)
                       ICODE_ATTR;
static void solidblock(unsigned char *address, unsigned mask, unsigned bits)
{
    *address = (*address & ~mask) | (bits & mask & fg_pattern)
                                  | (~bits & mask & bg_pattern);
}

static void flipinvblock(unsigned char *address, unsigned mask, unsigned bits)
                         ICODE_ATTR;
static void flipinvblock(unsigned char *address, unsigned mask, unsigned bits)
{
    *address ^= ~bits & mask;
}

static void bginvblock(unsigned char *address, unsigned mask, unsigned bits)
                       ICODE_ATTR;
static void bginvblock(unsigned char *address, unsigned mask, unsigned bits)
{
    mask &= bits;
    *address = (*address & ~mask) | (bg_pattern & mask);
}

static void fginvblock(unsigned char *address, unsigned mask, unsigned bits)
                       ICODE_ATTR;
static void fginvblock(unsigned char *address, unsigned mask, unsigned bits)
{
    mask &= ~bits;
    *address = (*address & ~mask) | (fg_pattern & mask);
}

static void solidinvblock(unsigned char *address, unsigned mask, unsigned bits)
                          ICODE_ATTR;
static void solidinvblock(unsigned char *address, unsigned mask, unsigned bits)
{
    *address = (*address & ~mask) | (~bits & mask & fg_pattern)
                                  | (bits & mask & bg_pattern);
}

lcd_blockfunc_type* const lcd_blockfuncs[8] = {
    flipblock, bgblock, fgblock, solidblock,
    flipinvblock, bginvblock, fginvblock, solidinvblock
};

static inline void setblock(unsigned char *address, unsigned mask, unsigned bits)
{
    *address = (*address & ~mask) | (bits & mask);
}

/*** drawing functions ***/

/* Clear the whole display */
void lcd_clear_display(void)
{
    unsigned bits = (drawmode & DRMODE_INVERSEVID) ? fg_pattern : bg_pattern;

    memset(lcd_framebuffer, bits, sizeof lcd_framebuffer);
    scrolling_lines = 0;
}

/* Set a single pixel */
void lcd_drawpixel(int x, int y)
{
    if (((unsigned)x < LCD_WIDTH) && ((unsigned)y < LCD_HEIGHT))
        lcd_pixelfuncs[drawmode](x, y);
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
    lcd_pixelfunc_type *pfunc = lcd_pixelfuncs[drawmode];

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
        if (((unsigned)x < LCD_WIDTH) && ((unsigned)y < LCD_HEIGHT))
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
    if (((unsigned)y >= LCD_HEIGHT) || (x1 >= LCD_WIDTH) || (x2 < 0))
        return;  
    
    /* clipping */
    if (x1 < 0)
        x1 = 0;
    if (x2 >= LCD_WIDTH)
        x2 = LCD_WIDTH-1;

    bfunc = lcd_blockfuncs[drawmode];
    dst   = &lcd_framebuffer[y][x1>>2];
    nx    = x2 - (x1 & ~3);
    mask  = 0xFFu << (2 * (x1 & 3));
    mask_right = 0xFFu >> (2 * (~nx & 3));
    
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
    if (((unsigned)x >= LCD_WIDTH) || (y1 >= LCD_HEIGHT) || (y2 < 0))
        return;  
    
    /* clipping */
    if (y1 < 0)
        y1 = 0;
    if (y2 >= LCD_HEIGHT)
        y2 = LCD_HEIGHT-1;
        
    bfunc = lcd_blockfuncs[drawmode];
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
    if ((width <= 0) || (height <= 0) || (x >= LCD_WIDTH) || (y >= LCD_HEIGHT)
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
    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x;
    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y;

    bfunc = lcd_blockfuncs[drawmode];
    dst   = &lcd_framebuffer[y][x>>2];
    nx    = width - 1 + (x & 3);
    mask  = 0xFFu << (2 * (x & 3));
    mask_right = 0xFFu >> (2 * (~nx & 3));
    
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
 * black (1) or white (0). Bits within a byte are arranged horizontally, LSB
 * at top.
 * The bytes are stored in row-major order, with byte 0 being top left,
 * byte 1 2nd from left etc. The first row of bytes defines pixel row
 * 0, the second row defines pixel row 1 etc.
 *
 * This is similar to the internal lcd hw format. */

/* Draw a partial monochrome bitmap */
void lcd_mono_bitmap_part(const unsigned char *src, int src_x, int src_y,
                          int stride, int x, int y, int width, int height)
                          ICODE_ATTR;
void lcd_mono_bitmap_part(const unsigned char *src, int src_x, int src_y,
                          int stride, int x, int y, int width, int height)
{
    int ny, nx, ymax;
    const unsigned char * src_end;
    lcd_pixelfunc_type* fgfunc;
    lcd_pixelfunc_type* bgfunc;

    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= LCD_WIDTH) || (y >= LCD_HEIGHT)
        || (x + width <= 0) || (y + height <= 0))
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
    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x;
    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y;

    src += stride * (src_y >> 3) + src_x; /* move starting point */
    src_y  &= 7;
    src_end = src + width;

    fgfunc = lcd_pixelfuncs[drawmode];    
    bgfunc = lcd_pixelfuncs[drawmode ^ DRMODE_INVERSEVID];    
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
 * LSB at the left.
 * The bytes are stored in row-major order, with byte 0 being top left,
 * byte 1 2nd from left etc. Each row of bytes defines one pixel row.
 *
 * This is the same as the internal lcd hw format. */

/* Draw a partial native bitmap */
void lcd_bitmap_part(const unsigned char *src, int src_x, int src_y,
                     int stride, int x, int y, int width, int height)
                     ICODE_ATTR;
void lcd_bitmap_part(const unsigned char *src, int src_x, int src_y,
                     int stride, int x, int y, int width, int height)
{
    int  shift, nx;
    unsigned char *dst, *dst_end;
    unsigned mask, mask_right;

    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= LCD_WIDTH) || (y >= LCD_HEIGHT)
        || (x + width <= 0) || (y + height <= 0))
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
    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x;
    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y;

    stride = (stride + 3) >> 2; /* convert to no. of bytes */

    src   += stride * src_y + (src_x >> 2); /* move starting point */
    src_x &= 3;
    x     -= src_x;
    dst   = &lcd_framebuffer[y][x>>2];
    shift = x & 3;
    nx    = width - 1 + shift + src_x;

    mask   = 0xFFu << (2 * (shift + src_x));
    mask_right = 0xFFu >> (2 * (~nx & 3));
    
    shift *= 2;
    dst_end = dst + height * LCD_FBWIDTH;
    do
    {
        const unsigned char *src_row = src;
        unsigned char *dst_row = dst;
        unsigned mask_row = mask;
        unsigned data = 0;
        
        for (x = nx; x >= 4; x -= 4)
        {
            data |= *src_row++ << shift;
            
            if (mask_row & 0xFF)
            {
                setblock(dst_row, mask_row, data);
                mask_row = 0xFF;
            }
            else
                mask_row >>= 8;

            dst_row++;
            data >>= 8;
        }
        data |= *src_row << shift;
        setblock(dst_row, mask_row & mask_right, data);

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
    struct font* pf = font_get(curfont);

    ucs = bidi_l2v(str, 1);

    while ((ch = *ucs++) != 0 && x < LCD_WIDTH)
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

        lcd_mono_bitmap_part(bits, ofs, 0, width, x, y, width - ofs, pf->height);
        
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
    int xpos,ypos,w,h;
    int lastmode = drawmode;

    /* make sure scrolling is turned off on the line we are updating */
    scrolling_lines &= ~(1 << y);

    if(!str || !str[0])
        return;

    lcd_getstringsize(str, &w, &h);
    xpos = xmargin + x*w / utf8length((char *)str);
    ypos = ymargin + y*h;
    drawmode = (style & STYLE_INVERT) ?
               (DRMODE_SOLID|DRMODE_INVERSEVID) : DRMODE_SOLID;
    lcd_putsxyofs(xpos, ypos, offset, str);
    drawmode ^= DRMODE_INVERSEVID;
    lcd_fillrect(xpos + w, ypos, LCD_WIDTH - (xpos + w), h);
    drawmode = lastmode;
}

/*** scrolling ***/

/* Reverse the invert setting of the scrolling line (if any) at given char
   position.  Setting will go into affect next time line scrolls. */
void lcd_invertscroll(int x, int y)
{
    struct scrollinfo* s;

    (void)x;

    s = &scroll[y];
    s->invert = !s->invert;
}

void lcd_stop_scroll(void)
{
    scrolling_lines=0;
}

void lcd_scroll_speed(int speed)
{
    scroll_ticks = scroll_tick_table[speed];
}

void lcd_scroll_step(int step)
{
    scroll_step = step;
}

void lcd_scroll_delay(int ms)
{
    scroll_delay = ms / (HZ / 10);
}

void lcd_bidir_scroll(int percent)
{
    bidir_limit = percent;
}

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

    s = &scroll[y];

    s->start_tick = current_tick + scroll_delay;
    s->invert = false;
    if (style & STYLE_INVERT) {
        s->invert = true;
        lcd_puts_style_offset(x,y,string,STYLE_INVERT,offset);
    }
    else
        lcd_puts_offset(x,y,string,offset);

    lcd_getstringsize(string, &w, &h);

    if (LCD_WIDTH - x * 8 - xmargin < w) {
        /* prepare scroll line */
        char *end;

        memset(s->line, 0, sizeof s->line);
        strcpy(s->line, (char *)string);

        /* get width */
        s->width = lcd_getstringsize((unsigned char *)s->line, &w, &h);

        /* scroll bidirectional or forward only depending on the string
           width */
        if ( bidir_limit ) {
            s->bidir = s->width < (LCD_WIDTH - xmargin) *
                (100 + bidir_limit) / 100;
        }
        else
            s->bidir = false;

        if (!s->bidir) { /* add spaces if scrolling in the round */
            strcat(s->line, "   ");
            /* get new width incl. spaces */
            s->width = lcd_getstringsize((unsigned char *)s->line, &w, &h);
        }

        end = strchr(s->line, '\0');
        strncpy(end, (char *)string, LCD_WIDTH/2);

        s->len = utf8length((char *)string);
        s->offset = offset;
        s->startx = x;
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

    /* initialize scroll struct array */
    scrolling_lines = 0;

    while ( 1 ) {
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
            xpos = xmargin + s->startx * s->width / s->len;
            ypos = ymargin + index * pf->height;

            if (s->bidir) { /* scroll bidirectional */
                if (s->offset <= 0) {
                    /* at beginning of line */
                    s->offset = 0;
                    s->backward = false;
                    s->start_tick = current_tick + scroll_delay * 2;
                }
                if (s->offset >= s->width - (LCD_WIDTH - xpos)) {
                    /* at end of line */
                    s->offset = s->width - (LCD_WIDTH - xpos);
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
            drawmode = (DRMODE_SOLID|DRMODE_INVERSEVID);
            lcd_fillrect(xpos, ypos, LCD_WIDTH - xpos, pf->height);
            drawmode = DRMODE_SOLID;
            lcd_putsxyofs(xpos, ypos, s->offset, (unsigned char *)s->line);
            if (s->invert)
            {
                drawmode = DRMODE_COMPLEMENT;
                lcd_fillrect(xpos, ypos, LCD_WIDTH - xpos, pf->height);
            }
            drawmode = lastmode;
            lcd_update_rect(xpos, ypos, LCD_WIDTH - xpos, pf->height);
        }

        sleep(scroll_ticks);
    }
}
