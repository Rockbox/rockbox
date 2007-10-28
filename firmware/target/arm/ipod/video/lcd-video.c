/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * LCD driver for iPod Video
 * 
 * Based on code from the ipodlinux project - http://ipodlinux.org/
 * Adapted for Rockbox in December 2005
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

/* The BCM bus width is 16 bits. But since the low address bits aren't decoded
 * by the chip (the 3 BCM address bits are mapped to address bits 16..18 of the
 * PP5022), writing 32 bits (and even more, using 'stmia') at once works. */
#define BCM_DATA      (*(volatile unsigned short*)(0x30000000))
#define BCM_DATA32    (*(volatile unsigned long *)(0x30000000))
#define BCM_WR_ADDR   (*(volatile unsigned short*)(0x30010000))
#define BCM_WR_ADDR32 (*(volatile unsigned long *)(0x30010000))
#define BCM_RD_ADDR   (*(volatile unsigned short*)(0x30020000))
#define BCM_RD_ADDR32 (*(volatile unsigned long *)(0x30020000))
#define BCM_CONTROL   (*(volatile unsigned short*)(0x30030000))

#define BCM_ALT_DATA      (*(volatile unsigned short*)(0x30040000))
#define BCM_ALT_DATA32    (*(volatile unsigned long *)(0x30040000))
#define BCM_ALT_WR_ADDR   (*(volatile unsigned short*)(0x30050000))
#define BCM_ALT_WR_ADDR32 (*(volatile unsigned long *)(0x30050000))
#define BCM_ALT_RD_ADDR   (*(volatile unsigned short*)(0x30060000))
#define BCM_ALT_RD_ADDR32 (*(volatile unsigned long *)(0x30060000))
#define BCM_ALT_CONTROL   (*(volatile unsigned short*)(0x30070000))

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

static inline void lcd_bcm_write32(unsigned address, unsigned value)
{
    /* write out destination address */
    BCM_WR_ADDR32 = address;

    /* wait for it to be write ready */
    while (!(BCM_CONTROL & 0x2));

    /* write out the value */
    BCM_DATA32 = value;
}

static void lcd_bcm_setup_rect(unsigned cmd,
                               unsigned x,
                               unsigned y,
                               unsigned width,
                               unsigned height)
{
    lcd_bcm_write32(0x1F8, 0xFFFA0005);
    lcd_bcm_write32(0xE0000, cmd);
    lcd_bcm_write32(0xE0004, x);
    lcd_bcm_write32(0xE0008, y);
    lcd_bcm_write32(0xE000C, x + width - 1);
    lcd_bcm_write32(0xE0010, y + height - 1);
    lcd_bcm_write32(0xE0014, (width * height) << 1);
    lcd_bcm_write32(0xE0018, (width * height) << 1);
    lcd_bcm_write32(0xE001C, 0);
}

static inline unsigned lcd_bcm_read32(unsigned address) 
{
    while (!(BCM_RD_ADDR & 1));

    /* write out destination address */
    BCM_RD_ADDR32 = address;

    /* wait for it to be read ready */
    while (!(BCM_CONTROL & 0x10));

    /* read the value */
    return BCM_DATA32;
}

static bool finishup_needed = false;

/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
    const fb_data *addr;

    if (x + width >= LCD_WIDTH)
        width = LCD_WIDTH - x;
    if (y + height >= LCD_HEIGHT)
        height = LCD_HEIGHT - y;
        
    if ((width <= 0) || (height <= 0))
        return; /* Nothing left to do - 0 is harmful to lcd_write_data(). */
        
    addr = &lcd_framebuffer[y][x];

    if (finishup_needed)
    {
        /* Bottom-half of original lcd_bcm_finishup() function */
        unsigned int data = lcd_bcm_read32(0x1F8);
        while (data == 0xFFFA0005 || data == 0xFFFF)
        {
            /* This loop can wait for up to 14ms - so we yield() */
            yield();
            data = lcd_bcm_read32(0x1F8);
        } 
    }
    lcd_bcm_read32(0x1FC);

    lcd_bcm_setup_rect(0x34, x, y, width, height);

    /* write out destination address  */
    BCM_WR_ADDR32 = 0xE0020;

    while (!(BCM_CONTROL & 0x2));  /* wait for it to be write ready */

    do 
    {
        lcd_write_data(addr, width);
        addr += LCD_WIDTH;
    }
    while (--height > 0);

    /* Top-half of original lcd_bcm_finishup() function */
    BCM_CONTROL = 0x31;

    lcd_bcm_read32(0x1FC);

    finishup_needed = true;
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

/* Line write helper function for lcd_yuv_blit. Write two lines of yuv420. */
extern void lcd_write_yuv420_lines(unsigned char const * const src[3],
                                   int width,
                                   int stride);

/* Performance function to blit a YUV bitmap directly to the LCD */
void lcd_yuv_blit(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    unsigned char const * yuv_src[3];
    off_t z;

    /* Sorry, but width and height must be >= 2 or else */
    width &= ~1;

    z = stride * src_y;
    yuv_src[0] = src[0] + z + src_x;
    yuv_src[1] = src[1] + (z >> 2) + (src_x >> 1);
    yuv_src[2] = src[2] + (yuv_src[1] - src[1]);

    if (finishup_needed)
    {
        /* Bottom-half of original lcd_bcm_finishup() function */
        unsigned int data = lcd_bcm_read32(0x1F8);
        while (data == 0xFFFA0005 || data == 0xFFFF)
        {
            /* This loop can wait for up to 14ms - so we yield() */
            yield();
            data = lcd_bcm_read32(0x1F8);
        } 
    }

    lcd_bcm_read32(0x1FC);
    
    lcd_bcm_setup_rect(0x34, x, y, width, height);

    /* write out destination address */
    BCM_WR_ADDR32 = 0xE0020;

    while (!(BCM_CONTROL & 0x2));  /* wait for it to be write ready */

    height >>= 1;
    do
    {
        lcd_write_yuv420_lines(yuv_src, width, stride);

        yuv_src[0] += stride << 1; /* Skip down two luma lines */
        yuv_src[1] += stride >> 1; /* Skip down one chroma line */
        yuv_src[2] += stride >> 1;
    }
    while (--height > 0);

    /* Top-half of original lcd_bcm_finishup() function */
    BCM_CONTROL = 0x31;

    lcd_bcm_read32(0x1FC);

    finishup_needed = true;
}
