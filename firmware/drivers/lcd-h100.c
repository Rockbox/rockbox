/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 by Linus Nielsen Feltzing
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

/*** definitions ***/

/* LCD command codes */
#define LCD_CNTL_POWER_CONTROL          0x25
#define LCD_CNTL_VOLTAGE_SELECT         0x2b
#define LCD_CNTL_LINE_INVERT_DRIVE      0x36
#define LCD_CNTL_GRAY_SCALE_PATTERN     0x39
#define LCD_CNTL_TEMP_GRADIENT_SELECT   0x4e
#define LCD_CNTL_OSC_FREQUENCY          0x5f
#define LCD_CNTL_ON_OFF                 0xae
#define LCD_CNTL_OSC_ON_OFF             0xaa
#define LCD_CNTL_OFF_MODE               0xbe
#define LCD_CNTL_REVERSE                0xa6
#define LCD_CNTL_ALL_LIGHTING           0xa4
#define LCD_CNTL_COMMON_OUTPUT_STATUS   0xc4
#define LCD_CNTL_COLUMN_ADDRESS_DIR     0xa0
#define LCD_CNTL_NLINE_ON_OFF           0xe4
#define LCD_CNTL_DISPLAY_MODE           0x66
#define LCD_CNTL_DUTY_SET               0x6d
#define LCD_CNTL_ELECTRONIC_VOLUME      0x81
#define LCD_CNTL_DATA_INPUT_DIR         0x84
#define LCD_CNTL_DISPLAY_START_LINE     0x8a
#define LCD_CNTL_AREA_SCROLL            0x10

#define LCD_CNTL_PAGE                   0xb1
#define LCD_CNTL_COLUMN                 0x13
#define LCD_CNTL_DATA_WRITE             0x1d

#define SCROLLABLE_LINES 26

/*** globals ***/

unsigned char lcd_framebuffer[LCD_HEIGHT/4][LCD_WIDTH] IBSS_ATTR;

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

/*** driver code is in lcd.S ***/

/*** hardware configuration ***/

int lcd_default_contrast(void)
{
    return 28;
}

#ifndef SIMULATOR

void lcd_set_contrast(int val)
{
    lcd_write_command_ex(LCD_CNTL_ELECTRONIC_VOLUME, val, -1);
}

void lcd_set_invert_display(bool yesno)
{
    lcd_write_command(LCD_CNTL_REVERSE | (yesno?1:0));
}

/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
    if (yesno) 
    {
        lcd_write_command(LCD_CNTL_COLUMN_ADDRESS_DIR | 1);
        lcd_write_command(LCD_CNTL_COMMON_OUTPUT_STATUS | 0);
        lcd_write_command_ex(LCD_CNTL_DUTY_SET, 0x20, 0);
    }
    else
    {
        lcd_write_command(LCD_CNTL_COLUMN_ADDRESS_DIR | 0);
        lcd_write_command(LCD_CNTL_COMMON_OUTPUT_STATUS | 1);
        lcd_write_command_ex(LCD_CNTL_DUTY_SET, 0x20, 1);
    }
}

#endif /* !SIMULATOR */

/* LCD init */
#ifdef SIMULATOR

void lcd_init(void)
{
    create_thread(scroll_thread, scroll_stack,
                  sizeof(scroll_stack), scroll_name);
}
#else

void lcd_init(void)
{
    /* GPO35 is the LCD A0 pin
       GPO46 is LCD RESET */
    or_l(0x00004008, &GPIO1_OUT);
    or_l(0x00004008, &GPIO1_ENABLE);
    or_l(0x00004008, &GPIO1_FUNCTION);

    /* Reset LCD */
    sleep(1);
    and_l(~0x00004000, &GPIO1_OUT);
    sleep(1);
    or_l(0x00004000, &GPIO1_OUT);
    sleep(1);

    lcd_write_command(LCD_CNTL_COLUMN_ADDRESS_DIR | 0);   /* Normal */
    lcd_write_command(LCD_CNTL_COMMON_OUTPUT_STATUS | 1); /* Reverse dir */
    lcd_write_command(LCD_CNTL_REVERSE | 0); /* Reverse OFF */
    lcd_write_command(LCD_CNTL_ALL_LIGHTING | 0); /* Normal */
    lcd_write_command_ex(LCD_CNTL_DUTY_SET, 0x20, 1);
    lcd_write_command(LCD_CNTL_OFF_MODE | 1); /* OFF -> VCC on drivers */
    lcd_write_command_ex(LCD_CNTL_VOLTAGE_SELECT, 3, -1);
    lcd_write_command_ex(LCD_CNTL_ELECTRONIC_VOLUME, 0x1c, -1);
    lcd_write_command_ex(LCD_CNTL_TEMP_GRADIENT_SELECT, 0, -1);

    lcd_write_command_ex(LCD_CNTL_LINE_INVERT_DRIVE, 0x10, -1);
    lcd_write_command(LCD_CNTL_NLINE_ON_OFF | 1); /* N-line ON */

    lcd_write_command_ex(LCD_CNTL_OSC_FREQUENCY, 3, -1);
    lcd_write_command(LCD_CNTL_OSC_ON_OFF | 1); /* Oscillator ON */

    lcd_write_command_ex(LCD_CNTL_POWER_CONTROL, 0x16, -1);
    sleep(HZ/10);                               /* 100 ms pause */
    lcd_write_command_ex(LCD_CNTL_POWER_CONTROL, 0x17, -1);

    lcd_write_command_ex(LCD_CNTL_DISPLAY_START_LINE, 0, -1);
    lcd_write_command_ex(LCD_CNTL_GRAY_SCALE_PATTERN, 0x42, -1);
    lcd_write_command_ex(LCD_CNTL_DISPLAY_MODE, 0, -1); /* Greyscale mode */
    lcd_write_command(LCD_CNTL_DATA_INPUT_DIR | 0); /* Column mode */
    
    lcd_clear_display();
    lcd_update();
    lcd_write_command(LCD_CNTL_ON_OFF | 1); /* LCD ON */

    create_thread(scroll_thread, scroll_stack,
                  sizeof(scroll_stack), scroll_name);
}

/*** update functions ***/

/* Performance function that works with an external buffer
   note that by and bheight are in 8-pixel units! */
void lcd_blit(const unsigned char* data, int x, int by, int width,
              int bheight, int stride)
{
    const unsigned char *src, *src_end;
    unsigned char *dst_u, *dst_l;
    static unsigned char upper[LCD_WIDTH] IBSS_ATTR;
    static unsigned char lower[LCD_WIDTH] IBSS_ATTR;
    unsigned int byte;

    by *= 2;

    while (bheight--)
    {
        src = data;
        src_end = data + width;
        dst_u = upper;
        dst_l = lower;
        do
        {
            byte = *src++;
            *dst_u++ = dibits[byte & 0x0F];
            byte >>= 4;
            *dst_l++ = dibits[byte & 0x0F];
        }
        while (src < src_end);

        lcd_write_command_ex(LCD_CNTL_PAGE, by++, -1);
        lcd_write_command_ex(LCD_CNTL_COLUMN, x, -1);
        lcd_write_command(LCD_CNTL_DATA_WRITE);
        lcd_write_data(upper, width);

        lcd_write_command_ex(LCD_CNTL_PAGE, by++, -1);
        lcd_write_command_ex(LCD_CNTL_COLUMN, x, -1);
        lcd_write_command(LCD_CNTL_DATA_WRITE);
        lcd_write_data(lower, width);

        data += stride;
    }
}


/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void) ICODE_ATTR;
void lcd_update(void)
{
    int y;

    /* Copy display bitmap to hardware */
    for (y = 0; y < LCD_HEIGHT/4; y++)
    {
        lcd_write_command_ex(LCD_CNTL_PAGE, y, -1);
        lcd_write_command_ex(LCD_CNTL_COLUMN, 0, -1);

        lcd_write_command(LCD_CNTL_DATA_WRITE);
        lcd_write_data (lcd_framebuffer[y], LCD_WIDTH);
    }
}

/* Update a fraction of the display. */
void lcd_update_rect(int, int, int, int) ICODE_ATTR;
void lcd_update_rect(int x, int y, int width, int height)
{
    int ymax;

    /* The Y coordinates have to work on even 8 pixel rows */
    ymax = (y + height-1) >> 2;
    y >>= 2;

    if(x + width > LCD_WIDTH)
        width = LCD_WIDTH - x;
    if (width <= 0)
        return; /* nothing left to do, 0 is harmful to lcd_write_data() */
    if(ymax >= LCD_HEIGHT/4)
        ymax = LCD_HEIGHT/4-1;

    /* Copy specified rectange bitmap to hardware */
    for (; y <= ymax; y++)
    {
        lcd_write_command_ex(LCD_CNTL_PAGE, y, -1);
        lcd_write_command_ex(LCD_CNTL_COLUMN, x, -1);

        lcd_write_command(LCD_CNTL_DATA_WRITE);
        lcd_write_data (&lcd_framebuffer[y][x], width);
    }
}
#endif /* !SIMULATOR */

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
    unsigned char *data = &lcd_framebuffer[y>>2][x];
    unsigned mask = pixmask[y & 3];
    *data = (*data & ~mask) | (fg_pattern & mask);
}

static void clearpixel(int x, int y)
{
    unsigned char *data = &lcd_framebuffer[y>>2][x];
    unsigned mask = pixmask[y & 3];
    *data = (*data & ~mask) | (bg_pattern & mask);
}

static void flippixel(int x, int y)
{
    lcd_framebuffer[y>>2][x] ^=  pixmask[y & 3];
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
    int x;
    unsigned char *dst, *dst_end;
    unsigned mask;
    lcd_blockfunc_type *bfunc;

    /* direction flip */
    if (x2 < x1)
    {
        x = x1;
        x1 = x2;
        x2 = x;
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
    dst   = &lcd_framebuffer[y>>2][x1];
    mask  = pixmask[y & 3];

    dst_end = dst + x2 - x1;
    do
        bfunc(dst++, mask, 0xFFu);
    while (dst <= dst_end);
}

/* Draw a vertical line (optimised) */
void lcd_vline(int x, int y1, int y2)
{
    int ny;
    unsigned char *dst;
    unsigned mask, mask_bottom;
    lcd_blockfunc_type *bfunc;

    /* direction flip */
    if (y2 < y1)
    {
        ny = y1;
        y1 = y2;
        y2 = ny;
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
    dst   = &lcd_framebuffer[y1>>2][x];
    ny    = y2 - (y1 & ~3);
    mask  = 0xFFu << (2 * (y1 & 3));
    mask_bottom = 0xFFu >> (2 * (~ny & 3));

    for (; ny >= 4; ny -= 4)
    {
        bfunc(dst, mask, 0xFFu);
        dst += LCD_WIDTH;
        mask = 0xFFu;
    }
    mask &= mask_bottom;
    bfunc(dst, mask, 0xFFu);
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
    int ny;
    unsigned char *dst, *dst_end;
    unsigned mask, mask_bottom;
    unsigned bits = 0;
    lcd_blockfunc_type *bfunc;
    bool fillopt = false;

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
    
    if (drawmode & DRMODE_INVERSEVID)
    {
        if (drawmode & DRMODE_BG)
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
    bfunc = lcd_blockfuncs[drawmode];
    dst   = &lcd_framebuffer[y>>2][x];
    ny    = height - 1 + (y & 3);
    mask  = 0xFFu << (2 * (y & 3));
    mask_bottom = 0xFFu >> (2 * (~ny & 3));

    for (; ny >= 4; ny -= 4)
    {
        if (fillopt && (mask == 0xFFu))
            memset(dst, bits, width);
        else
        {
            unsigned char *dst_row = dst;

            dst_end = dst_row + width;
            do
                bfunc(dst_row++, mask, 0xFFu);
            while (dst_row < dst_end);
        }

        dst += LCD_WIDTH;
        mask = 0xFFu;
    }
    mask &= mask_bottom;

    if (fillopt && (mask == 0xFFu))
        memset(dst, bits, width);
    else
    {
        dst_end = dst + width;
        do
            bfunc(dst++, mask, 0xFFu);
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
void lcd_mono_bitmap_part(const unsigned char *src, int src_x, int src_y,
                          int stride, int x, int y, int width, int height)
                          ICODE_ATTR;
void lcd_mono_bitmap_part(const unsigned char *src, int src_x, int src_y,
                          int stride, int x, int y, int width, int height)
{
    int shift, ny;
    unsigned char *dst, *dst_end;
    unsigned mask, mask_bottom;
    lcd_blockfunc_type *bfunc;

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

    src    += stride * (src_y >> 3) + src_x; /* move starting point */
    src_y  &= 7;
    y      -= src_y;
    dst    = &lcd_framebuffer[y>>2][x];
    shift  = y & 3;
    ny     = height - 1 + shift + src_y;

    bfunc  = lcd_blockfuncs[drawmode];
    mask   = 0xFFu << (shift + src_y);
    mask_bottom = 0xFFu >> (~ny & 7);
    
    if (shift == 0)
    {
        unsigned dmask1, dmask2, data;

        for (; ny >= 8; ny -= 8)
        {
            const unsigned char *src_row = src;
            unsigned char *dst_row = dst + LCD_WIDTH;
            
            dmask1 = dibits[mask&0x0F];
            dmask2 = dibits[(mask>>4)&0x0F];
            dst_end = dst_row + width;

            if (dmask1 != 0)
            {
                do
                {
                    data = *src_row++;
                    bfunc(dst_row - LCD_WIDTH, dmask1, dibits[data&0x0F]);
                    bfunc(dst_row++, dmask2, dibits[(data>>4)&0x0F]);
                }
                while (dst_row < dst_end);
            }
            else
            {
                do
                    bfunc(dst_row++, dmask2, dibits[((*src_row++)>>4)&0x0F]);
                while (dst_row < dst_end);
            }
            src += stride;
            dst += 2*LCD_WIDTH;
            mask = 0xFFu;
        }
        mask &= mask_bottom;
        dmask1 = dibits[mask&0x0F];
        dmask2 = dibits[(mask>>4)&0x0F];
        dst_end = dst + width;
        
        if (dmask1 != 0)
        {
            if (dmask2 != 0)
            {
                do
                {
                    data = *src++;
                    bfunc(dst, dmask1, dibits[data&0x0F]);
                    bfunc((dst++) + LCD_WIDTH, dmask2, dibits[(data>>4)&0x0F]);
                }
                while (dst < dst_end);
            }
            else
            {
                do
                    bfunc(dst++, dmask1, dibits[(*src++)&0x0F]);
                while (dst < dst_end);
            }
        }
        else
        {
            do
                bfunc((dst++) + LCD_WIDTH, dmask2, dibits[((*src++)>>4)&0x0F]);
            while (dst < dst_end);
        }
    }
    else
    {
        dst_end = dst + width;
        do
        {
            const unsigned char *src_col = src++;
            unsigned char *dst_col = dst++;
            unsigned mask_col = mask;
            unsigned data = 0;
            
            for (y = ny; y >= 8; y -= 8)                    
            {
                data |= *src_col << shift;

                if (mask_col & 0xFFu)
                {
                    if (mask_col & 0x0F)
                        bfunc(dst_col, dibits[mask_col&0x0F], dibits[data&0x0F]);
                    bfunc(dst_col + LCD_WIDTH, dibits[(mask_col>>4)&0x0F],
                          dibits[(data>>4)&0x0F]);
                    mask_col = 0xFFu;
                }
                else
                    mask_col >>= 8;

                src_col += stride;
                dst_col += 2*LCD_WIDTH;
                data >>= 8;
            }
            data |= *src_col << shift;
            mask_bottom &= mask_col;
            if (mask_bottom & 0x0F)
                bfunc(dst_col, dibits[mask_bottom&0x0F], dibits[data&0x0F]);
            if (mask_bottom & 0xF0)
                bfunc(dst_col + LCD_WIDTH, dibits[(mask_bottom&0xF0)>>4],
                      dibits[(data>>4)&0x0F]);
        }
        while (dst < dst_end);
    }
}

/* Draw a full monochrome bitmap */
void lcd_mono_bitmap(const unsigned char *src, int x, int y, int width, int height)
{
    lcd_mono_bitmap_part(src, 0, 0, width, x, y, width, height);
}

/* About Rockbox' internal native bitmap format:
 *
 * A bitmap contains two bits for every pixel. 00 = white, 01 = light grey,
 * 10 = dark grey, 11 = black. Bits within a byte are arranged vertically, LSB
 * at top.
 * The bytes are stored in row-major order, with byte 0 being top left,
 * byte 1 2nd from left etc. The first row of bytes defines pixel rows
 * 0..3, the second row defines pixel row 4..7 etc.
 *
 * This is the same as the internal lcd hw format. */

/* Draw a partial native bitmap */
void lcd_bitmap_part(const unsigned char *src, int src_x, int src_y,
                     int stride, int x, int y, int width, int height)
                     ICODE_ATTR;
void lcd_bitmap_part(const unsigned char *src, int src_x, int src_y,
                     int stride, int x, int y, int width, int height)
{
    int shift, ny;
    unsigned char *dst, *dst_end;
    unsigned mask, mask_bottom;

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

    src    += stride * (src_y >> 2) + src_x; /* move starting point */
    src_y  &= 3;
    y      -= src_y;
    dst    = &lcd_framebuffer[y>>2][x];
    shift  = y & 3;
    ny     = height - 1 + shift + src_y;

    mask   = 0xFFu << (2 * (shift + src_y));
    mask_bottom = 0xFFu >> (2 * (~ny & 3));
    
    if (shift == 0)
    {
        for (; ny >= 4; ny -= 4)
        {
            if (mask == 0xFFu)
                memcpy(dst, src, width);
            else
            {
                const unsigned char *src_row = src;
                unsigned char *dst_row = dst;
                
                dst_end = dst_row + width;
                do 
                    setblock(dst_row++, mask, *src_row++);
                while (dst_row < dst_end);
            }

            src += stride;
            dst += LCD_WIDTH;
            mask = 0xFFu;
        }
        mask &= mask_bottom;

        if (mask == 0xFFu)
            memcpy(dst, src, width);
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
        shift *= 2;
        dst_end = dst + width;
        do
        {
            const unsigned char *src_col = src++;
            unsigned char *dst_col = dst++;
            unsigned mask_col = mask;
            unsigned data = 0;
            
            for (y = ny; y >= 4; y -= 4)
            {
                data |= *src_col << shift;

                if (mask_col & 0xFFu)
                {
                    setblock(dst_col, mask_col, data);
                    mask_col = 0xFFu;
                }
                else
                    mask_col >>= 8;

                src_col += stride;
                dst_col += LCD_WIDTH;
                data >>= 8;
            }
            data |= *src_col << shift;
            setblock(dst_col, mask_col & mask_bottom, data);
        }
        while (dst < dst_end);
    }
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

        /* check input range */
        if (ch < pf->firstchar || ch >= pf->firstchar+pf->size)
            ch = pf->defaultchar;
        ch -= pf->firstchar;

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
            drawmode = s->invert ?
                       (DRMODE_SOLID|DRMODE_INVERSEVID) : DRMODE_SOLID;
            lcd_putsxyofs(xpos, ypos, s->offset, s->line);
            drawmode = lastmode;
            lcd_update_rect(xpos, ypos, LCD_WIDTH - xpos, pf->height);
        }

        sleep(scroll_ticks);
    }
}

