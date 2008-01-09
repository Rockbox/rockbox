/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Mark Arigo
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

/* send LCD data */
static void lcd_send_data(unsigned data)
{
    while (LCD1_CONTROL & LCD1_BUSY_MASK); /* wait for LCD */
    LCD1_DATA = data;
}

/* send LCD command */
static void lcd_send_command(unsigned cmd)
{
    while (LCD1_CONTROL & LCD1_BUSY_MASK); /* wait for LCD */
    LCD1_CMD = cmd;
}

/* LCD init */
void lcd_init_device(void)
{
    int i;

    DEV_INIT1 &= ~0xfc000000;

    i = DEV_INIT1;
    DEV_INIT1 = i;

    DEV_INIT2 &= ~0x400;
    udelay(10000);

    LCD1_CONTROL &= ~0x4;
    udelay(15);

    LCD1_CONTROL |= 0x4;
    udelay(10);

    LCD1_CONTROL = 0x690;
    LCD1_CONTROL = 0x694;

    /* OF just reads these */
    i = LCD1_CONTROL;
    i = inl(0x70003004);
    i = LCD1_CMD;
    i = inl(0x7000300c);

#if 0
    /* this is skipped in the OF */ 
    LCD1_CONTROL &= ~0x200;
    LCD1_CONTROL &= ~0x800;
    LCD1_CONTROL &= ~0x400;
#endif

    LCD1_CONTROL |= 0x1;
    udelay(15000);

    lcd_send_command(0xe2);
    lcd_send_command(0x2f);
    lcd_send_command(0x26);
    lcd_send_command(0xcc);
    lcd_send_command(0xe8);
    lcd_send_command(0x81);
    lcd_send_command(0);
    lcd_send_command(0x40);
    lcd_send_command(0xa6);
    lcd_send_command(0x88);
    lcd_send_command(0xb0);
    lcd_send_command(0x10);
    lcd_send_command(0);
}

/*** hardware configuration ***/
int lcd_default_contrast(void)
{
    return DEFAULT_CONTRAST_SETTING;
}

void lcd_set_contrast(int val)
{
    lcd_send_command(0x81);
    lcd_send_command(val);
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
   note that by and bheight are in 8-pixel units! */
void lcd_blit(const unsigned char* data, int x, int by, int width,
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

/* Performance function that works with an external buffer
   note that by and bheight are in 4-pixel units! */
void lcd_grey_phase_blit(const struct grey_data *data, int x, int by, 
                         int width, int bheight, int stride)
{
    /* TODO: Implement lcd_grey_phase_blit() */
    (void)data;
    (void)x;
    (void)by;
    (void)width;
    (void)bheight;
    (void)stride;
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
    unsigned char *addr;
    unsigned int cmd0, cmd1, cmd2;
    int r, c, x1, y1, start_row, last_row;

    x1 = (x0 + width) - 1;
    y1 = (y0 + height) - 1;
    if ((x1 <= 0) || (y1 <= 0))
        return;

    if(x1 >= LCD_WIDTH)
        x1 = LCD_WIDTH - 1;

    if(y1 >= LCD_HEIGHT)
        y1 = LCD_HEIGHT - 1;

    start_row = y0/8;
    last_row  = y1/8;

    cmd1 = (x0 & 0xff) >> 4;
    cmd1 = (cmd1 + 5) | 0x10;
    cmd2 = x0 & 0xf;

    for (r = start_row; r <= last_row; r++) {
        cmd0 = (r & 0xff) | 0xb0;
        lcd_send_command(cmd0);
        lcd_send_command(cmd1);
        lcd_send_command(cmd2);

        addr = (unsigned char*)&lcd_framebuffer[r][x0];
        for (c = x0; c <= x1; c++)
            lcd_send_data(*(addr++));
    }

    lcd_send_command(0xaf);
}
