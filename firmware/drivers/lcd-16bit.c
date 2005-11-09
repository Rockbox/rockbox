/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Dave Chapman
 *
 * Rockbox driver for 16-bit colour LCDs
 *
 * iPod driver based on code from ipodlinux - http://ipodlinux.org
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
#include "bidi.h"

/*** definitions ***/

#define IPOD_HW_REVISION (*((volatile unsigned long*)(0x00002084)))

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

#define LCD_CNTL_PAGE                   0xb1
#define LCD_CNTL_COLUMN                 0x13
#define LCD_CNTL_DATA_WRITE             0x1d

#define SCROLLABLE_LINES 26

#define RGB_PACK(r,g,b) (swap16(((r>>3)<<11)|((g>>2)<<5)|(b>>3)))

/*** globals ***/
unsigned char lcd_framebuffer[LCD_HEIGHT][LCD_WIDTH*2] __attribute__ ((aligned (2)));

static unsigned fg_pattern = 0x0000; /* Black */
static unsigned bg_pattern = 0xbc2d; /* "Rockbox blue" */
static int drawmode = DRMODE_SOLID;
static int xmargin = 0;
static int ymargin = 0;
static int curfont = FONT_SYSFIXED;
static int lcd_type = 1; /* 0 = "old" Color/Photo, 1 = "new" Color & Nano */

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

#ifdef IRIVER_H300_SERIES
static void lcd_cmd_data(int cmd, int data)
{
}

static void lcd_send_lo(int v)
{
}

static void lcd_send_hi(int v)
{
}

#define outl(a, v)
#define inl(x) 0

#else
#define IPOD_PP5020_RTC         0x60005010

#define LCD_DATA 0x10
#define LCD_CMD  0x08

#define IPOD_LCD_BASE      0x70008a0c
#define IPOD_LCD_BUSY_MASK 0x80000000

static int timer_get_current(void)
{
        return inl(IPOD_PP5020_RTC);
}

/* check if number of useconds has past */
static int timer_check(int clock_start, int usecs)
{
        unsigned long clock;
        clock = inl(IPOD_PP5020_RTC);

        if ( (clock - clock_start) >= usecs ) {
                return 1;
        } else {
                return 0;
        }
}

static void lcd_wait_write(void)
{
        if ((inl(IPOD_LCD_BASE) & IPOD_LCD_BUSY_MASK) != 0) {
                int start = timer_get_current();

                do {
                        if ((inl(IPOD_LCD_BASE) & IPOD_LCD_BUSY_MASK) == 0) break;
                } while (timer_check(start, 1000) == 0);
        }
}

static void lcd_send_lo(int v)
{
        lcd_wait_write();
        outl(v | 0x80000000, 0x70008A0C);
}

static void lcd_send_hi(int v)
{
        lcd_wait_write();
        outl(v | 0x81000000, 0x70008A0C);
}

static void lcd_cmd_data(int cmd, int data)
{
    if (lcd_type == 0) {
        lcd_send_lo(cmd);
        lcd_send_lo(data);
    } else {
        lcd_send_lo(0x0);
        lcd_send_lo(cmd);
        lcd_send_hi((data >> 8) & 0xff);
        lcd_send_hi(data & 0xff);
    }
}
#endif

/*** hardware configuration ***/

int lcd_default_contrast(void)
{
    return 28;
}

#ifndef SIMULATOR

void lcd_set_contrast(int val)
{
  #warning: Implement lcd_set_contrast()
}

void lcd_set_invert_display(bool yesno)
{
  #warning: Implement lcd_set_invert_display()
}

/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
#warning: Implement lcd_set_flip()
}

/* Rolls up the lcd display by the specified amount of lines.
 * Lines that are rolled out over the top of the screen are
 * rolled in from the bottom again. This is a hardware 
 * remapping only and all operations on the lcd are affected.
 * -> 
 * @param int lines - The number of lines that are rolled. 
 *  The value must be 0 <= pixels < LCD_HEIGHT. */
void lcd_roll(int lines)
{
    lines &= LCD_HEIGHT-1;
#warning: To do: Implement lcd_roll()
}

#endif /* !SIMULATOR */

/* LCD init */
void lcd_init(void)
{  
    lcd_clear_display();
    
#ifndef SIMULATOR
#if CONFIG_LCD == LCD_IPODCOLOR
    if (IPOD_HW_REVISION == 0x60000) {
        lcd_type = 0;
    } else {
        int gpio_a01, gpio_a04;

        /* A01 */
        gpio_a01 = (inl(0x6000D030) & 0x2) >> 1;
        /* A04 */
        gpio_a04 = (inl(0x6000D030) & 0x10) >> 4;

        if (((gpio_a01 << 1) | gpio_a04) == 0 || ((gpio_a01 << 1) | gpio_a04) == 2) {
            lcd_type = 0;
        } else {
            lcd_type = 1;
        }
    }

    outl(inl(0x6000d004) | 0x4, 0x6000d004); /* B02 enable */
    outl(inl(0x6000d004) | 0x8, 0x6000d004); /* B03 enable */
    outl(inl(0x70000084) | 0x2000000, 0x70000084); /* D01 enable */
    outl(inl(0x70000080) | 0x2000000, 0x70000080); /* D01 =1 */

    outl(inl(0x6000600c) | 0x20000, 0x6000600c);    /* PWM enable */

    if (lcd_type == 0) {
       lcd_cmd_data(0xef, 0x0);
       lcd_cmd_data(0x1, 0x0);
       lcd_cmd_data(0x80, 0x1);
       lcd_cmd_data(0x10, 0x8);
       lcd_cmd_data(0x18, 0x6);
       lcd_cmd_data(0x7e, 0x4);
       lcd_cmd_data(0x7e, 0x5);
       lcd_cmd_data(0x7f, 0x1);
    }
#elif CONFIG_LCD == LCD_IPODNANO
    /* iPodLinux doesn't appear have any LCD init code for the Nano */
#endif

#endif

    create_thread(scroll_thread, scroll_stack,
                  sizeof(scroll_stack), scroll_name);
}


#ifndef SIMULATOR
/*** update functions ***/

/* Performance function that works with an external buffer
   note that by and bheight are in 4-pixel units! */
void lcd_blit(const unsigned char* data, int x, int by, int width,
              int bheight, int stride)
{
    #warning Implement lcd_blit()
}

/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
    int rect1, rect2, rect3, rect4;

    unsigned short *addr = (unsigned short *)lcd_framebuffer;

    /* calculate the drawing region */
    #if CONFIG_LCD == LCD_IPODCOLOR
    rect1 = x;                          /* start vert */
    rect2 = (LCD_WIDTH - 1) - y;        /* start horiz */
    rect3 = (x + height) - 1;           /* end vert */
    rect4 = (rect2 - width) + 1;        /* end horiz */
    #else
    rect1 = y;                         /* start horiz */
    rect2 = x;                         /* start vert */
    rect3 = (y + width) - 1;           /* max horiz */
    rect4 = (x + height) - 1;          /* max vert */
    #endif
    /* setup the drawing region */
    if (lcd_type == 0) {
        lcd_cmd_data(0x12, rect1);      /* start vert */
        lcd_cmd_data(0x13, rect2);      /* start horiz */
        lcd_cmd_data(0x15, rect3);      /* end vert */
        lcd_cmd_data(0x16, rect4);      /* end horiz */
    } else {
        /* swap max horiz < start horiz */
        if (rect3 < rect1) {
            int t;
            t = rect1;
            rect1 = rect3;
            rect3 = t;
        }

        /* swap max vert < start vert */
        if (rect4 < rect2) {
            int t;
            t = rect2;
            rect2 = rect4;
            rect4 = t;
        }

        /* max horiz << 8 | start horiz */
        lcd_cmd_data(0x44, (rect3 << 8) | rect1);
        /* max vert << 8 | start vert */
        lcd_cmd_data(0x45, (rect4 << 8) | rect2);

        /* start vert = max vert */
        #if CONFIG_LCD == LCD_IPODCOLOR
        rect2 = rect4;
        #endif

        /* position cursor (set AD0-AD15) */
        /* start vert << 8 | start horiz */
        lcd_cmd_data(0x21, (rect2 << 8) | rect1);

        /* start drawing */
        lcd_send_lo(0x0);
        lcd_send_lo(0x22);
    }

    addr += x * LCD_WIDTH + y;

    while (height > 0) {
        int c, r;
        int h, pixels_to_write;

        pixels_to_write = (width * height) * 2;

        /* calculate how much we can do in one go */
        h = height;
        if (pixels_to_write > 64000) {
            h = (64000/2) / width;
            pixels_to_write = (width * h) * 2;
        }

        outl(0x10000080, 0x70008a20);
        outl((pixels_to_write - 1) | 0xc0010000, 0x70008a24);
        outl(0x34000000, 0x70008a20);

        /* for each row */
        for (r = 0; r < h; r++) {
            /* for each column */
            for (c = 0; c < width; c += 2) {
                unsigned two_pixels;

                two_pixels = addr[0] | (addr[1] << 16);
                addr += 2;

                while ((inl(0x70008a20) & 0x1000000) == 0);

                /* output 2 pixels */
                outl(two_pixels, 0x70008b00);
            }

            addr += LCD_WIDTH - width;
        }

        while ((inl(0x70008a20) & 0x4000000) == 0);

        outl(0x0, 0x70008a24);

        height = height - h;
    }
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
   lcd_update_rect(0,0,LCD_WIDTH,LCD_HEIGHT);
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

void lcd_set_foreground(struct rgb color)
{
    fg_pattern=RGB_PACK(color.red,color.green,color.blue);
}

struct rgb lcd_get_foreground(void)
{
    struct rgb colour;

    colour.red=((fg_pattern&0xf800)>>11)<<3;
    colour.green=((fg_pattern&0x03e0)>>5)<<2;
    colour.blue=(fg_pattern&0x1f)<<3;

    return colour;
}

void lcd_set_background(struct rgb color)
{
    bg_pattern=RGB_PACK(color.red,color.green,color.blue);
}


struct rgb lcd_get_background(void)
{
    struct rgb colour;

    colour.red=((bg_pattern&0xf800)>>11)<<3;
    colour.green=((bg_pattern&0x03e0)>>5)<<2;
    colour.blue=(bg_pattern&0x1f)<<3;
    return colour;
}

void lcd_set_drawinfo(int mode, struct rgb fg_color,
                                struct rgb bg_color)
{
    lcd_set_drawmode(mode);
    lcd_set_foreground(fg_color);
    lcd_set_background(bg_color);
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
    unsigned short *data = &lcd_framebuffer[y][x*2];
    *data = fg_pattern;
}

static void clearpixel(int x, int y)
{
    unsigned short *data = &lcd_framebuffer[y][x*2];
    *data = bg_pattern;
}

static void flippixel(int x, int y)
{
  /* What should this do on a color display? */
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
static void flipblock(unsigned short *address, unsigned mask, unsigned bits)
                      ICODE_ATTR;
static void flipblock(unsigned short *address, unsigned mask, unsigned bits)
{
    *address ^= bits & mask;
}

static void bgblock(unsigned short *address, unsigned mask, unsigned bits)
                    ICODE_ATTR;
static void bgblock(unsigned short *address, unsigned mask, unsigned bits)
{
    if (mask > 0) {
      *address = bg_pattern;
    } else {
      *address = fg_pattern;
    }
}

static void fgblock(unsigned short *address, unsigned mask, unsigned bits)
                    ICODE_ATTR;
static void fgblock(unsigned short *address, unsigned mask, unsigned bits)
{
    if (mask > 0) {
      *address = fg_pattern;
    } else {
      *address = bg_pattern;
    }
}

static void solidblock(unsigned short *address, unsigned mask, unsigned bits)
                       ICODE_ATTR;
static void solidblock(unsigned short *address, unsigned mask, unsigned bits)
{
    *address = (*address & ~mask) | (bits & mask & fg_pattern)
                                  | (~bits & mask & bg_pattern);
}

static void flipinvblock(unsigned short *address, unsigned mask, unsigned bits)
                         ICODE_ATTR;
static void flipinvblock(unsigned short *address, unsigned mask, unsigned bits)
{
    *address ^= ~bits & mask;
}

static void bginvblock(unsigned short *address, unsigned mask, unsigned bits)
                       ICODE_ATTR;
static void bginvblock(unsigned short *address, unsigned mask, unsigned bits)
{
    mask &= bits;
    *address = (*address & ~mask) | (bg_pattern & mask);
}

static void fginvblock(unsigned short *address, unsigned mask, unsigned bits)
                       ICODE_ATTR;
static void fginvblock(unsigned short *address, unsigned mask, unsigned bits)
{
    mask &= ~bits;
    *address = (*address & ~mask) | (fg_pattern & mask);
}

static void solidinvblock(unsigned short *address, unsigned mask, unsigned bits)
                          ICODE_ATTR;
static void solidinvblock(unsigned short *address, unsigned mask, unsigned bits)
{
    *address = (*address & ~mask) | (~bits & mask & fg_pattern)
                                  | (bits & mask & bg_pattern);
}

lcd_blockfunc_type* const lcd_blockfuncs[8] = {
    flipblock, bgblock, fgblock, solidblock,
    flipinvblock, bginvblock, fginvblock, solidinvblock
};

/*** drawing functions ***/

/* Clear the whole display */
void lcd_clear_display(void)
{
    int i;
    unsigned short bits = (drawmode & DRMODE_INVERSEVID) ? fg_pattern : bg_pattern;
    unsigned short* addr = (unsigned short*)lcd_framebuffer;

    for (i=0;i<LCD_HEIGHT*LCD_WIDTH;i++) {
        *(addr++)=bits;
    }
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
    mask  = 3 << (2 * (y & 3));

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
    unsigned bits = fg_pattern;
    lcd_blockfunc_type *bfunc;
    bool fillopt;

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
    
    fillopt = (drawmode & DRMODE_INVERSEVID) ? 
              (drawmode & DRMODE_BG) : (drawmode & DRMODE_FG);
    if (fillopt &&(drawmode & DRMODE_INVERSEVID))
        bits = bg_pattern;
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

static unsigned char masks[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};

/* Draw a partial monochrome bitmap */
void lcd_mono_bitmap_part(const unsigned char *src, int src_x, int src_y,
                          int stride, int x, int y, int width, int height)
                          ICODE_ATTR;
void lcd_mono_bitmap_part(const unsigned char *src, int src_x, int src_y,
                          int stride, int x, int y, int width, int height)
{
    int in_x,in_y;
    int out_x;
    int out_y;
    unsigned char pixel;
    int src_width=src_x+width+stride;
    unsigned short* addr;

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

//    src += stride * (src_y >> 3) + src_x; /* move starting point */

    /* Copying from src_x,src_y to (x,y).  Size is width,height */
    in_x=src_x;
    for (out_x=x;out_x<(x+width);out_x++) {
      in_y=src_y;
      for (out_y=y;out_y<(y+height);out_y++) {
         pixel=(*src)&masks[in_y];
         addr=(unsigned short*)&lcd_framebuffer[out_y][out_x*2];
         if (pixel > 0) {
            *addr=fg_pattern;
         } else {
            *addr=bg_pattern;
         }
         in_y++;
      }
      in_x++;
      src++;
    }
}

/* Draw a full monochrome bitmap */
void lcd_mono_bitmap(const unsigned char *src, int x, int y, int width, int height)
{
    lcd_mono_bitmap_part(src, 0, 0, width, x, y, width, height);
}

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

    src    += stride * (src_y >> 2) + src_x; /* move starting point */
    src_y  &= 3;
    y      -= src_y;
    dst    = &lcd_framebuffer[y>>2][x];
    shift  = y & 3;
    ny     = height - 1 + shift + src_y;

    bfunc  = lcd_blockfuncs[drawmode];
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
                    bfunc(dst_row++, mask, *src_row++);
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
                bfunc(dst++, mask, *src++);
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
                    bfunc(dst_col, mask_col, data);
                    mask_col = 0xFFu;
                }
                else
                    mask_col >>= 8;

                src_col += stride;
                dst_col += LCD_WIDTH;
                data >>= 8;
            }
            data |= *src_col << shift;
            bfunc(dst_col, mask_col & mask_bottom, data);
        }
        while (dst < dst_end);
    }
}

/* Draw a full native bitmap */
void lcd_bitmap(const unsigned char *src, int x, int y, int width, int height)
{
    unsigned short* s=src;
    unsigned short* d=(unsigned short*)&lcd_framebuffer[y][x*2];
    int k=LCD_WIDTH-width;
    int i,j;

    for (i=0;i<height;i++) {
        for (j=0;j<width;j++) {
            *(d++)=*(s++);
        }
        d+=k;
    }

//OLD Implementation: lcd_bitmap_part(src, 0, 0, width, x, y, width, height);

}

/* put a string at a given pixel position, skipping first ofs pixel columns */
static void lcd_putsxyofs(int x, int y, int ofs, const unsigned char *str)
{
    int ch;
    struct font* pf = font_get(curfont);

    if (bidi_support_enabled)
        str = bidi_l2v(str, 1);

    while ((ch = *str++) != '\0' && x < LCD_WIDTH)
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

void lcd_puts_style(int x, int y, const unsigned char *str, int style)
{
    int xpos,ypos,w,h;
    int lastmode = drawmode;

    /* make sure scrolling is turned off on the line we are updating */
    scrolling_lines &= ~(1 << y);

    if(!str || !str[0])
        return;

    lcd_getstringsize(str, &w, &h);
    xpos = xmargin + x*w / strlen(str);
    ypos = ymargin + y*h;
    lcd_putsxy(xpos, ypos, str);
#if 0
    drawmode = (DRMODE_SOLID|DRMODE_INVERSEVID);
    lcd_fillrect(xpos + w, ypos, LCD_WIDTH - (xpos + w), h);
    if (style & STYLE_INVERT)
    {
        drawmode = DRMODE_COMPLEMENT;
        lcd_fillrect(xpos, ypos, LCD_WIDTH - xpos, h);
    }
    drawmode = lastmode;
#endif
}

/* put a string at a given char position */
void lcd_puts(int x, int y, const unsigned char *str)
{
    lcd_puts_style(x, y, str, STYLE_DEFAULT);
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
    struct scrollinfo* s;
    int w, h;

    s = &scroll[y];

    s->start_tick = current_tick + scroll_delay;
    s->invert = false;
    if (style & STYLE_INVERT) {
        s->invert = true;
        lcd_puts_style(x,y,string,STYLE_INVERT);
    }
    else
        lcd_puts(x,y,string);

    lcd_getstringsize(string, &w, &h);

    if (LCD_WIDTH - x * 8 - xmargin < w) {
        /* prepare scroll line */
        char *end;

        memset(s->line, 0, sizeof s->line);
        strcpy(s->line, string);

        /* get width */
        s->width = lcd_getstringsize(s->line, &w, &h);

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
            s->width = lcd_getstringsize(s->line, &w, &h);
        }

        end = strchr(s->line, '\0');
        strncpy(end, string, LCD_WIDTH/2);

        s->len = strlen(string);
        s->offset = 0;
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
            lcd_putsxyofs(xpos, ypos, s->offset, s->line);
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

