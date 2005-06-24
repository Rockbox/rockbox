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

#include "lcd.h"
#include "kernel.h"
#include "thread.h"
#include <string.h>
#include <stdlib.h>
#include "file.h"
#include "debug.h"
#include "system.h"
#include "font.h"
#include "hwcompat.h"

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
#define LCD_SET_MULTIPLEX_RATIO                   ((char)0xA8)
#define LCD_SET_BIAS_TC_OSC                       ((char)0xA9)
#define LCD_SET_1OVER4_BIAS_RATIO                 ((char)0xAA)
#define LCD_SET_INDICATOR_OFF                     ((char)0xAC)
#define LCD_SET_INDICATOR_ON                      ((char)0xAD)
#define LCD_SET_DISPLAY_OFF                       ((char)0xAE)
#define LCD_SET_DISPLAY_ON                        ((char)0xAF)
#define LCD_SET_PAGE_ADDRESS                      ((char)0xB0)
#define LCD_SET_COM_OUTPUT_SCAN_DIRECTION         ((char)0xC0)
#define LCD_SET_TOTAL_FRAME_PHASES                ((char)0xD2)
#define LCD_SET_DISPLAY_OFFSET                    ((char)0xD3)
#define LCD_SET_READ_MODIFY_WRITE_MODE            ((char)0xE0)
#define LCD_SOFTWARE_RESET                        ((char)0xE2)
#define LCD_NOP                                   ((char)0xE3)
#define LCD_SET_END_OF_READ_MODIFY_WRITE_MODE     ((char)0xEE)

/* LCD command codes */
#define LCD_CNTL_RESET          0xe2    /* Software reset */
#define LCD_CNTL_POWER          0x2f    /* Power control */
#define LCD_CNTL_CONTRAST       0x81    /* Contrast */
#define LCD_CNTL_OUTSCAN        0xc8    /* Output scan direction */
#define LCD_CNTL_SEGREMAP       0xa1    /* Segment remap */
#define LCD_CNTL_DISPON         0xaf    /* Display on */

#define LCD_CNTL_PAGE           0xb0    /* Page address */
#define LCD_CNTL_HIGHCOL        0x10    /* Upper column address */
#define LCD_CNTL_LOWCOL         0x00    /* Lower column address */

#define SCROLLABLE_LINES 13

/*** globals ***/

unsigned char lcd_framebuffer[LCD_HEIGHT/8][LCD_WIDTH];

static int drawmode = DRMODE_SOLID;
static int xmargin = 0;
static int ymargin = 0;
static int curfont = FONT_SYSFIXED;
#ifndef SIMULATOR
static int xoffset; /* needed for flip */
#endif

/* scrolling */
static volatile int scrolling_lines=0; /* Bitpattern of which lines are scrolling */
static void scroll_thread(void);
static char scroll_stack[DEFAULT_STACK_SIZE];
static const char scroll_name[] = "scroll";
static char scroll_ticks = 12; /* # of ticks between updates*/
static int scroll_delay = HZ/2; /* ticks delay before start */
static char scroll_step = 6;  /* pixels per scroll step */
static int bidir_limit = 50;  /* percent */
static struct scrollinfo scroll[SCROLLABLE_LINES];

static const char scroll_tick_table[16] = {
 /* Hz values:
    1, 1.25, 1.55, 2, 2.5, 3.12, 4, 5, 6.25, 8.33, 10, 12.5, 16.7, 20, 25, 33 */
    100, 80, 64, 50, 40, 32, 25, 20, 16, 12, 10, 8, 6, 5, 4, 3
};

/*** driver routines ***/

/* optimised archos recorder code is in lcd.S */

#if CONFIG_CPU == TCC730
/* Optimization opportunity:
   In the following I do exactly as in the archos firmware.
   There is probably a better way (ie. do only one mask operation)
*/
void lcd_write_command(int cmd) {
    P2 &= 0xF7;
    P2 &= 0xDF;
    P2 &= 0xFB;
    P0 = cmd;
    P2 |= 0x04;
    P2 |= 0x08;
    P2 |= 0x20;
}

void lcd_write_data( const unsigned char* data, int count ) {
    int i;
    for (i=0; i < count; i++) {
        P2 |= 0x08;
        P2 &= 0xDF;
        P2 &= 0xFB;
        P0 = data[i];
        P2 |= 0x04;
        P2 |= 0x08;
        P2 |= 0x20;
    }
}
#endif

/*** hardware configuration ***/

int lcd_default_contrast(void)
{
#ifdef SIMULATOR
    return 30;
#elif CONFIG_LCD == LCD_GMINI100
    return 31;
#else
    return (read_hw_mask() & LCD_CONTRAST_BIAS) ? 31 : 49;
#endif
}

#ifndef SIMULATOR

void lcd_set_contrast(int val)
{
    lcd_write_command(LCD_CNTL_CONTRAST);
    lcd_write_command(val);
}

void lcd_set_invert_display(bool yesno)
{
    if (yesno) 
        lcd_write_command(LCD_SET_REVERSE_DISPLAY);
    else 
        lcd_write_command(LCD_SET_NORMAL_DISPLAY);
}

/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
#ifdef HAVE_DISPLAY_FLIPPED
    if (!yesno) 
#else
    if (yesno) 
#endif
#if CONFIG_LCD == LCD_GMINI100
    {
        lcd_write_command(LCD_SET_SEGMENT_REMAP | 0x01);
        lcd_write_command(LCD_SET_COM_OUTPUT_SCAN_DIRECTION | 0x08);
        xoffset = 132 - LCD_WIDTH;
    } 
    else 
    {
        lcd_write_command(LCD_SET_SEGMENT_REMAP);
        lcd_write_command(LCD_SET_COM_OUTPUT_SCAN_DIRECTION | 0x08);
        xoffset = 0;
    }
#else

    {
        lcd_write_command(LCD_SET_SEGMENT_REMAP);
        lcd_write_command(LCD_SET_COM_OUTPUT_SCAN_DIRECTION);
        xoffset = 132 - LCD_WIDTH; /* 132 colums minus the 112 we have */
    }
    else 
    {
        lcd_write_command(LCD_SET_SEGMENT_REMAP | 0x01);
        lcd_write_command(LCD_SET_COM_OUTPUT_SCAN_DIRECTION | 0x08);
        xoffset = 0;
    }
#endif
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
    lcd_write_command(LCD_SET_DISPLAY_START_LINE | (lines & (LCD_HEIGHT-1)));
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
#if CONFIG_CPU == TCC730
    /* Initialise P0 & some P2 output pins:
       P0 -> all pins normal cmos output 
       P2 -> pins 1 to 5 normal cmos output. */
    P0CON = 0xff;
    P2CONL |= 0x5a;
    P2CONL &= 0x5b;
    P2CONH |= 1;
#else
    /* Initialize PB0-3 as output pins */
    PBCR2 &= 0xff00; /* MD = 00 */
    PBIOR |= 0x000f; /* IOR = 1 */
#endif

    /* inits like the original firmware */
    lcd_write_command(LCD_SOFTWARE_RESET);
    lcd_write_command(LCD_SET_INTERNAL_REGULATOR_RESISTOR_RATIO + 4);
    lcd_write_command(LCD_SET_1OVER4_BIAS_RATIO + 0); /* force 1/4 bias: 0 */
    lcd_write_command(LCD_SET_POWER_CONTROL_REGISTER + 7); 
               /* power control register: op-amp=1, regulator=1, booster=1 */
    lcd_write_command(LCD_SET_DISPLAY_ON);
    lcd_write_command(LCD_SET_NORMAL_DISPLAY);
    lcd_set_flip(false);
    lcd_write_command(LCD_SET_DISPLAY_START_LINE + 0);
    lcd_set_contrast(lcd_default_contrast());
    lcd_write_command(LCD_SET_PAGE_ADDRESS);
    lcd_write_command(LCD_SET_LOWER_COLUMN_ADDRESS + 0);
    lcd_write_command(LCD_SET_HIGHER_COLUMN_ADDRESS + 0);

    lcd_clear_display();
    lcd_update();

    create_thread(scroll_thread, scroll_stack,
                  sizeof(scroll_stack), scroll_name);
}

/*** Update functions ***/

/* Performance function that works with an external buffer
   note that y and height are in 8-pixel units! */
void lcd_blit(const unsigned char* p_data, int x, int y, int width,
              int height, int stride)
{
    /* Copy display bitmap to hardware */
    while (height--)
    {
        lcd_write_command (LCD_CNTL_PAGE | (y++ & 0xf));
        lcd_write_command (LCD_CNTL_HIGHCOL | (((x+xoffset)>>4) & 0xf));
        lcd_write_command (LCD_CNTL_LOWCOL | ((x+xoffset) & 0xf));

        lcd_write_data(p_data, width);
        p_data += stride;
    } 
}


/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void) __attribute__ ((section (".icode")));
void lcd_update(void)
{
    int y;

    /* Copy display bitmap to hardware */
    for (y = 0; y < LCD_HEIGHT/8; y++)
    {
        lcd_write_command (LCD_CNTL_PAGE | (y & 0xf));
        lcd_write_command (LCD_CNTL_HIGHCOL | ((xoffset>>4) & 0xf));
        lcd_write_command (LCD_CNTL_LOWCOL | (xoffset & 0xf));

        lcd_write_data (lcd_framebuffer[y], LCD_WIDTH);
    }
}

/* Update a fraction of the display. */
void lcd_update_rect(int, int, int, int) __attribute__ ((section (".icode")));
void lcd_update_rect(int x_start, int y, int width, int height)
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
        lcd_write_command (LCD_CNTL_PAGE | (y & 0xf));
        lcd_write_command (LCD_CNTL_HIGHCOL | (((x_start+xoffset)>>4) & 0xf));
        lcd_write_command (LCD_CNTL_LOWCOL | ((x_start+xoffset) & 0xf));

        lcd_write_data (&lcd_framebuffer[y][x_start], width);
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
    lcd_framebuffer[y/8][x] |= 1 << (y & 7);
}

static void clearpixel(int x, int y)
{
    lcd_framebuffer[y/8][x] &= ~(1 << (y & 7));
}

static void flippixel(int x, int y)
{
    lcd_framebuffer[y/8][x] ^= 1 << (y & 7);
}

static void nopixel(int x, int y)
{
    (void)x;
    (void)y;
}

lcd_pixelfunc_type* pixelfunc[8] = {flippixel, nopixel, setpixel, setpixel,
                               nopixel, clearpixel, nopixel, clearpixel};
                               
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

lcd_blockfunc_type* blockfunc[4] = {flipblock, bgblock, fgblock, solidblock};

/*** drawing functions ***/

/* Clear the whole display */
void lcd_clear_display(void)
{
    if (drawmode & DRMODE_INVERSEVID)
        memset (lcd_framebuffer, 0xFF, sizeof lcd_framebuffer);
    else
        memset (lcd_framebuffer, 0, sizeof lcd_framebuffer);
    scrolling_lines = 0;
}

/* Set a single pixel */
void lcd_drawpixel(int x, int y)
{
    if (((unsigned)x < LCD_WIDTH) || ((unsigned)y < LCD_HEIGHT))
        pixelfunc[drawmode](x, y);
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
    lcd_pixelfunc_type *pfunc = pixelfunc[drawmode];

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
    unsigned char *dst;
    unsigned char mask, bits;
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
        
    bfunc = blockfunc[drawmode & ~DRMODE_INVERSEVID];
    bits  = (drawmode & DRMODE_INVERSEVID) ? 0x00 : 0xFFu;
    dst   = &lcd_framebuffer[y/8][x1];
    mask  = 1 << (y & 7);

    for (x = x1; x <= x2; x++)
        bfunc(dst++, mask, bits);
}

/* Draw a vertical line (optimised) */
void lcd_vline(int x, int y1, int y2)
{
    int ny;
    unsigned char *dst;
    unsigned char mask_top, mask_bottom, bits;
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
        
    bfunc = blockfunc[drawmode & ~DRMODE_INVERSEVID];
    bits  = (drawmode & DRMODE_INVERSEVID) ? 0x00 : 0xFFu;
    dst   = &lcd_framebuffer[y1/8][x];
    ny    = y2 - (y1 & ~7);
    mask_top    = 0xFFu << (y1 & 7);
    mask_bottom = 0xFFu >> (7 - (ny & 7));
    
    if (ny >= 8)
    {
        bfunc(dst, mask_top, bits);
        dst += LCD_WIDTH;
        
        for (; ny > 15; ny -= 8)
        {
            bfunc(dst, 0xFFu, bits);
            dst += LCD_WIDTH;
        }
    }
    else
        mask_bottom &= mask_top;
        
    bfunc(dst, mask_bottom, bits);
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

/* helper function for lcd_fillrect() */
static void fillrow(lcd_blockfunc_type *bfunc, unsigned char *address,
                    int width, unsigned mask, unsigned bits)
{
    int i;
    
    for (i = 0; i < width; i++)
        bfunc(address++, mask, bits);
}

/* Fill a rectangular area */
void lcd_fillrect(int x, int y, int width, int height)
{
    int ny;
    unsigned char *dst;
    unsigned char mask_top, mask_bottom, bits;
    lcd_blockfunc_type *bfunc;
    bool fillopt = (drawmode & DRMODE_INVERSEVID) ? 
                   (drawmode & DRMODE_BG) : (drawmode & DRMODE_FG);

    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= LCD_WIDTH) || (y >= LCD_HEIGHT)
        || (x + width < 0) || (y + height < 0))
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
        
    bfunc = blockfunc[drawmode & ~DRMODE_INVERSEVID];
    bits  = (drawmode & DRMODE_INVERSEVID) ? 0x00 : 0xFFu;
    dst   = &lcd_framebuffer[y/8][x];
    ny    = height - 1 + (y & 7);
    mask_top    = 0xFFu << (y & 7);
    mask_bottom = 0xFFu >> (7 - (ny & 7));

    if (ny >= 8)
    {
        if (fillopt && mask_top == 0xFF)
            memset(dst, bits, width);
        else
            fillrow(bfunc, dst, width, mask_top, bits);
        dst += LCD_WIDTH;
        
        for (; ny > 15; ny -= 8)
        {
            if (fillopt)
                memset(dst, bits, width);
            else
                fillrow(bfunc, dst, width, 0xFFu, bits);
            dst += LCD_WIDTH;
        }
    }
    else
        mask_bottom &= mask_top;
    
    if (fillopt && mask_bottom == 0xFF)
        memset(dst, bits, width);
    else
        fillrow(bfunc, dst, width, mask_bottom, bits);
}

/* About Rockbox' internal bitmap format:
 *
 * A bitmap contains one bit for every pixel that defines if that pixel is
 * black (1) or white (0). Bits within a byte are arranged vertically, LSB
 * at top.
 * The bytes are stored in row-major order, with byte 0 being top left,
 * byte 1 2nd from left etc. The first row of bytes defines pixel rows
 * 0..7, the second row defines pixel row 8..15 etc.
 *
 * This is the same as the internal lcd hw format. */

/* Draw a bitmap at (x, y), size (nx, ny)
   if 'clear' is true, clear destination area first */
void lcd_bitmap(const unsigned char *src, int x, int y, int nx, int ny,
                bool clear) __attribute__ ((section (".icode")));
void lcd_bitmap(const unsigned char *src, int x, int y, int nx, int ny,
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

/*** Line oriented text output ***/

void lcd_puts_style(int x, int y, const unsigned char *str, int style)
{
    int xpos,ypos,w,h;
    int lastmode = lcd_get_drawmode();

    /* make sure scrolling is turned off on the line we are updating */
    scrolling_lines &= ~(1 << y);

    if(!str || !str[0])
        return;

    lcd_getstringsize(str, &w, &h);
    xpos = xmargin + x*w / strlen(str);
    ypos = ymargin + y*h;
    lcd_putsxy(xpos, ypos, str);
    lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    lcd_fillrect(xpos + w, ypos, LCD_WIDTH - (xpos + w), h);
    if (style & STYLE_INVERT)
    {
        lcd_set_drawmode(DRMODE_COMPLEMENT);
        lcd_fillrect(xpos, ypos, LCD_WIDTH - xpos, h);
    }
    lcd_set_drawmode(lastmode);
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

            lastmode = lcd_get_drawmode();
            lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            lcd_fillrect(xpos, ypos, LCD_WIDTH - xpos, pf->height);
            lcd_set_drawmode(DRMODE_SOLID);
            lcd_putsxyofs(xpos, ypos, s->offset, s->line);
            if (s->invert)
            {
                lcd_set_drawmode(DRMODE_COMPLEMENT);
                lcd_fillrect(xpos, ypos, LCD_WIDTH - xpos, pf->height);
            }
            lcd_set_drawmode(lastmode);
            lcd_update_rect(xpos, ypos, LCD_WIDTH - xpos, pf->height);
        }

        sleep(scroll_ticks);
    }
}

