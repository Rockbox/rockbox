/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"

#ifdef HAVE_LCD_BITMAP

#include "lcd.h"
#include "kernel.h"
#include "thread.h"
#include <string.h>
#include <stdlib.h>
#include "file.h"
#include "debug.h"
#include "system.h"
#include "font.h"

/*** definitions ***/

#define LCD_SET_LOWER_COLUMN_ADDRESS              ((char)0x00)
#define LCD_SET_HIGHER_COLUMN_ADDRESS             ((char)0x10)
#define LCD_SET_INTERNAL_REGULATOR_RESISTOR_RATIO ((char)0x20)
#define LCD_SET_POWER_CONTROL_REGISTER            ((char)0x28)
#define LCD_SET_DISPLAY_START_LINE                ((char)0x40)
#define LCD_SET_CONTRAST_CONTROL_REGISTER         ((char)0x81)
#define LCD_SET_SEGMENT_REMAP                     ((char)0xA0)
#define LCD_SET_LCD_BIAS                          ((char)0xA2)
#define LCD_SET_ENTIRE_DISPLAY_OFF                ((char)0xA4)
#define LCD_SET_ENTIRE_DISPLAY_ON                 ((char)0xA5)
#define LCD_SET_NORMAL_DISPLAY                    ((char)0xA6)
#define LCD_SET_REVERSE_DISPLAY                   ((char)0xA7)
#define LCD_SET_INDICATOR_OFF                     ((char)0xAC)
#define LCD_SET_INDICATOR_ON                      ((char)0xAD)
#define LCD_SET_DISPLAY_OFF                       ((char)0xAE)
#define LCD_SET_DISPLAY_ON                        ((char)0xAF)
#define LCD_SET_PAGE_ADDRESS                      ((char)0xB0)
#define LCD_SET_COM_OUTPUT_SCAN_DIRECTION         ((char)0xC0)
#define LCD_SET_DISPLAY_OFFSET                    ((char)0xD3)
#define LCD_SET_READ_MODIFY_WRITE_MODE            ((char)0xE0)
#define LCD_SOFTWARE_RESET                        ((char)0xE2)
#define LCD_NOP                                   ((char)0xE3)
#define LCD_SET_END_OF_READ_MODIFY_WRITE_MODE     ((char)0xEE)

/* LCD command codes */
#define LCD_CNTL_RESET          0xe2    // Software reset
#define LCD_CNTL_POWER          0x2f    // Power control
#define LCD_CNTL_CONTRAST       0x81    // Contrast
#define LCD_CNTL_OUTSCAN        0xc8    // Output scan direction
#define LCD_CNTL_SEGREMAP       0xa1    // Segment remap
#define LCD_CNTL_DISPON         0xaf    // Display on

#define LCD_CNTL_PAGE           0xb0    // Page address
#define LCD_CNTL_HIGHCOL        0x10    // Upper column address
#define LCD_CNTL_LOWCOL         0x00    // Lower column address

#define SCROLL_SPACING 3

struct scrollinfo {
    char line[MAX_PATH + LCD_WIDTH/2 + SCROLL_SPACING + 2];
    int len;    /* length of line in chars */
    int width;  /* length of line in pixels */
    int offset;
    int startx;
    int starty;
};

static void scroll_thread(void);
static char scroll_stack[DEFAULT_STACK_SIZE];
static char scroll_name[] = "scroll";
static char scroll_speed = 8; /* updates per second */
static char scroll_step = 6;  /* pixels per scroll step */
static struct scrollinfo scroll; /* only one scroll line at the moment */
static int scroll_count = 0;
static int xmargin = 0;
static int ymargin = 0;
static int curfont = FONT_SYSFIXED;

#ifdef SIMULATOR
unsigned char lcd_framebuffer[LCD_WIDTH][LCD_HEIGHT/8];
#else
static unsigned char lcd_framebuffer[LCD_WIDTH][LCD_HEIGHT/8];
#endif

/* All zeros and ones bitmaps for area filling */
static unsigned char zeros[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static unsigned char ones[8]  = { 0xff, 0xff, 0xff, 0xff,
                                  0xff, 0xff, 0xff, 0xff};

void lcd_init(void)
{
    create_thread(scroll_thread, scroll_stack,
                  sizeof(scroll_stack), scroll_name);
}
void lcd_clear_display (void)
{
    memset (lcd_framebuffer, 0, sizeof lcd_framebuffer);
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

int lcd_getstringsize(unsigned char *str, int *w, int *h)
{
    struct font* pf = font_get(curfont);
    int ch;
    int width = 0;

    while((ch = *str++)) {
        /* check input range*/
        if (ch < pf->firstchar || ch >= pf->firstchar+pf->size)
            ch = pf->defaultchar;
        ch -= pf->firstchar;

        /* get proportional width and glyph bits*/
        width += pf->width? pf->width[ch]: pf->maxwidth;
    }
    if ( w )
        *w = width;
    if ( h )
        *h = pf->height;

    return width;
}

/* put a string at a given char position */
void lcd_puts(int x, int y, unsigned char *str)
{
    int xpos,ypos,w,h;

#if defined(HAVE_LCD_CHARCELLS)
    /* We make the simulator truncate the string if it reaches the right edge,
       as otherwise it'll wrap. The real target doesn't wrap. */

    char buffer[12];
    if(strlen(str)+x > 11 ) {
        strncpy(buffer, str, sizeof buffer);
        buffer[11-x]=0;
        str = buffer;
    }
    xmargin = 0;
    ymargin = 8;
#endif

    if(!str || !str[0])
        return;

    lcd_getstringsize(str, &w, &h);
    xpos = xmargin + x*w / strlen(str);
    ypos = ymargin + y*h;
    lcd_putsxy(xpos, ypos, str);
    /* lcd_clearrect(xpos + w, ypos, LCD_WIDTH - (xpos + w), h); */

#if defined(HAVE_LCD_CHARCELLS)
    /* this function is being used when simulating a charcell LCD and
       then we update immediately */
    lcd_update();
#endif
}

extern char* get_lcd_pattern(int width);

/* put a string at a given pixel position, skipping first ofs pixel columns */
static void lcd_putsxyofs(int x, int y, int ofs, unsigned char *str)
{
    int ch;
    struct font* pf = font_get(curfont);

    while ((ch = *str++) != '\0' && x < LCD_WIDTH)
    {
        int width;

        /* check input range */
        if (ch < pf->firstchar || ch >= pf->firstchar+pf->size)
#ifndef SIMULATOR
            ch = pf->defaultchar;
#else
        {
            if (ch > 8)
                ch = pf->defaultchar;
            else
            {
                char* bm = get_lcd_pattern(ch);
                width = 6;
                lcd_bitmap(bm, x, y, width, 8, true);
                x += width;
                continue;
            }
        }
#endif
        ch -= pf->firstchar;

        /* no partial-height drawing for now... */
        if (y + pf->height > LCD_HEIGHT)
            break;

        /* get proportional width and glyph bits */
        width = pf->width ? pf->width[ch] : pf->maxwidth;
        width = MIN (width, LCD_WIDTH - x);

        if (ofs != 0)
        {
            if (ofs > width)
            {
                ofs -= width;
                continue;
            }
            width -= ofs;
        }

        if (width > 0)
        {
            int rows = (pf->height + 7) / 8;
            bitmap_t* bits = pf->bits + 
                (pf->offset ? pf->offset[ch] : (pf->height * ch));
            lcd_bitmap (((unsigned char*) bits) + ofs*rows, x, y,
                        width, pf->height, true);
            x += width;
        }
        ofs = 0;
    }
}

/* put a string at a given pixel position */
void lcd_putsxy(int x, int y, unsigned char *str)
{
    lcd_putsxyofs(x, y, 0, str);
}

/*
 * All bitmaps have this format:
 * Bits within a byte are arranged veritcally, LSB at top.
 * Bytes are stored in column-major format, with byte 0 at top left,
 * byte 1 is 2nd from top, etc.  Bytes following left-most column
 * starts 2nd left column, etc.
 *
 * Note: The HW takes bitmap bytes in row-major order.
 *
 * Memory copy of display bitmap
 */

/*
 * Draw a bitmap at (x, y), size (nx, ny)
 * if 'clear' is true, clear destination area first
 */
void lcd_bitmap (unsigned char *src, int x, int y, int nx, int ny,
                 bool clear) __attribute__ ((section (".icode")));
void lcd_bitmap (unsigned char *src, int x, int y, int nx, int ny,
                 bool clear)
{
    unsigned char *dst;
    unsigned char *dst2;
    unsigned int data, mask, mask2, mask3, mask4;
    int shift;

    if (((unsigned)x >= LCD_WIDTH) || ((unsigned)y >= LCD_HEIGHT))
        return;
    if (((unsigned)(x + nx)) >= LCD_WIDTH)
        nx = LCD_WIDTH - x;
    if (((unsigned)(y + ny)) >= LCD_HEIGHT)
        ny = LCD_HEIGHT - y;      

    shift = y & 7;
    dst2 = &lcd_framebuffer[x][y/8];
    ny += shift;

    /* Calculate bit masks */
    mask4 = ~(0xfe << ((ny-1) & 7));
    if (clear)
    {
        mask = ~(0xff << shift);
        mask2 = 0;
        mask3 = ~mask4;
        if (ny <= 8)
            mask3 |= mask;
    }
    else
        mask = mask2 = mask3 = 0xff;

    /* Loop for each column */
    for (x = 0; x < nx; x++)
    {
        dst = dst2;
        dst2 += LCD_HEIGHT/8;
        data = 0;
        y = 0;

        if (ny > 8)
        {
            /* First partial row */
            data = *src++ << shift;
            *dst = (*dst & mask) | data;
            data >>= 8;
            dst++;

            /* Intermediate rows */
            for (y = 8; y < ny-8; y += 8)
            {
                data |= *src++ << shift;
                *dst = (*dst & mask2) | data;
                data >>= 8;
                dst++;
            }
        }

        /* Last partial row */
        if (y + shift < ny)
            data |= *src++ << shift;
        *dst = (*dst & mask3) | (data & mask4);
    }
}

/* 
 * Draw a rectangle with upper left corner at (x, y)
 * and size (nx, ny)
 */
void lcd_drawrect (int x, int y, int nx, int ny)
{
    int i;

    if (x > LCD_WIDTH)
        return;
    if (y > LCD_HEIGHT)
        return;

    if (x + nx > LCD_WIDTH)
        nx = LCD_WIDTH - x;
    if (y + ny > LCD_HEIGHT)
        ny = LCD_HEIGHT - y;

    /* vertical lines */
    for (i = 0; i < ny; i++) {
        DRAW_PIXEL(x, (y + i));
        DRAW_PIXEL((x + nx - 1), (y + i));
    }

    /* horizontal lines */
    for (i = 0; i < nx; i++) {
        DRAW_PIXEL((x + i),y);
        DRAW_PIXEL((x + i),(y + ny - 1));
    }
}

/*
 * Clear a rectangular area at (x, y), size (nx, ny)
 */
void lcd_clearrect (int x, int y, int nx, int ny)
{
    int i;
    for (i = 0; i < nx; i++)
        lcd_bitmap (zeros, x+i, y, 1, ny, true);
}

/*
 * Fill a rectangular area at (x, y), size (nx, ny)
 */
void lcd_fillrect (int x, int y, int nx, int ny)
{
    int i;
    for (i = 0; i < nx; i++)
        lcd_bitmap (ones, x+i, y, 1, ny, true);
}

/* Invert a rectangular area at (x, y), size (nx, ny) */
void lcd_invertrect (int x, int y, int nx, int ny)
{
    int i, j;

    if (x > LCD_WIDTH)
        return;
    if (y > LCD_HEIGHT)
        return;

    if (x + nx > LCD_WIDTH)
        nx = LCD_WIDTH - x;
    if (y + ny > LCD_HEIGHT)
        ny = LCD_HEIGHT - y;

    for (i = 0; i < nx; i++)
        for (j = 0; j < ny; j++)
            INVERT_PIXEL((x + i), (y + j));
}

void lcd_drawline( int x1, int y1, int x2, int y2 )
{
    int numpixels;
    int i;
    int deltax, deltay;
    int d, dinc1, dinc2;
    int x, xinc1, xinc2;
    int y, yinc1, yinc2;

    deltax = abs(x2 - x1);
    deltay = abs(y2 - y1);

    if(deltax >= deltay)
    {
        numpixels = deltax;
        d = 2 * deltay - deltax;
        dinc1 = deltay * 2;
        dinc2 = (deltay - deltax) * 2;
        xinc1 = 1;
        xinc2 = 1;
        yinc1 = 0;
        yinc2 = 1;
    }
    else
    {
        numpixels = deltay;
        d = 2 * deltax - deltay;
        dinc1 = deltax * 2;
        dinc2 = (deltax - deltay) * 2;
        xinc1 = 0;
        xinc2 = 1;
        yinc1 = 1;
        yinc2 = 1;
    }
    numpixels++; /* include endpoints */

    if(x1 > x2)
    {
        xinc1 = -xinc1;
        xinc2 = -xinc2;
    }

    if(y1 > y2)
    {
        yinc1 = -yinc1;
        yinc2 = -yinc2;
    }

    x = x1;
    y = y1;

    for(i=0; i<numpixels; i++)
    {
        DRAW_PIXEL(x,y);

        if(d < 0)
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

void lcd_clearline( int x1, int y1, int x2, int y2 )
{
    int numpixels;
    int i;
    int deltax, deltay;
    int d, dinc1, dinc2;
    int x, xinc1, xinc2;
    int y, yinc1, yinc2;

    deltax = abs(x2 - x1);
    deltay = abs(y2 - y1);

    if(deltax >= deltay)
    {
        numpixels = deltax;
        d = 2 * deltay - deltax;
        dinc1 = deltay * 2;
        dinc2 = (deltay - deltax) * 2;
        xinc1 = 1;
        xinc2 = 1;
        yinc1 = 0;
        yinc2 = 1;
    }
    else
    {
        numpixels = deltay;
        d = 2 * deltax - deltay;
        dinc1 = deltax * 2;
        dinc2 = (deltax - deltay) * 2;
        xinc1 = 0;
        xinc2 = 1;
        yinc1 = 1;
        yinc2 = 1;
    }
    numpixels++; /* include endpoints */

    if(x1 > x2)
    {
        xinc1 = -xinc1;
        xinc2 = -xinc2;
    }

    if(y1 > y2)
    {
        yinc1 = -yinc1;
        yinc2 = -yinc2;
    }

    x = x1;
    y = y1;

    for(i=0; i<numpixels; i++)
    {
        CLEAR_PIXEL(x,y);

        if(d < 0)
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

/*
 * Set a single pixel
 */
void lcd_drawpixel(int x, int y)
{
    DRAW_PIXEL(x,y);
}

/*
 * Clear a single pixel
 */
void lcd_clearpixel(int x, int y)
{
    CLEAR_PIXEL(x,y);
}

/*
 * Invert a single pixel
 */
void lcd_invertpixel(int x, int y)
{
    INVERT_PIXEL(x,y);
}

void lcd_puts_scroll(int x, int y, unsigned char* string )
{
    struct scrollinfo* s = &scroll;
    int w, h;

    lcd_puts(x,y,string);
    lcd_getstringsize(string, &w, &h);

    if (LCD_WIDTH - x*8 - xmargin < w)
    {
        /* prepare scroll line */
        char *end;

        memset(s->line, 0, sizeof s->line);
        strcpy(s->line, string);
        strcat(s->line, "   ");
        /* get new width incl. spaces */
        s->width = lcd_getstringsize(s->line, &w, &h);

        for (end = s->line; *end; end++);
        strncpy(end, string, LCD_WIDTH/2);

        s->len = strlen(string);
        s->offset = 0;
        s->startx = x;
        s->starty = y;
        scroll_count = 1;
    }
}


void lcd_stop_scroll(void)
{
    if ( scroll_count ) {
        int w,h;
        struct scrollinfo* s = &scroll;
        scroll_count = 0;

        lcd_getstringsize(s->line, &w, &h);
        lcd_clearrect(xmargin + s->startx*w/s->len,
                      ymargin + s->starty*h,
                      LCD_WIDTH - xmargin,
                      h);

        /* restore scrolled row */
        lcd_puts(s->startx,s->starty,s->line);
        lcd_update();
    }
}

void lcd_scroll_pause(void)
{
    scroll_count = 0;
}

void lcd_scroll_resume(void)
{
    scroll_count = 1;
}

void lcd_scroll_speed(int speed)
{
    scroll_step = speed;
}

static void scroll_thread(void)
{
    struct scrollinfo* s = &scroll;

    while ( 1 ) {
        if ( !scroll_count ) {
            yield();
            continue;
        }
        /* wait 0.5s before starting scroll */
        if ( scroll_count < scroll_speed/2 )
            scroll_count++;
        else {
            int w, h;
            int xpos, ypos;

            s->offset += scroll_step;

            if (s->offset >= s->width)
                s->offset %= s->width;

            lcd_getstringsize(s->line, &w, &h);
            xpos = xmargin + (s->startx-1) * w / s->len;
            ypos = ymargin + s->starty * h;

            lcd_clearrect(xpos, ypos, LCD_WIDTH - xmargin, h);
            lcd_putsxyofs(xpos, ypos, s->offset, s->line);
            lcd_update();
        }
        sleep(HZ/scroll_speed);
    }
}

#endif

/* -----------------------------------------------------------------
 * local variables:
 * eval: (load-file "../rockbox-mode.el")
 * end:
 */
