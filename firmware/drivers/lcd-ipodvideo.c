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

static int finishup_needed = 0;

/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height) ICODE_ATTR;
void lcd_update_rect(int x, int y, int width, int height)
{
    {
        int endy = x + width;
        /* Ensure x and width are both even - so we can read 32-bit aligned 
           data from lcd_framebuffer */
        x &= ~1;
        width &= ~1;
        if (x + width < endy) {
            width += 2;
        }
    }

    if (finishup_needed) {
        unsigned int data;
        /* Bottom-half of original lcd_bcm_finishup() function */
        do {
            /* This function takes about 14ms to execute - so we yield() */
            yield();
            data = lcd_bcm_read32(0x1F8);
        } while (data == 0xFFFA0005 || data == 0xFFFF);
    }

    lcd_bcm_read32(0x1FC);

    {
        int rect1, rect2, rect3, rect4;
        int count = (width * height) << 1;
        /* calculate the drawing region */
        rect1 = x;                         /* start horiz */
        rect2 = y;                         /* start vert */
        rect3 = (x + width) - 1;           /* max horiz */
        rect4 = (y + height) - 1;          /* max vert */

        /* setup the drawing region */
        lcd_bcm_setup_rect(0x34, rect1, rect2, rect3, rect4, count);
    }

    /* write out destination address as two 16bit values */
    outw((0xE0020 & 0xffff), 0x30010000);
    outw((0xE0020 >> 16), 0x30010000);

    /* wait for it to be write ready */
    while ((inw(0x30030000) & 0x2) == 0);

    {
        unsigned short *src = (unsigned short*)&lcd_framebuffer[y][x];
        unsigned short *end = &src[LCD_WIDTH * height];
        int line_rem = (LCD_WIDTH - width);
        while (src < end) {
            /* Duff's Device to unroll loop */
            register int count = width ;
            register int n=( count + 7 ) / 8;
            switch( count % 8 ) {
                case 0: do{ outw(*(src++), 0x30000000);
                case 7:     outw(*(src++), 0x30000000);
                case 6:     outw(*(src++), 0x30000000);
                case 5:     outw(*(src++), 0x30000000);
                case 4:     outw(*(src++), 0x30000000);
                case 3:     outw(*(src++), 0x30000000);
                case 2:     outw(*(src++), 0x30000000);
                case 1:     outw(*(src++), 0x30000000);
              } while(--n>0);
            }
            src += line_rem;
        }
    }

    /* Top-half of original lcd_bcm_finishup() function */
    outw(0x31, 0x30030000); 

    lcd_bcm_read32(0x1FC);

    finishup_needed = 1;
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

#define CSUB_X 2
#define CSUB_Y 2

#define RYFAC (31*257)
#define GYFAC (63*257)
#define BYFAC (31*257)
#define RVFAC 11170     /* 31 * 257 *  1.402    */
#define GVFAC (-11563)  /* 63 * 257 * -0.714136 */
#define GUFAC (-5572)   /* 63 * 257 * -0.344136 */
#define BUFAC 14118     /* 31 * 257 *  1.772    */

#define ROUNDOFFS (127*257)

/* Performance function to blit a YUV bitmap directly to the LCD */
void lcd_yuv_blit(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    int ymax;

    width = (width + 1) & ~1;

    if (finishup_needed) {
        unsigned int data;
        /* Bottom-half of original lcd_bcm_finishup() function */
        do {
            /* This function takes about 14ms to execute - so we yield() */
            yield();
            data = lcd_bcm_read32(0x1F8);
        } while (data == 0xFFFA0005 || data == 0xFFFF);
    }

    lcd_bcm_read32(0x1FC);

    {
        int rect1, rect2, rect3, rect4;
        int count = (width * height) << 1;
        /* calculate the drawing region */
        rect1 = x;                         /* start horiz */
        rect2 = y;                         /* start vert */
        rect3 = (x + width) - 1;           /* max horiz */
        rect4 = (y + height) - 1;          /* max vert */

        /* setup the drawing region */
        lcd_bcm_setup_rect(0x34, rect1, rect2, rect3, rect4, count);
    }

    /* write out destination address as two 16bit values */
    outw((0xE0020 & 0xffff), 0x30010000);
    outw((0xE0020 >> 16), 0x30010000);

    /* wait for it to be write ready */
    while ((inw(0x30030000) & 0x2) == 0);

    ymax = y + height - 1 ;

    for (; y <= ymax ; y++)
    {
        /* upsampling, YUV->RGB conversion and reduction to RGB565 in one go */
        const unsigned char *ysrc = src[0] + stride * src_y + src_x;
        const unsigned char *usrc = src[1] + (stride/CSUB_X) * (src_y/CSUB_Y)
                                           + (src_x/CSUB_X);
        const unsigned char *vsrc = src[2] + (stride/CSUB_X) * (src_y/CSUB_Y)
                                           + (src_x/CSUB_X);
        const unsigned char *row_end = ysrc + width;

        int y, u, v;
        int red, green, blue;
        unsigned rbits, gbits, bbits;

        int rc, gc, bc;

        do
        {
            u = *usrc++ - 128;
            v = *vsrc++ - 128;
            rc = RVFAC * v + ROUNDOFFS;
            gc = GVFAC * v + GUFAC * u + ROUNDOFFS;
            bc = BUFAC * u + ROUNDOFFS;

            /* Pixel 1 */
            y = *ysrc++;
            red   = RYFAC * y + rc;
            green = GYFAC * y + gc;
            blue  = BYFAC * y + bc;

            if ((unsigned)red > (RYFAC*255+ROUNDOFFS))
            {
                if (red < 0)
                    red = 0;
                else
                    red = (RYFAC*255+ROUNDOFFS);
            }
            if ((unsigned)green > (GYFAC*255+ROUNDOFFS))
            {
                if (green < 0)
                    green = 0;
                else
                    green = (GYFAC*255+ROUNDOFFS);
            }
            if ((unsigned)blue > (BYFAC*255+ROUNDOFFS))
            {
                if (blue < 0)
                    blue = 0;
                else
                    blue = (BYFAC*255+ROUNDOFFS);
            }
            rbits = ((unsigned)red) >> 16 ;
            gbits = ((unsigned)green) >> 16 ;
            bbits = ((unsigned)blue) >> 16 ;

            outw((rbits << 11) | (gbits << 5) | bbits, 0x30000000);

            /* Pixel 2 */
            y = *ysrc++;
            red   = RYFAC * y + rc;
            green = GYFAC * y + gc;
            blue  = BYFAC * y + bc;

            if ((unsigned)red > (RYFAC*255+ROUNDOFFS))
            {
                if (red < 0)
                    red = 0;
                else
                    red = (RYFAC*255+ROUNDOFFS);
            }
            if ((unsigned)green > (GYFAC*255+ROUNDOFFS))
            {
                if (green < 0)
                    green = 0;
                else
                    green = (GYFAC*255+ROUNDOFFS);
            }
            if ((unsigned)blue > (BYFAC*255+ROUNDOFFS))
            {
                if (blue < 0)
                    blue = 0;
                else
                    blue = (BYFAC*255+ROUNDOFFS);
            }
            rbits = ((unsigned)red) >> 16 ;
            gbits = ((unsigned)green) >> 16 ;
            bbits = ((unsigned)blue) >> 16 ;

            outw((rbits << 11) | (gbits << 5) | bbits, 0x30000000);
        }
        while (ysrc < row_end);

        src_y++;
    }

    /* Top-half of original lcd_bcm_finishup() function */
    outw(0x31, 0x30030000); 

    lcd_bcm_read32(0x1FC);

    finishup_needed = 1;
}
