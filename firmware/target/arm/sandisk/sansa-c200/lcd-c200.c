/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
 *
 * Copyright (C) 2007 by Mark Arigo
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

#define LCD_BASE  (*(volatile unsigned long *)(0x70003000))
#define LCD_CMD   (*(volatile unsigned long *)(0x70003008))
#define LCD_DATA  (*(volatile unsigned long *)(0x70003010))

#define LCD_BUSY  0x8000

/* check if number of useconds has past */
static inline bool timer_check(int clock_start, int usecs)
{
    return ((int)(USEC_TIMER - clock_start)) >= usecs;
}

/* wait for LCD with timeout */
static inline void lcd_wait_write(void)
{
    int start = USEC_TIMER;

    do {
        if ((LCD_BASE & LCD_BUSY) == 0)
            break;
    } while (timer_check(start, 1000) == 0);
}

/* send LCD data */
static void lcd_send_data(unsigned data)
{
    lcd_wait_write();
    LCD_DATA = (data >> 8) & 0xff;

    lcd_wait_write();
    LCD_DATA = data & 0xff;
}

/* send LCD command */
static void lcd_send_command(unsigned cmd)
{
    lcd_wait_write();
    LCD_CMD = cmd;
}

/* LCD init */
void lcd_init_device(void)
{
    /* This is from the c200 of bootloader beginning at offset 0xbbf4 */
    outl(inl(0x70000010) & ~0xfc000000, 0x70000010);
    outl(inl(0x70000010), 0x70000010);

    DEV_INIT &= ~0x400;
    udelay(10000);
    
    LCD_BASE &= ~0x4;
    udelay(15);

    LCD_BASE |= 0x4;
    udelay(10);

    LCD_BASE = 0x4687;
    udelay(10000);

    lcd_send_command(0x2c);
    udelay(20000);

    lcd_send_command(0x02);
    lcd_send_command(0x01);
    udelay(20000);

    lcd_send_command(0x26);
    lcd_send_command(0x01);
    udelay(20000);

    lcd_send_command(0x26);
    lcd_send_command(0x09);
    udelay(20000);

    lcd_send_command(0x26);
    lcd_send_command(0x0b);
    udelay(20000);

    lcd_send_command(0x26);
    lcd_send_command(0x0f);
    udelay(20000);

    lcd_send_command(0x10);
    lcd_send_command(0x07);

    lcd_send_command(0x20);
    lcd_send_command(0x03);

    lcd_send_command(0x24);
    lcd_send_command(0x03);

    lcd_send_command(0x28);
    lcd_send_command(0x01);

    lcd_send_command(0x2a);
    lcd_send_command(0x55);

    lcd_send_command(0x30);
    lcd_send_command(0x10);

    lcd_send_command(0x32);
    lcd_send_command(0x0e);

    lcd_send_command(0x34);
    lcd_send_command(0x0d);

    lcd_send_command(0x36);
    lcd_send_command(0);

    lcd_send_command(0x40);
    lcd_send_command(0x82);

    lcd_send_command(0x43);                  /* vertical dimensions */
    lcd_send_command(0x1a);                  /* y1 + 0x1a */
    lcd_send_command(LCD_HEIGHT - 1 + 0x1a); /* y2 + 0x1a */

    lcd_send_command(0x42);                  /* horizontal dimensions */
    lcd_send_command(0);                     /* x1 */
    lcd_send_command(LCD_WIDTH - 1);         /* x2 */
    udelay(100000);

    lcd_send_command(0x51);
}

/*** hardware configuration ***/
int lcd_default_contrast(void)
{
    return DEFAULT_CONTRAST_SETTING;
}

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

/* Performance function to blit a YUV bitmap directly to the LCD */
void lcd_yuv_blit(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    (void)src;
    (void)src_x;
    (void)src_y;
    (void)stride;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

/* Update a fraction of the display. */
void lcd_update_rect(int x0, int y0, int width, int height)
{
    unsigned short *addr;
    int c, r;
    int x1 = (x0 + width) - 1;
    int y1 = (y0 + height) - 1;

    if ((x1 <= 0) || (y1 <= 0))
        return;

    if(y1 >= LCD_HEIGHT)
        y1 = LCD_HEIGHT - 1;

    lcd_send_command(0x43);
    lcd_send_command(y0 + 0x1a);
    lcd_send_command(y1 + 0x1a);

    if(x1 >= LCD_WIDTH)
        x1 = LCD_WIDTH - 1;

    lcd_send_command(0x42);
    lcd_send_command(x0);
    lcd_send_command(x1);

    addr = (unsigned short*)&lcd_framebuffer[y0][x0];

    /* for each row */
    for (r = 0; r < height; r++) {
        /* for each column */
        for (c = 0; c < width; c++) {
            /* output 1 pixel */
            lcd_send_data(*(addr++));
        }

        addr += LCD_WIDTH - width;
    }
}
