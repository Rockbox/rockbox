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
 * Rockbox driver for iPod LCDs
 *
 * Based on code from ipodlinux - http://ipodlinux.org
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

/*** globals ***/
static int lcd_type = 1; /* 0 = "old" Color/Photo, 1 = "new" Color & Nano */

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
static int timer_check(unsigned long clock_start, unsigned long usecs)
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
    int rect1, rect2, rect3, rect4;

    unsigned long *addr = (unsigned long *)lcd_framebuffer;

    /* TODO: Ensure x is even - so we read 32-bit aligned data from 
             lcd_framebuffer */
    
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

    addr += x * LCD_WIDTH + y/2;

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

