/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Richard S. La Charité III
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
#include "file.h"
#include "debug.h"
#include "system.h"
#include "font.h"

/*** definitions ***/

#define LCD_REMOTE_CNTL_ADC_NORMAL          0xa0
#define LCD_REMOTE_CNTL_ADC_REVERSE         0xa1
#define LCD_REMOTE_CNTL_SHL_NORMAL          0xc0
#define LCD_REMOTE_CNTL_SHL_REVERSE         0xc8
#define LCD_REMOTE_CNTL_DISPLAY_ON_OFF      0xae
#define LCD_REMOTE_CNTL_ENTIRE_ON_OFF       0xa4
#define LCD_REMOTE_CNTL_REVERSE_ON_OFF      0xa6
#define LCD_REMOTE_CNTL_NOP                 0xe3
#define LCD_REMOTE_CNTL_POWER_CONTROL       0x2b
#define LCD_REMOTE_CNTL_SELECT_REGULATOR    0x20
#define LCD_REMOTE_CNTL_SELECT_BIAS         0xa2
#define LCD_REMOTE_CNTL_SELECT_VOLTAGE      0x81
#define LCD_REMOTE_CNTL_INIT_LINE           0x40
#define LCD_REMOTE_CNTL_SET_PAGE_ADDRESS    0xB0

#define LCD_REMOTE_CNTL_HIGHCOL             0x10    /* Upper column address */
#define LCD_REMOTE_CNTL_LOWCOL              0x00    /* Lower column address */

#define CS_LO       GPIO1_OUT &= ~0x00000004
#define CS_HI       GPIO1_OUT |= 0x00000004
#define CLK_LO      GPIO_OUT &= ~0x10000000
#define CLK_HI      GPIO_OUT |= 0x10000000
#define DATA_LO     GPIO1_OUT &= ~0x00040000
#define DATA_HI     GPIO1_OUT |= 0x00040000
#define RS_LO       GPIO_OUT &= ~0x00010000
#define RS_HI       GPIO_OUT |= 0x00010000

/* delay loop */
#define DELAY   do { int _x; for(_x=0;_x<3;_x++);} while (0)

#define SCROLLABLE_LINES 13

/*** globals ***/

unsigned char lcd_remote_framebuffer[LCD_REMOTE_HEIGHT/8][LCD_REMOTE_WIDTH]
#ifndef SIMULATOR
              __attribute__ ((section(".idata")))
#endif	
		;
		
static int curfont = FONT_SYSFIXED;
static int xmargin = 0;
static int ymargin = 0;
#ifndef SIMULATOR
static int xoffset; /* needed for flip */

/* remote hotplug */
static int countdown;  /* for remote plugging debounce */
static bool last_remote_status = false;
static bool init_remote = false;  /* scroll thread should init lcd */
static bool remote_initialized = false;
/* cached settings values */
static bool cached_invert = false;
static bool cached_flip = false;
static int cached_contrast = 32;
static int cached_roll = 0;
#endif

/* All zeros and ones bitmaps for area filling */
static const unsigned char zeros[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static const unsigned char ones[8]  = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

/* scrolling */
static volatile int scrolling_lines=0; /* Bitpattern of which lines are scrolling */
#ifndef SIMULATOR
static void scroll_thread(void);
#endif
static long scroll_stack[DEFAULT_STACK_SIZE/sizeof(long)];
static const char scroll_name[] = "remote_scroll";
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

#ifndef SIMULATOR
void lcd_remote_backlight_on(void)
{
    GPIO_OUT &= ~0x00000800;
}

void lcd_remote_backlight_off(void)
{
    GPIO_OUT |= 0x00000800;
}

void lcd_remote_write_command(int cmd)
{
    int i;
    
    RS_LO;
    CS_LO;
    
    for (i = 0; i < 8; i++)
    {
        if (cmd & 0x80)
            DATA_HI;
        else
            DATA_LO;
        
        CLK_HI;
        cmd <<= 1;
        DELAY;
        
        CLK_LO;
    }
    
    CS_HI;
}

void lcd_remote_write_data(const unsigned char* p_bytes, int count)
{
    int i, j;
    int data;
    
    RS_HI;
    CS_LO;
    
    for (i = 0; i < count; i++)
    {
        data = p_bytes[i];
        
        for (j = 0; j < 8; j++)
        {
            if (data & 0x80)
                DATA_HI;
            else
                DATA_LO;
            
            CLK_HI;
            data <<= 1;
            DELAY;
            
            CLK_LO;
        }
    }
    
    CS_HI;
}

void lcd_remote_write_command_ex(int cmd, int data)
{
    int i;
    
    CS_LO;
    RS_LO;
    
    for (i = 0; i < 8; i++)
    {
        if (cmd & 0x80)
            DATA_HI;
        else
            DATA_LO;
        
        CLK_HI;
        cmd <<= 1;
        DELAY;
        
        CLK_LO;
    }
    
    for (i = 0; i < 8; i++)
    {
        if (data & 0x80)
            DATA_HI;
        else
            DATA_LO;
        
        CLK_HI;
        data <<= 1;
        DELAY;
        
        CLK_LO;
    }
    
    CS_HI;
}
#endif /* !SIMULATOR */

/*** hardware configuration ***/

int lcd_remote_default_contrast(void)
{
    return 32;
}

#ifndef SIMULATOR

void lcd_remote_powersave(bool on)
{
    if (remote_initialized)
    {
        lcd_remote_write_command(LCD_REMOTE_CNTL_DISPLAY_ON_OFF | (on ? 0 : 1));
        lcd_remote_write_command(LCD_REMOTE_CNTL_ENTIRE_ON_OFF | (on ? 1 : 0));
    }
}

void lcd_remote_set_contrast(int val)
{
    cached_contrast = val;
    if (remote_initialized)
        lcd_remote_write_command_ex(LCD_REMOTE_CNTL_SELECT_VOLTAGE, val);
}

void lcd_remote_set_invert_display(bool yesno)
{
    cached_invert = yesno;
    if (remote_initialized)
        lcd_remote_write_command(LCD_REMOTE_CNTL_REVERSE_ON_OFF | yesno);
}

/* turn the display upside down (call lcd_remote_update() afterwards) */
void lcd_remote_set_flip(bool yesno)
{
    cached_flip = yesno;
    if (yesno)
    {
        xoffset = 0;
        if (remote_initialized)
        {
            lcd_remote_write_command(LCD_REMOTE_CNTL_ADC_NORMAL);
            lcd_remote_write_command(LCD_REMOTE_CNTL_SHL_NORMAL);
        }
    }
    else
    {
        xoffset = 132 - LCD_REMOTE_WIDTH;
        if (remote_initialized)
        {
            lcd_remote_write_command(LCD_REMOTE_CNTL_ADC_REVERSE);
            lcd_remote_write_command(LCD_REMOTE_CNTL_SHL_REVERSE);
        }
    }
}

/* Rolls up the lcd display by the specified amount of lines.
 * Lines that are rolled out over the top of the screen are
 * rolled in from the bottom again. This is a hardware 
 * remapping only and all operations on the lcd are affected.
 * -> 
 * @param int lines - The number of lines that are rolled. 
 *  The value must be 0 <= pixels < LCD_REMOTE_HEIGHT. */
void lcd_remote_roll(int lines)
{
    char data[2];
    
    cached_roll = lines;

    if (remote_initialized)
    {
        lines &= LCD_REMOTE_HEIGHT-1;
        data[0] = lines & 0xff;
        data[1] = lines >> 8;
    
        lcd_remote_write_command(LCD_REMOTE_CNTL_INIT_LINE | 0x0); // init line
        lcd_remote_write_data(data, 2);
    }
}

/* The actual LCD init */
static void remote_lcd_init(void)
{
    lcd_remote_write_command(LCD_REMOTE_CNTL_SELECT_BIAS | 0x0);

    lcd_remote_write_command(LCD_REMOTE_CNTL_POWER_CONTROL | 0x5);
    sleep(1);
    lcd_remote_write_command(LCD_REMOTE_CNTL_POWER_CONTROL | 0x6);
    sleep(1);
    lcd_remote_write_command(LCD_REMOTE_CNTL_POWER_CONTROL | 0x7);
    
    lcd_remote_write_command(LCD_REMOTE_CNTL_SELECT_REGULATOR | 0x4); // 0x4 Select regulator @ 5.0 (default);
    
    sleep(1);
    
    lcd_remote_write_command(LCD_REMOTE_CNTL_INIT_LINE | 0x0); // init line
    lcd_remote_write_command(LCD_REMOTE_CNTL_SET_PAGE_ADDRESS | 0x0); // page address
    lcd_remote_write_command_ex(0x10, 0x00); // Column MSB + LSB

    lcd_remote_write_command(LCD_REMOTE_CNTL_DISPLAY_ON_OFF | 1);
    
    remote_initialized = true;

    lcd_remote_set_flip(cached_flip);
    lcd_remote_set_contrast(cached_contrast);
    lcd_remote_set_invert_display(cached_invert);
    lcd_remote_roll(cached_roll);
}

/* Monitor remote hotswap */
static void remote_tick(void)
{
    bool current_status;

    current_status = ((GPIO_READ & 0x40000000) == 0);
    /* Only report when the status has changed */
    if (current_status != last_remote_status)
    {
        last_remote_status = current_status;
        countdown = current_status ? HZ : 1;
    }
    else
    {
        /* Count down until it gets negative */
        if (countdown >= 0)
            countdown--;

        if (countdown == 0)
        {
            if (current_status)
            {
                init_remote = true;   
                /* request init in scroll_thread */
            }
            else
            {
                CLK_LO;
                CS_HI;
                remote_initialized = false;
            }
        }
    }
}

/* Initialise ports and kick off monitor */
void lcd_remote_init(void)
{
    GPIO_FUNCTION   |= 0x10010800;  /* GPIO11: Backlight
                                       GPIO16: RS
                                       GPIO28: CLK */
    
    GPIO1_FUNCTION  |= 0x00040004;  /* GPIO34: CS
                                       GPIO50: Data */
    GPIO_ENABLE     |= 0x10010800;
    GPIO1_ENABLE    |= 0x00040004;
    
    lcd_remote_clear_display();

    tick_add_task(remote_tick);
    create_thread(scroll_thread, scroll_stack,
                  sizeof(scroll_stack), scroll_name);
}

/*** update functions ***/

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_remote_update(void) __attribute__ ((section (".icode")));
void lcd_remote_update(void)
{
    int y;
    
    if (!remote_initialized)
        return;

    /* Copy display bitmap to hardware */
    for (y = 0; y < LCD_REMOTE_HEIGHT / 8; y++)
    {
        lcd_remote_write_command(LCD_REMOTE_CNTL_SET_PAGE_ADDRESS | y);
        lcd_remote_write_command(LCD_REMOTE_CNTL_HIGHCOL | ((xoffset>>4) & 0xf));
        lcd_remote_write_command(LCD_REMOTE_CNTL_LOWCOL | (xoffset & 0xf));
        lcd_remote_write_data(lcd_remote_framebuffer[y], LCD_REMOTE_WIDTH);
    }
}

/* Update a fraction of the display. */
void lcd_remote_update_rect(int, int, int, int) __attribute__ ((section (".icode")));
void lcd_remote_update_rect(int x_start, int y, int width, int height)
{
    int ymax;
    
    if (!remote_initialized)
        return;

    /* The Y coordinates have to work on even 8 pixel rows */
    ymax = (y + height-1)/8;
    y /= 8;

    if(x_start + width > LCD_REMOTE_WIDTH)
        width = LCD_REMOTE_WIDTH - x_start;
    if (width <= 0)
        return; /* nothing left to do, 0 is harmful to lcd_write_data() */
    if(ymax >= LCD_REMOTE_HEIGHT/8)
        ymax = LCD_REMOTE_HEIGHT/8-1;

    /* Copy specified rectange bitmap to hardware */
    for (; y <= ymax; y++)
    {        
        lcd_remote_write_command(LCD_REMOTE_CNTL_SET_PAGE_ADDRESS | y);
        lcd_remote_write_command(LCD_REMOTE_CNTL_HIGHCOL
                                 | (((x_start+xoffset)>>4) & 0xf));
        lcd_remote_write_command(LCD_REMOTE_CNTL_LOWCOL 
                                 | ((x_start+xoffset) & 0xf));
        lcd_remote_write_data(&lcd_remote_framebuffer[y][x_start], width);
    }
}
#endif /* !SIMULATOR */

/*** parameter handling ***/

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

/*** drawing functions ***/

void lcd_remote_clear_display(void)
{
    memset(lcd_remote_framebuffer, 0, sizeof lcd_remote_framebuffer);
}

/* Set a single pixel */
void lcd_remote_drawpixel(int x, int y)
{
    REMOTE_DRAW_PIXEL(x,y);
}

/* Clear a single pixel */
void lcd_remote_clearpixel(int x, int y)
{
    REMOTE_CLEAR_PIXEL(x,y);
}

/* Invert a single pixel */
void lcd_remote_invertpixel(int x, int y)
{
    REMOTE_INVERT_PIXEL(x,y);
}

void lcd_remote_drawline(int x1, int y1, int x2, int y2)
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
        REMOTE_DRAW_PIXEL(x,y);

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

void lcd_remote_clearline(int x1, int y1, int x2, int y2)
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
        REMOTE_CLEAR_PIXEL(x,y);

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

void lcd_remote_drawrect(int x, int y, int nx, int ny)
{
    int i;

    if (x > LCD_REMOTE_WIDTH)
    {
        return;
    }
    
    if (y > LCD_REMOTE_HEIGHT)
    {
        return;
    }

    if (x + nx > LCD_REMOTE_WIDTH)
    {
        nx = LCD_REMOTE_WIDTH - x;
    }
    
    if (y + ny > LCD_REMOTE_HEIGHT)
    {
        ny = LCD_REMOTE_HEIGHT - y;
    }

    /* vertical lines */
    for (i = 0; i < ny; i++)
    {
        REMOTE_DRAW_PIXEL(x, (y + i));
        REMOTE_DRAW_PIXEL((x + nx - 1), (y + i));
    }

    /* horizontal lines */
    for (i = 0; i < nx; i++)
    {
        REMOTE_DRAW_PIXEL((x + i),y);
        REMOTE_DRAW_PIXEL((x + i),(y + ny - 1));
    }
}

/* Clear a rectangular area at (x, y), size (nx, ny) */
void lcd_remote_clearrect(int x, int y, int nx, int ny)
{
    int i;
    for (i = 0; i < nx; i++)
        lcd_remote_bitmap(zeros, x+i, y, 1, ny, true);
}

/* Fill a rectangular area at (x, y), size (nx, ny) */
void lcd_remote_fillrect(int x, int y, int nx, int ny)
{
    int i;
    for (i = 0; i < nx; i++)
        lcd_remote_bitmap(ones, x+i, y, 1, ny, true);
}

/* Invert a rectangular area at (x, y), size (nx, ny) */
void lcd_remote_invertrect(int x, int y, int nx, int ny)
{
    int i, j;

    if (x > LCD_REMOTE_WIDTH)
        return;
    if (y > LCD_REMOTE_HEIGHT)
        return;

    if (x + nx > LCD_REMOTE_WIDTH)
        nx = LCD_REMOTE_WIDTH - x;
    if (y + ny > LCD_REMOTE_HEIGHT)
        ny = LCD_REMOTE_HEIGHT - y;

    for (i = 0; i < nx; i++)
        for (j = 0; j < ny; j++)
            REMOTE_INVERT_PIXEL((x + i), (y + j));
}

void lcd_remote_bitmap(const unsigned char *src, int x, int y, int nx, int ny,
                       bool clear) __attribute__ ((section (".icode")));
void lcd_remote_bitmap(const unsigned char *src, int x, int y, int nx, int ny,
                       bool clear)
{
    const unsigned char *src_col;
    unsigned char *dst, *dst_col;
    unsigned int data, mask1, mask2, mask3, mask4;
    int stride, shift;
    
    if (((unsigned) x >= LCD_REMOTE_WIDTH) || ((unsigned) y >= LCD_REMOTE_HEIGHT))
    {
        return;
    }
    
    stride = nx;    /* otherwise right-clipping will destroy the image */

    if (((unsigned) (x + nx)) >= LCD_REMOTE_WIDTH)
    {
        nx = LCD_REMOTE_WIDTH - x;
    }
    
    if (((unsigned) (y + ny)) >= LCD_REMOTE_HEIGHT)
    {
        ny = LCD_REMOTE_HEIGHT - y;
    }
        
    dst = &lcd_remote_framebuffer[y >> 3][x];
    shift = y & 7;
    
    if (!shift && clear)  /* shortcut for byte aligned match with clear */
    {
        while (ny >= 8)   /* all full rows */
        {
            memcpy(dst, src, nx);
            src += stride;
            dst += LCD_REMOTE_WIDTH;
            ny -= 8;
        }
        if (ny == 0)     /* nothing left to do? */
        {
            return;
        }
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
        {
            mask3 |= mask1;
        }
    }
    else
    {
        mask1 = mask2 = mask3 = 0xff;
    }

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
            dst_col += LCD_REMOTE_WIDTH;
            data >>= 8;

            /* Intermediate rows */
            for (y = 8; y < ny-8; y += 8)
            {
                data |= *src_col << shift;
                *dst_col = (*dst_col & mask2) | data;
                src_col += stride;
                dst_col += LCD_REMOTE_WIDTH;
                data >>= 8;
            }
        }

        /* Last partial row */
        if (y + shift < ny)
        {
            data |= *src_col << shift;
        }
        
        *dst_col = (*dst_col & mask3) | (data & mask4);
    }
}

/* put a string at a given pixel position, skipping first ofs pixel columns */
static void lcd_remote_putsxyofs(int x, int y, int ofs, const unsigned char *str)
{
    int ch;
    struct font* pf = font_get(curfont);

    while ((ch = *str++) != '\0' && x < LCD_REMOTE_WIDTH)
    {
        int gwidth, width;

        /* check input range */
        if (ch < pf->firstchar || ch >= pf->firstchar+pf->size)
            ch = pf->defaultchar;
        ch -= pf->firstchar;

        /* get proportional width and glyph bits */
        gwidth = pf->width ? pf->width[ch] : pf->maxwidth;
        width = MIN (gwidth, LCD_REMOTE_WIDTH - x);

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
                    lcd_remote_bitmap (bits + ofs, x, y + i, width,
                                MIN(8, pf->height - i), true);
                    bits += gwidth;
                }
            }
            else
                lcd_remote_bitmap ((unsigned char*) bits, x, y, gwidth,
                            pf->height, true);
            x += width;
        }
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
    lcd_remote_puts_style(x, y, str, STYLE_DEFAULT);
}

void lcd_remote_puts_style(int x, int y, const unsigned char *str, int style)
{
    int xpos,ypos,w,h;

    /* make sure scrolling is turned off on the line we are updating */
    //scrolling_lines &= ~(1 << y);

    if(!str || !str[0])
        return;

    lcd_remote_getstringsize(str, &w, &h);
    xpos = xmargin + x*w / strlen(str);
    ypos = ymargin + y*h;
    lcd_remote_putsxy(xpos, ypos, str);
    lcd_remote_clearrect(xpos + w, ypos, LCD_REMOTE_WIDTH - (xpos + w), h);
    if (style & STYLE_INVERT)
        lcd_remote_invertrect(xpos, ypos, LCD_REMOTE_WIDTH - xpos, h);

}

/*** scrolling ***/

/* Reverse the invert setting of the scrolling line (if any) at given char
   position.  Setting will go into affect next time line scrolls. */
void lcd_remote_invertscroll(int x, int y)
{
    struct scrollinfo* s;

    (void)x;

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
    struct scrollinfo* s;
    int w, h;

    s = &scroll[y];

    s->start_tick = current_tick + scroll_delay;
    s->invert = false;
    if (style & STYLE_INVERT) {
        s->invert = true;
        lcd_remote_puts_style(x,y,string,STYLE_INVERT);
    }
    else
        lcd_remote_puts(x,y,string);

    lcd_remote_getstringsize(string, &w, &h);

    if (LCD_REMOTE_WIDTH - x * 8 - xmargin < w) {
        /* prepare scroll line */
        char *end;

        memset(s->line, 0, sizeof s->line);
        strcpy(s->line, string);

        /* get width */
        s->width = lcd_remote_getstringsize(s->line, &w, &h);

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
            s->width = lcd_remote_getstringsize(s->line, &w, &h);
        }

        end = strchr(s->line, '\0');
        strncpy(end, string, LCD_REMOTE_WIDTH/2);

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

#ifndef SIMULATOR
static void scroll_thread(void)
{
    struct font* pf;
    struct scrollinfo* s;
    int index;
    int xpos, ypos;

    /* initialize scroll struct array */
    scrolling_lines = 0;

    while ( 1 ) {

        if (init_remote)   /* request to initialize the remote lcd */
        {
            init_remote = false; /* clear request */
            remote_lcd_init();
            lcd_remote_update();
        }

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

            lcd_remote_clearrect(xpos, ypos, LCD_REMOTE_WIDTH - xpos, pf->height);
            lcd_remote_putsxyofs(xpos, ypos, s->offset, s->line);
            if (s->invert)
                lcd_remote_invertrect(xpos, ypos, LCD_REMOTE_WIDTH - xpos, pf->height);
            lcd_remote_update_rect(xpos, ypos, LCD_REMOTE_WIDTH - xpos, pf->height);
        }

        sleep(scroll_ticks);
    }
}
#endif /* SIMULATOR */

