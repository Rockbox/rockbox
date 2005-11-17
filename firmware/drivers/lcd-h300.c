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

void lcd_write_reg(int reg, int val)
{
    *(volatile unsigned short *)0xf0000000 = reg;
    *(volatile unsigned short *)0xf0000002 = val;
}

void lcd_begin_write_gram(void)
{
    *(volatile unsigned short *)0xf0000000 = 0x22;
}

void lcd_write_data(const unsigned short* p_bytes, int count) ICODE_ATTR;
void lcd_write_data(const unsigned short* p_bytes, int count)
{
    while(count--)
        *(volatile unsigned short *)0xf0000002 = *p_bytes++;
}

/*** hardware configuration ***/

void lcd_set_contrast(int val)
{
    (void)val;
}

void lcd_set_invert_display(bool yesno)
{
    (void)yesno;
}

/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
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
    (void)lines;
}


/* LCD init */
void lcd_init_device(void)
{
    /* GPO46 is LCD RESET */
    or_l(0x00004000, &GPIO1_OUT);
    or_l(0x00004000, &GPIO1_ENABLE);
    or_l(0x00004000, &GPIO1_FUNCTION);

    /* Reset LCD */
    sleep(1);
    and_l(~0x00004000, &GPIO1_OUT);
    sleep(1);
    or_l(0x00004000, &GPIO1_OUT);
    sleep(1);

    lcd_write_reg(0x00, 0x0001);
    sleep(1);
    lcd_write_reg(0x07, 0x0040);
    lcd_write_reg(0x12, 0x0000);
    lcd_write_reg(0x13, 0x0000);
    sleep(1);
    lcd_write_reg(0x11, 0x0003);
    lcd_write_reg(0x12, 0x0008);
    lcd_write_reg(0x13, 0x3617);
    lcd_write_reg(0x12, 0x0008);
    lcd_write_reg(0x10, 0x0004);
    lcd_write_reg(0x10, 0x0004);
    lcd_write_reg(0x11, 0x0002);
    lcd_write_reg(0x12, 0x0018);
    lcd_write_reg(0x10, 0x0044);
    sleep(1);
    lcd_write_reg(0x10, 0x0144);
    lcd_write_reg(0x10, 0x0540);
    lcd_write_reg(0x13, 0x3218);
    lcd_write_reg(0x01, 0x001b);
    lcd_write_reg(0x02, 0x0700);
    lcd_write_reg(0x03, 0x7038);
    lcd_write_reg(0x04, 0x7030);
    lcd_write_reg(0x05, 0x0000);
    lcd_write_reg(0x40, 0x0000);
    lcd_write_reg(0x41, 0x0000);
    lcd_write_reg(0x42, 0xdb00);
    lcd_write_reg(0x43, 0x0000);
    lcd_write_reg(0x44, 0xaf00);
    lcd_write_reg(0x45, 0xdb00);

    lcd_write_reg(0x0b, 0x0002);
    lcd_write_reg(0x0c, 0x0003);
    sleep(1);
    lcd_write_reg(0x10, 0x4540);
    lcd_write_reg(0x07, 0x0041);
    sleep(1);
    lcd_write_reg(0x08, 0x0808);
    lcd_write_reg(0x09, 0x003f);
    sleep(1);
    lcd_write_reg(0x07, 0x0636);
    sleep(1);
    lcd_write_reg(0x07, 0x0626);
    sleep(1);
    lcd_write_reg(0x30, 0x0003);
    lcd_write_reg(0x31, 0x0707);
    lcd_write_reg(0x32, 0x0007);
    lcd_write_reg(0x33, 0x0705);
    lcd_write_reg(0x34, 0x0007);
    lcd_write_reg(0x35, 0x0000);
    lcd_write_reg(0x36, 0x0407);
    lcd_write_reg(0x37, 0x0507);
    lcd_write_reg(0x38, 0x1d09);
    lcd_write_reg(0x39, 0x0303);
    
    lcd_write_reg(0x13, 0x2610);

    /* LCD ON */
    lcd_write_reg(0x07, 0x0061);
    sleep(1);
    lcd_write_reg(0x07, 0x0067);
    sleep(1);
    lcd_write_reg(0x07, 0x0637);
}

/*** update functions ***/

/* Performance function that works with an external buffer
   note that by and bheight are in 8-pixel units! */
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


/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void) ICODE_ATTR;
void lcd_update(void)
{
    /* Copy display bitmap to hardware */
    lcd_write_reg(0x21, 0);
    lcd_begin_write_gram();
    lcd_write_data((unsigned short *)lcd_framebuffer, LCD_WIDTH*LCD_HEIGHT);
}

/* Update a fraction of the display. */
void lcd_update_rect(int, int, int, int) ICODE_ATTR;
void lcd_update_rect(int x, int y, int width, int height)
{
    int ymax = y + height;

    if(x + width > LCD_WIDTH)
        width = LCD_WIDTH - x;
    if (width <= 0)
        return; /* nothing left to do, 0 is harmful to lcd_write_data() */
    if(ymax >= LCD_HEIGHT)
        ymax = LCD_HEIGHT-1;

    /* Copy specified rectangle bitmap to hardware */
    for (; y <= ymax; y++)
    {
        lcd_write_reg(0x21, (x << 8) + y);
        lcd_begin_write_gram();
        lcd_write_data ((unsigned short *)&lcd_framebuffer[y][x], width);
    }
}
