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
#include "hwcompat.h"

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

#define CHAR_WIDTH 6
#define CHAR_HEIGHT 8
#define ICON_HEIGHT 8

unsigned char lcd_framebuffer[LCD_WIDTH][LCD_HEIGHT/8];

/* All zeros and ones bitmaps for area filling */
static unsigned char zeros[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static unsigned char ones[8]  = { 0xff, 0xff, 0xff, 0xff,
                                  0xff, 0xff, 0xff, 0xff};

static int double_height=1;


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

void lcd_clear_display(void)
{
    memset (lcd_framebuffer, 0, sizeof lcd_framebuffer);
}

/* put a string at a given pixel position, skipping first ofs pixel columns */
static void lcd_putsxyofs(int x, int y, int ofs, unsigned char *str)
{
    int ch;
    struct font* pf = font_get(2);

    while ((ch = *str++) != '\0' && x < LCD_WIDTH)
    {
        int width;

        /* check input range */
        if (ch < pf->firstchar || ch >= pf->firstchar+pf->size)
            ch = pf->defaultchar;
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

void lcd_puts(int x, int y, unsigned char *str)
{
    int xpos,ypos;
    
    /* We make the simulator truncate the string if it reaches the right edge,
       as otherwise it'll wrap. The real target doesn't wrap. */

    char buffer[12];
    if(strlen(str)+x > 11 ) {
        strncpy(buffer, str, sizeof buffer);
        buffer[11-x]=0;
        str = buffer;
    }

    xpos = CHAR_WIDTH * x;
    ypos = ICON_HEIGHT + CHAR_HEIGHT * y;
    lcd_clearrect(xpos, ypos, LCD_WIDTH - xpos, CHAR_HEIGHT * double_height);
    lcd_putsxy(xpos, ypos, str);

    /* this function is being used when simulating a charcell LCD and
       then we update immediately */
    lcd_update();
}

void lcd_double_height(bool on)
{
    double_height = 1;
    if (on)
        double_height = 2;
}

static char patterns[8][7];

void lcd_define_pattern(int which, char *pattern, int length)
{
    int i, j;
    int pat = which / 8;
    char icon[8];
    memset(icon, 0, sizeof icon);
    
    DEBUGF("Defining pattern %d\n", pat);
    for (j = 0; j <= 5; j++) {
        for (i = 0; i < length; i++) {
            if ((pattern[i])&(1<<(j)))
                icon[5-j] |= (1<<(i));
        }
    }
    for (i = 0; i <= 5; i++)
    {
        patterns[pat][i] = icon[i];
    }
}

char* get_lcd_pattern(int which)
{
    DEBUGF("Get pattern %d\n", which);
    return patterns[which];
}

extern void lcd_puts(int x, int y, unsigned char *str);

void lcd_putc(int x, int y, unsigned char ch)
{
    char str[2];
    int xpos = x * CHAR_WIDTH;
    int ypos = ICON_HEIGHT + y * CHAR_HEIGHT;
    if (ch <= 8)
    {
        char* bm = get_lcd_pattern(ch);
        lcd_bitmap(bm, xpos, ypos, CHAR_WIDTH, CHAR_HEIGHT, true);
        return;
    }
    if (ch == 137) {
        /* Have no good font yet. Simulate the cursor character. */
        ch = '>';
    }
    str[0] = ch;
    str[1] = 0;

    lcd_clearrect(xpos, ypos, CHAR_WIDTH, CHAR_HEIGHT * double_height);
    lcd_putsxy(xpos, ypos, str);

    /* this function is being used when simulating a charcell LCD and
       then we update immediately */
    lcd_update();
}
