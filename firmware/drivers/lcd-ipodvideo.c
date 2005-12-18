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
 * Rockbox driver for iPod Video LCDs
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
    /* iPodLinux doesn't appear have any LCD init code for the Video */
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

static void lcd_bcm_write32(unsigned address, unsigned value)
{
    /* write out destination address as two 16bit values */
    outw(address, 0x30010000);
    outw((address >> 16), 0x30010000);

    /* wait for it to be write ready */
    while ((inw(0x30030000) & 0x2) == 0);

    /* write out the value low 16, high 16 */
    outw(value, 0x30000000);
    outw((value >> 16), 0x30000000);
}

static void lcd_bcm_setup_rect(unsigned cmd,
                               unsigned start_horiz, 
                               unsigned start_vert, 
                               unsigned max_horiz,
                               unsigned max_vert, 
                               unsigned count)
{
    lcd_bcm_write32(0x1F8, 0xFFFA0005);
    lcd_bcm_write32(0xE0000, cmd);
    lcd_bcm_write32(0xE0004, start_horiz);
    lcd_bcm_write32(0xE0008, start_vert);
    lcd_bcm_write32(0xE000C, max_horiz);
    lcd_bcm_write32(0xE0010, max_vert);
    lcd_bcm_write32(0xE0014, count);
    lcd_bcm_write32(0xE0018, count);
    lcd_bcm_write32(0xE001C, 0);
}

static unsigned lcd_bcm_read32(unsigned address) {
    while ((inw(0x30020000) & 1) == 0);

    /* write out destination address as two 16bit values */
    outw(address, 0x30020000);
    outw((address >> 16), 0x30020000);

    /* wait for it to be read ready */
    while ((inw(0x30030000) & 0x10) == 0);

    /* read the value */
    return inw(0x30000000) | inw(0x30000000) << 16;
}

static void lcd_bcm_finishup(void) {
    unsigned data; 

    outw(0x31, 0x30030000); 

    lcd_bcm_read32(0x1FC);

    do {
        data = lcd_bcm_read32(0x1F8);
    } while (data == 0xFFFA0005 || data == 0xFFFF);

    lcd_bcm_read32(0x1FC);
}

/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
    int rect1,rect2,rect3,rect4;
    int newx,newwidth;
    int count;
    unsigned int curpixel=0;
    int c, r;
    unsigned long *addr;
    int p0,p1;

    /* Ensure x and width are both even - so we can read 32-bit aligned 
       data from lcd_framebuffer */
    newx=x&~1;
    newwidth=width&~1;
    if (newx+newwidth < x+width) { newwidth+=2; }
    x=newx; width=newwidth;

    /* calculate the drawing region */
    rect1 = x;                         /* start horiz */
    rect2 = y;                         /* start vert */
    rect3 = (x + width) - 1;           /* max horiz */
    rect4 = (y + height) - 1;          /* max vert */

    /* setup the drawing region */
    count=(width * height) << 1;
    lcd_bcm_setup_rect(0x34, rect1, rect2, rect3, rect4, count);

    addr = (unsigned long*)&lcd_framebuffer[y][x];

    /* for each row */
    for (r = 0; r < height; r++) {
        /* for each column */
        for (c = 0; c < width; c += 2) {
            /* output 2 pixels */
            lcd_bcm_write32(0xE0020 + (curpixel << 2), *(addr++));
            curpixel++;
        }

        addr += (LCD_WIDTH - width)/2;
    }

    lcd_bcm_finishup();
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}
