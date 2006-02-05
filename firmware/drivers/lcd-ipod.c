/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Rockbox driver for iPod LCDs
 *
 * Based on code from the ipodlinux project - http://ipodlinux.org/
 * Adapted for Rockbox in November 2005
 *
 * Original file: linux/arch/armnommu/mach-ipod/fb.c
 *
 * Copyright (c) 2003-2005 Bernard Leach (leachbj@bouncycastle.org)
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
#include "system.h"


/* check if number of useconds has past */
static int timer_check(int clock_start, int usecs)
{
    if ( ((int)(USEC_TIMER - clock_start)) >= usecs ) {
        return 1;
    } else {
        return 0;
    }
}


#if (CONFIG_LCD == LCD_IPOD2BPP)

/*** hardware configuration ***/

#define IPOD_LCD_BASE            0xc0001000
#define IPOD_LCD_BUSY_MASK        0x80000000

/* LCD command codes for HD66789R */


#define LCD_CMD  0x08
#define LCD_DATA 0x10

static unsigned int lcd_contrast = 0x6a;


/* wait for LCD with timeout */
static void lcd_wait_write(void)
{
    int start = USEC_TIMER;

    do {
        if ((inl(IPOD_LCD_BASE) & 0x8000) == 0) break;
    } while (timer_check(start, 1000) == 0);
}


/* send LCD data */
static void lcd_send_data(int data_lo, int data_hi)
{
    lcd_wait_write();
        outl(data_lo, IPOD_LCD_BASE + LCD_DATA);
        lcd_wait_write();
        outl(data_hi, IPOD_LCD_BASE + LCD_DATA);
}

/* send LCD command */
static void lcd_prepare_cmd(int cmd)
{
    lcd_wait_write();
    
        outl(0x0, IPOD_LCD_BASE + LCD_CMD);
        lcd_wait_write();
        outl(cmd, IPOD_LCD_BASE + LCD_CMD);
    
}

/* send LCD command and data */
static void lcd_cmd_and_data(int cmd, int data_lo, int data_hi)
{
    lcd_prepare_cmd(cmd);

    lcd_send_data(data_lo, data_hi);
}

int lcd_default_contrast(void)
{
    return 28;
}

/** 
 * 
 * LCD init 
 **/
void lcd_init_device(void){
    /* driver output control - 160x128 */
    lcd_cmd_and_data(0x1, 0x1, 0xf);
    lcd_cmd_and_data(0x5, 0x0, 0x10);
}

/*** update functions ***/
/* srccopy bitblt, opcode is currently ignored*/

/* Performance function that works with an external buffer
   note that x and bwidtht are in 8-pixel units! */
void lcd_blit(const unsigned char* data, int x, int by, int width,
              int bheight, int stride)
{
    /* TODO implement this on iPod  */
    (void)data;
    (void)x;
    (void)by;
    (void)width;
    (void)bheight;
    (void)stride;
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
    /* TODO Implement lcd_roll() */
    lines &= LCD_HEIGHT-1;
}

/*** hardware configuration ***/

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

void lcd_set_contrast(int val)
{
    lcd_cmd_and_data(0x4, 0x4, val);
    lcd_contrast = val;
}

void lcd_set_invert_display(bool yesno)
{
  /* TODO: Implement lcd_set_invert_display() */
  (void)yesno;
}

/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
  /* TODO: Implement lcd_set_flip() */
  (void)yesno;
}

void lcd_update_rect(int x, int y, int width, int height)
{
    int cursor_pos, xx;
    int ny;
    int sx = x, sy = y, mx = width, my = height;

    /* only update the ipod if we are writing to the screen */
    
    sx >>= 3;
    //mx = (mx+7)>>3;
    mx >>= 3;

    cursor_pos = sx + (sy << 5);

    for ( ny = sy; ny <= my; ny++ ) {
        unsigned char * img_data;
        

        // move the cursor
        lcd_cmd_and_data(0x11, cursor_pos >> 8, cursor_pos & 0xff);

        // setup for printing
        lcd_prepare_cmd(0x12);

        img_data = &lcd_framebuffer[ny][sx<<1];

        // 160/8 -> 20 == loops 20 times
        // make sure we loop at least once
        for ( xx = sx; xx <= mx; xx++ ) {
            // display a character
            lcd_send_data(*(img_data+1), *img_data);

            img_data += 2;
        }

        // update cursor pos counter
        cursor_pos += 0x20;
    }
}

/** Switch on or off the backlight **/
void lcd_enable (bool on){
    int lcd_state;

        lcd_state = inl(IPOD_LCD_BASE);
    if (on){
        lcd_state = lcd_state | 0x2;
        outl(lcd_state, IPOD_LCD_BASE);
        lcd_cmd_and_data(0x7, 0x0, 0x11);
    }
    else {
        lcd_state = lcd_state & ~0x2;
        outl(lcd_state, IPOD_LCD_BASE);
        lcd_cmd_and_data(0x7, 0x0, 0x9);
    }
}

#else

#define IPOD_LCD_BASE           0x70008a0c
#define IPOD_LCD_BUSY_MASK      0x80000000

/* LCD command codes for HD66789R */
#define LCD_CNTL_RAM_ADDR_SET           0x21
#define LCD_CNTL_WRITE_TO_GRAM          0x22
#define LCD_CNTL_HORIZ_RAM_ADDR_POS     0x44
#define LCD_CNTL_VERT_RAM_ADDR_POS      0x45

/*** globals ***/
static int lcd_type = 1; /* 0 = "old" Color/Photo, 1 = "new" Color & Nano */

static void lcd_wait_write(void)
{
    if ((inl(IPOD_LCD_BASE) & IPOD_LCD_BUSY_MASK) != 0) {
        int start = USEC_TIMER;

        do {
            if ((inl(IPOD_LCD_BASE) & IPOD_LCD_BUSY_MASK) == 0) break;
        } while (timer_check(start, 1000) == 0);
    }
}

static void lcd_send_lo(int v)
{
    lcd_wait_write();
    outl(v | 0x80000000, IPOD_LCD_BASE);
}

static void lcd_send_hi(int v)
{
    lcd_wait_write();
    outl(v | 0x81000000, IPOD_LCD_BASE);
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

/*** hardware configuration ***/

void lcd_set_contrast(int val)
{
  /* TODO: Implement lcd_set_contrast() */
  (void)val;
}

void lcd_set_invert_display(bool yesno)
{
  /* TODO: Implement lcd_set_invert_display() */
  (void)yesno;
}

/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
  /* TODO: Implement lcd_set_flip() */
  (void)yesno;
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
    /* TODO: Implement lcd_roll() */
    lines &= LCD_HEIGHT-1;
}

/* LCD init */
void lcd_init_device(void)
{  
#if CONFIG_LCD == LCD_IPODCOLOR
    if (ipod_hw_rev == 0x60000) {
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

#elif CONFIG_LCD == LCD_IPODNANO
    /* iPodLinux doesn't appear have any LCD init code for the Nano */
#endif
}

/*** update functions ***/

/* Performance function that works with an external buffer
   note that by and bheight are in 4-pixel units! */
void lcd_blit(const fb_data* data, int x, int by, int width,
              int bheight, int stride)
{
    /* TODO: Implement lcd_blit() */
    (void)data;
    (void)x;
    (void)by;
    (void)width;
    (void)bheight;
    (void)stride;
}

/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
    int y0, x0, y1, x1;
    int newx,newwidth;

    unsigned long *addr = (unsigned long *)lcd_framebuffer;

    /* Ensure x and width are both even - so we can read 32-bit aligned 
       data from lcd_framebuffer */
    newx=x&~1;
    newwidth=width&~1;
    if (newx+newwidth < x+width) { newwidth+=2; }
    x=newx; width=newwidth;

    /* calculate the drawing region */
#if CONFIG_LCD == LCD_IPODNANO
    y0 = x;                         /* start horiz */
    x0 = y;                         /* start vert */
    y1 = (x + width) - 1;           /* max horiz */
    x1 = (y + height) - 1;          /* max vert */
#elif CONFIG_LCD == LCD_IPODCOLOR
    y0 = y;                         /* start vert */
    x0 = (LCD_WIDTH - 1) - x;       /* start horiz */
    y1 = (y + height) - 1;          /* end vert */
    x1 = (x0 - width) + 1;          /* end horiz */
#endif
    /* setup the drawing region */
    if (lcd_type == 0) {
        lcd_cmd_data(0x12, y0);      /* start vert */
        lcd_cmd_data(0x13, x0);      /* start horiz */
        lcd_cmd_data(0x15, y1);      /* end vert */
        lcd_cmd_data(0x16, x1);      /* end horiz */
    } else {
        /* swap max horiz < start horiz */
        if (y1 < y0) {
            int t;
            t = y0;
            y0 = y1;
            y1 = t;
        }

        /* swap max vert < start vert */
        if (x1 < x0) {
            int t;
            t = x0;
            x0 = x1;
            x1 = t;
        }

        /* max horiz << 8 | start horiz */
        lcd_cmd_data(LCD_CNTL_HORIZ_RAM_ADDR_POS, (y1 << 8) | y0);
        /* max vert << 8 | start vert */
        lcd_cmd_data(LCD_CNTL_VERT_RAM_ADDR_POS, (x1 << 8) | x0);

        /* start vert = max vert */
#if CONFIG_LCD == LCD_IPODCOLOR
        x0 = x1;
#endif

        /* position cursor (set AD0-AD15) */
        /* start vert << 8 | start horiz */
        lcd_cmd_data(LCD_CNTL_RAM_ADDR_SET, ((x0 << 8) | y0));

        /* start drawing */
        lcd_send_lo(0x0);
        lcd_send_lo(LCD_CNTL_WRITE_TO_GRAM);
    }

    addr = (unsigned long*)&lcd_framebuffer[y][x];

    while (height > 0) {
        int c, r;
        int h, pixels_to_write;

        pixels_to_write = (width * height) * 2;
        h = height;

        /* calculate how much we can do in one go */
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
                while ((inl(0x70008a20) & 0x1000000) == 0);

                /* output 2 pixels */
                outl(*(addr++), 0x70008b00);
            }

            addr += (LCD_WIDTH - width)/2;
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
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

#endif
