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

#ifdef HAVE_LCD_BITMAP

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

/*** definitions ***/

/* LCD command codes */
#define LCD_CNTL_ON_OFF                     0xae
#define LCD_CNTL_OFF_MODE                   0xbe
#define LCD_CNTL_REVERSE                    0xa6
#define LCD_CNTL_ALL_LIGHTING               0xa4
#define LCD_CNTL_COMMON_OUTPUT_STATUS       0xc4
#define LCD_CNTL_COLUMN_ADDRESS_DIR         0xa0
#define LCD_CNTL_NLINE_ON_OFF               0xe4
#define LCD_CNTL_DISPLAY_MODE               0x66
#define LCD_CNTL_ELECTRIC_VOLUME            0x81
#define LCD_CNTL_DISPLAY_START_LINE         0x8a

#define LCD_CNTL_PAGE           0xb1
#define LCD_CNTL_COLUMN         0x13
#define LCD_CNTL_DATA_WRITE     0x1d

#define SCROLL_SPACING 3

#define SCROLLABLE_LINES 13

struct scrollinfo {
    char line[MAX_PATH + LCD_WIDTH/2 + SCROLL_SPACING + 2];
    int len;    /* length of line in chars */
    int width;  /* length of line in pixels */
    int offset;
    int startx;
    bool backward; /* scroll presently forward or backward? */
    bool bidir;
    bool invert; /* invert the scrolled text */
    long start_tick;
};

static volatile int scrolling_lines=0; /* Bitpattern of which lines are scrolling */

static void scroll_thread(void);
static char scroll_stack[DEFAULT_STACK_SIZE];
static const char scroll_name[] = "scroll";
static char scroll_ticks = 12; /* # of ticks between updates*/
static int scroll_delay = HZ/2; /* ticks delay before start */
static char scroll_step = 6;  /* pixels per scroll step */
static int bidir_limit = 50;  /* percent */
static struct scrollinfo scroll[SCROLLABLE_LINES];
static int xmargin = 0;
static int ymargin = 0;
static int curfont = FONT_SYSFIXED;
#ifndef SIMULATOR
static int xoffset = 0; /* needed for flip */
#endif

unsigned char lcd_framebuffer[LCD_HEIGHT/8][LCD_WIDTH];

/* All zeros and ones bitmaps for area filling */  
static const unsigned char zeros[16] = { 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
static const unsigned char ones[16]  = { 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

int lcd_default_contrast(void)
{
    return 32;
}

#ifdef SIMULATOR

void lcd_init(void)
{
    create_thread(scroll_thread, scroll_stack,
                  sizeof(scroll_stack), scroll_name);
}

#else

/*
 * Initialize LCD
 */
void lcd_init (void)
{
    /* GPO35 is the LCD A0 pin, GPO46 is LCD RESET */
    GPIO1_OUT |= 0x00004008;
    GPIO1_FUNCTION |= 0x00004008;
    GPIO1_ENABLE |= 0x00004008;
    
    lcd_write_command(LCD_CNTL_ON_OFF | 1); /* LCD ON */
    lcd_write_command(LCD_CNTL_COLUMN_ADDRESS_DIR | 0);   /* Normal */
    lcd_write_command(LCD_CNTL_COMMON_OUTPUT_STATUS | 1); /* Reverse dir */
    lcd_write_command(LCD_CNTL_REVERSE | 0); /* Reverse OFF */
    lcd_write_command(LCD_CNTL_ALL_LIGHTING | 0); /* Normal */
    lcd_write_command(LCD_CNTL_OFF_MODE | 1); /* OFF -> VCC on drivers */
    lcd_write_command(LCD_CNTL_NLINE_ON_OFF | 1); /* N-line ON */

    lcd_write_command_ex(LCD_CNTL_DISPLAY_MODE, 1); /* Monochrome mode */

    lcd_clear_display();
    lcd_update();

    create_thread(scroll_thread, scroll_stack,
                  sizeof(scroll_stack), scroll_name);
}


/* Performance function that works with an external buffer
   note that y and height are in 8-pixel units! */
void lcd_blit (const unsigned char* p_data, int x, int y, int width,
               int height, int stride)
{
    /* Copy display bitmap to hardware */
    while (height--)
    {
        lcd_write_command_ex(LCD_CNTL_PAGE, y++ & 0xf);
        lcd_write_command_ex(LCD_CNTL_COLUMN, x+xoffset);

        lcd_write_command(LCD_CNTL_DATA_WRITE);
        lcd_write_data(p_data, width);
        p_data += stride;
    } 
}


/*
 * Update the display.
 * This must be called after all other LCD functions that change the display.
 */
void lcd_update (void) __attribute__ ((section (".icode")));
void lcd_update (void)
{
    int y;

    /* Copy display bitmap to hardware */
    for (y = 0; y < LCD_HEIGHT/8; y++)
    {
        lcd_write_command_ex(LCD_CNTL_PAGE, y);
        lcd_write_command_ex(LCD_CNTL_COLUMN, 0);

        lcd_write_command(LCD_CNTL_DATA_WRITE);
        lcd_write_data (lcd_framebuffer[y], LCD_WIDTH);
    }
}

/*
 * Update a fraction of the display.
 */
void lcd_update_rect (int, int, int, int) __attribute__ ((section (".icode")));
void lcd_update_rect (int x_start, int y,
                      int width, int height)
{
    int ymax;

    /* The Y coordinates have to work on even 8 pixel rows */
    ymax = (y + height-1)/8;
    y /= 8;

    if(x_start + width > LCD_WIDTH)
        width = LCD_WIDTH - x_start;
    if (width <= 0)
        return; /* nothing left to do, 0 is harmful to lcd_write_data() */
    if(ymax >= LCD_HEIGHT/8)
        ymax = LCD_HEIGHT/8-1;

    /* Copy specified rectange bitmap to hardware */
    for (; y <= ymax; y++)
    {
        lcd_write_command_ex(LCD_CNTL_PAGE, y);
        lcd_write_command_ex(LCD_CNTL_COLUMN, x_start+xoffset);

        lcd_write_command(LCD_CNTL_DATA_WRITE);
        lcd_write_data (&lcd_framebuffer[y][x_start], width);
    }
}

void lcd_set_contrast(int val)
{
    lcd_write_command_ex(LCD_CNTL_ELECTRIC_VOLUME, val);
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
        xoffset = 160 - LCD_WIDTH; /* 160 colums minus the 160 we have */
    }
    else 
    {
        lcd_write_command(LCD_CNTL_COLUMN_ADDRESS_DIR | 0);
        lcd_write_command(LCD_CNTL_COMMON_OUTPUT_STATUS | 1);
        xoffset = 0;
    }
}

/**
 * Rolls up the lcd display by the specified amount of lines.
 * Lines that are rolled out over the top of the screen are
 * rolled in from the bottom again. This is a hardware 
 * remapping only and all operations on the lcd are affected.
 * -> 
 * @param int lines - The number of lines that are rolled. 
 *  The value must be 0 <= pixels < LCD_HEIGHT.
 */
void lcd_roll(int lines)
{
    char data[2];

    lines &= LCD_HEIGHT-1;
    data[0] = lines & 0xff;
    data[1] = lines >> 8;
    
    lcd_write_command(LCD_CNTL_DISPLAY_START_LINE);
    lcd_write_data(data, 2);
}

#endif /* SIMULATOR */

void lcd_clear_display (void)
{
    memset (lcd_framebuffer, 0, sizeof lcd_framebuffer);
    scrolling_lines = 0;
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
void lcd_puts(int x, int y, const unsigned char *str)
{
    lcd_puts_style(x, y, str, STYLE_DEFAULT);
}

void lcd_puts_style(int x, int y, const unsigned char *str, int style)
{
    int xpos,ypos,w,h;

#if defined(SIMULATOR) && defined(HAVE_LCD_CHARCELLS)
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

    /* make sure scrolling is turned off on the line we are updating */
    scrolling_lines &= ~(1 << y);

    if(!str || !str[0])
        return;

    lcd_getstringsize(str, &w, &h);
    xpos = xmargin + x*w / strlen(str);
    ypos = ymargin + y*h;
    lcd_putsxy(xpos, ypos, str);
    lcd_clearrect(xpos + w, ypos, LCD_WIDTH - (xpos + w), h);
    if (style & STYLE_INVERT)
        lcd_invertrect(xpos, ypos, LCD_WIDTH - xpos, h);

#if defined(SIMULATOR) && defined(HAVE_LCD_CHARCELLS)
    lcd_update();
#endif
}

/* put a string at a given pixel position, skipping first ofs pixel columns */
static void lcd_putsxyofs(int x, int y, int ofs, const unsigned char *str)
{
    int ch;
    struct font* pf = font_get(curfont);

    while ((ch = *str++) != '\0' && x < LCD_WIDTH)
    {
        int gwidth, width;

        /* check input range */
        if (ch < pf->firstchar || ch >= pf->firstchar+pf->size)
            ch = pf->defaultchar;
        ch -= pf->firstchar;

        /* no partial-height drawing for now... */
        if (y + pf->height > LCD_HEIGHT)
            break;

        /* get proportional width and glyph bits */
        gwidth = pf->width ? pf->width[ch] : pf->maxwidth;
        width = MIN (gwidth, LCD_WIDTH - x);

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
            unsigned int i;
            const unsigned char* bits = pf->bits +
                (pf->offset ? pf->offset[ch] 
                            : ((pf->height + 7) / 8 * pf->maxwidth * ch));

            if (ofs != 0)
            {
                for (i = 0; i < pf->height; i += 8)
                {
                    lcd_bitmap (bits + ofs, x, y + i, width,
                                MIN(8, pf->height - i), true);
                    bits += gwidth;
                }
            }
            else
                lcd_bitmap ((unsigned char*) bits, x, y, gwidth,
                            pf->height, true);
            x += width;
        }
        ofs = 0;
    }
}

/* put a string at a given pixel position */
void lcd_putsxy(int x, int y, const unsigned char *str)
{
    lcd_putsxyofs(x, y, 0, str);
}

/*
 * About Rockbox' internal bitmap format:
 *
 * A bitmap contains one bit for every pixel that defines if that pixel is
 * black (1) or white (0). Bits within a byte are arranged vertically, LSB
 * at top.
 * The bytes are stored in row-major order, with byte 0 being top left,
 * byte 1 2nd from left etc. The first row of bytes defines pixel rows
 * 0..7, the second row defines pixel row 8..15 etc.
 *
 * This is the same as the internal lcd hw format.
 */

/*
 * Draw a bitmap at (x, y), size (nx, ny)
 * if 'clear' is true, clear destination area first
 */
void lcd_bitmap (const unsigned char *src, int x, int y, int nx, int ny,
                 bool clear) __attribute__ ((section (".icode")));
void lcd_bitmap (const unsigned char *src, int x, int y, int nx, int ny,
                 bool clear)
{
    const unsigned char *src_col;
    unsigned char *dst, *dst_col;
    unsigned int data, mask1, mask2, mask3, mask4;
    int stride, shift;

    if (((unsigned) x >= LCD_WIDTH) || ((unsigned) y >= LCD_HEIGHT))
        return;

    stride = nx;    /* otherwise right-clipping will destroy the image */

    if (((unsigned) (x + nx)) >= LCD_WIDTH)
        nx = LCD_WIDTH - x;
    if (((unsigned) (y + ny)) >= LCD_HEIGHT)
        ny = LCD_HEIGHT - y;
        
    dst = &lcd_framebuffer[y >> 3][x];
    shift = y & 7;
    
    if (!shift && clear)  /* shortcut for byte aligned match with clear */
    {
        while (ny >= 8)   /* all full rows */
        {
            memcpy(dst, src, nx);
            src += stride;
            dst += LCD_WIDTH;
            ny -= 8;
        }
        if (ny == 0)     /* nothing left to do? */
            return;
        /* last partial row to do by default routine */
    }

    ny += shift;

    /* Calculate bit masks */
    mask4 = ~(0xfe << ((ny-1) & 7));  /* data mask for last partial row */
    if (clear)
    {
        mask1 = ~(0xff << shift); /* clearing of first partial row */
        mask2 = 0;                /* clearing of intermediate (full) rows */
        mask3 = ~mask4;           /* clearing of last partial row */
        if (ny <= 8)
            mask3 |= mask1;
    }
    else
        mask1 = mask2 = mask3 = 0xff;

    /* Loop for each column */
    for (x = 0; x < nx; x++)
    {
        src_col = src++;
        dst_col = dst++;
        data = 0;
        y = 0;

        if (ny > 8)
        {
            /* First partial row */
            data = *src_col << shift;
            *dst_col = (*dst_col & mask1) | data;
            src_col += stride;
            dst_col += LCD_WIDTH;
            data >>= 8;

            /* Intermediate rows */
            for (y = 8; y < ny-8; y += 8)
            {
                data |= *src_col << shift;
                *dst_col = (*dst_col & mask2) | data;
                src_col += stride;
                dst_col += LCD_WIDTH;
                data >>= 8;
            }
        }

        /* Last partial row */
        if (y + shift < ny)
            data |= *src_col << shift;
        *dst_col = (*dst_col & mask3) | (data & mask4);
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

/* Reverse the invert setting of the scrolling line (if any) at given char
   position.  Setting will go into affect next time line scrolls. */
void lcd_invertscroll(int x, int y)
{
    struct scrollinfo* s;

    (void)x;

    s = &scroll[y];
    s->invert = !s->invert;
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

void lcd_stop_scroll(void)
{
    scrolling_lines=0;
}

static const char scroll_tick_table[16] = {
 /* Hz values:
    1, 1.25, 1.55, 2, 2.5, 3.12, 4, 5, 6.25, 8.33, 10, 12.5, 16.7, 20, 25, 33 */
    100, 80, 64, 50, 40, 32, 25, 20, 16, 12, 10, 8, 6, 5, 4, 3
};

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
static void scroll_thread(void)
{
    struct font* pf;
    struct scrollinfo* s;
    int index;
    int xpos, ypos;

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

            lcd_clearrect(xpos, ypos, LCD_WIDTH - xpos, pf->height);
            lcd_putsxyofs(xpos, ypos, s->offset, s->line);
            if (s->invert)
                lcd_invertrect(xpos, ypos, LCD_WIDTH - xpos, pf->height);
            lcd_update_rect(xpos, ypos, LCD_WIDTH - xpos, pf->height);
        }

        sleep(scroll_ticks);
    }
}

#endif
