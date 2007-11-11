/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Karl Kurbjun
 *
 * Some of this is based on the Cowon A2 Firmware release:
 * http://www.cowonglobal.com/download/gnu/cowon_pmp_a2_src_1.59_GPL.tar.gz
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
#include "string.h"
#include "lcd.h"
#include "kernel.h"
#include "memory.h"
#include "system-target.h"

/* Copies a rectangle from one framebuffer to another. Can be used in
   single transfer mode with width = num pixels, and height = 1 which
   allows a full-width rectangle to be copied more efficiently. */
extern void lcd_copy_buffer_rect(fb_data *dst, const fb_data *src,
                                 int width, int height);

static volatile bool lcd_on = true;
volatile bool lcd_poweroff = false;
/*
** These are imported from lcd-16bit.c
*/
extern unsigned fg_pattern;
extern unsigned bg_pattern;

bool lcd_enabled(void)
{
    return lcd_on;
}

/* LCD init - based on code from ingenient-bsp/bootloader/board/dm320/splash.c
 *  and code by Catalin Patulea from the M:Robe 500i linux port
 */
void lcd_init_device(void)
{
    unsigned int addr;
    
    /* Clear the Frame */
    memset16(FRAME, 0x0000, LCD_WIDTH*LCD_HEIGHT);

    outw(0x00ff, IO_OSD_MODE);
    outw(0x0002, IO_OSD_VIDWINMD);
    outw(0x2001, IO_OSD_OSDWINMD0);
    outw(0x0002, IO_OSD_OSDWINMD1);
    outw(0x0000, IO_OSD_ATRMD);
    outw(0x0000, IO_OSD_RECTCUR);

    outw((480*2) / 32, IO_OSD_OSDWIN0OFST);
    addr = ((int)FRAME-CONFIG_SDRAM_START) / 32;
    outw(addr >> 16, IO_OSD_OSDWINADH);
    outw(addr & 0xFFFF, IO_OSD_OSDWIN0ADL);

    outw(80, IO_OSD_BASEPX);
    outw(2, IO_OSD_BASEPY);

    outw(0, IO_OSD_OSDWIN0XP);
    outw(0, IO_OSD_OSDWIN0YP);
    outw(480, IO_OSD_OSDWIN0XL);
    outw(640, IO_OSD_OSDWIN0YL);
}

/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
    fb_data *dst, *src;

    if (!lcd_on)
        return;

    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x; /* Clip right */
    if (x < 0)
        width += x, x = 0; /* Clip left */
    if (width <= 0)
        return; /* nothing left to do */

    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y; /* Clip bottom */
    if (y < 0)
        height += y, y = 0; /* Clip top */
    if (height <= 0)
        return; /* nothing left to do */

#if CONFIG_ORIENTATION == SCREEN_PORTAIT
    dst = (fb_data *)FRAME + LCD_WIDTH*y + x;
    src = &lcd_framebuffer[y][x];

    /* Copy part of the Rockbox framebuffer to the second framebuffer */
    if (width < LCD_WIDTH)
    {
        /* Not full width - do line-by-line */
        lcd_copy_buffer_rect(dst, src, width, height);
    }
    else
    {
        /* Full width - copy as one line */
        lcd_copy_buffer_rect(dst, src, LCD_WIDTH*height, 1);
    }
#else

#if 0
    src = &lcd_framebuffer[y][x];
    
    register int xc, yc;
    register fb_data *start=(fb_data *)FRAME + (LCD_HEIGHT-x)*LCD_WIDTH + y;

    for(yc=0;yc<height;yc++)
    {
        dst=start+yc;
        for(xc=0; xc<width; xc++)
        {
            *dst=*src++;
            dst-=LCD_HEIGHT;
        }
    }
#else
    lcd_update();
#endif
#endif
}

void lcd_enable(bool state)
{
    (void)state;
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    if (!lcd_on)
        return;
#if CONFIG_ORIENTATION == SCREEN_PORTAIT
    lcd_copy_buffer_rect((fb_data *)FRAME, &lcd_framebuffer[0][0],
                         LCD_WIDTH*LCD_HEIGHT, 1);
#else
    register fb_data *dst, *src=&lcd_framebuffer[0][0];
    register unsigned int x, y;
    
    register fb_data *start=FRAME + LCD_HEIGHT*(LCD_WIDTH-1)+1;

    for(y=0; y<LCD_HEIGHT;y++)
    {
        dst=start+y;
        for(x=0; x<LCD_WIDTH; x++)
        {
            *dst=*src++;
            dst-=LCD_HEIGHT;
        }
    }
#endif
}

/* Line write helper function for lcd_yuv_blit. Write two lines of yuv420. */
extern void lcd_write_yuv420_lines(fb_data *dst,
                                   unsigned char chroma_buf[LCD_HEIGHT/2*3],
                                   unsigned char const * const src[3],
                                   int width,
                                   int stride);
/* Performance function to blit a YUV bitmap directly to the LCD */
/* For the Gigabeat - show it rotated */
/* So the LCD_WIDTH is now the height */
void lcd_yuv_blit(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    /* Caches for chroma data so it only need be recaculated every other
       line */
/*    unsigned char chroma_buf[LCD_HEIGHT/2*3];*/ /* 480 bytes */
    unsigned char const * yuv_src[3];
    off_t z;

    if (!lcd_on)
        return;

    /* Sorry, but width and height must be >= 2 or else */
    width &= ~1;
    height >>= 1;

    fb_data *dst = (fb_data*)FRAME + x * LCD_WIDTH + (LCD_WIDTH - y) - 1;

    z = stride*src_y;
    yuv_src[0] = src[0] + z + src_x;
    yuv_src[1] = src[1] + (z >> 2) + (src_x >> 1);
    yuv_src[2] = src[2] + (yuv_src[1] - src[1]);

    do
    {
/*        lcd_write_yuv420_lines(dst, chroma_buf, yuv_src, width,
                               stride);
                               */
        yuv_src[0] += stride << 1; /* Skip down two luma lines */
        yuv_src[1] += stride >> 1; /* Skip down one chroma line */
        yuv_src[2] += stride >> 1;
        dst -= 2;
    }
    while (--height > 0);
}

void lcd_set_contrast(int val) {
  (void) val;
  // TODO:
}

void lcd_set_invert_display(bool yesno) {
  (void) yesno;
  // TODO:
}

void lcd_blit(const fb_data* data, int bx, int y, int bwidth,
              int height, int stride)
{
  (void) data;
  (void) bx;
  (void) y;
  (void) bwidth;
  (void) height;
  (void) stride;
  //TODO:
}

void lcd_set_flip(bool yesno) {
  (void) yesno;
  // TODO:
}

