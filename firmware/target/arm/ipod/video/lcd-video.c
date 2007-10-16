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

static inline unsigned lcd_bcm_read32(unsigned address) {
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

/*   YUV- > RGB565 conversion
 *   |R|   |1.000000 -0.000001  1.402000| |Y'|
 *   |G| = |1.000000 -0.334136 -0.714136| |Pb|
 *   |B|   |1.000000  1.772000  0.000000| |Pr|
 *   Scaled, normalized, rounded and tweaked to yield RGB 565:
 *   |R|   |74   0 101| |Y' -  16| >> 9
 *   |G| = |74 -24 -51| |Cb - 128| >> 8
 *   |B|   |74 128   0| |Cr - 128| >> 9
*/

#define RGBYFAC   74   /*  1.0      */
#define RVFAC    101   /*  1.402    */
#define GVFAC   (-51)  /* -0.714136 */
#define GUFAC   (-24)  /* -0.334136 */
#define BUFAC    128   /*  1.772    */

/* ROUNDOFFS contain constant for correct round-offs as well as
   constant parts of the conversion matrix (e.g. (Y'-16)*RGBYFAC
   -> constant part = -16*RGBYFAC). Through extraction of these
   constant parts we save at leat 4 substractions in the conversion
   loop */
#define ROUNDOFFSR (256 - 16*RGBYFAC - 128*RVFAC)
#define ROUNDOFFSG (128 - 16*RGBYFAC - 128*GVFAC - 128*GUFAC)
#define ROUNDOFFSB (256 - 16*RGBYFAC             - 128*BUFAC)

#define MAX_5BIT 0x1f
#define MAX_6BIT 0x3f

/* Performance function to blit a YUV bitmap directly to the LCD */
void lcd_yuv_blit(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height) ICODE_ATTR;
void lcd_yuv_blit(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    width = (width + 1) & ~1;

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

    const int ymax = y + height - 1;
    const int stride_div_sub_x = stride >> 1;
    unsigned char *ysrc = 0;
    unsigned char *usrc = 0;
    unsigned char *vsrc = 0;
    unsigned char *row_end = 0;
    int uvoffset;
    int yp, up, vp, rc, gc, bc; /* temporary variables */
    int red1, green1, blue1;    /* contain RGB of 1st pixel */
    int red2, green2, blue2;    /* contain RGB of 2nd pixel */

    for (; y <= ymax ; y++)
    {
        /* upsampling, YUV->RGB conversion and reduction to RGB565 in one go */
        uvoffset = stride_div_sub_x*(src_y >> 1) + (src_x >> 1);
        ysrc = src[0] + stride * src_y + src_x;
        usrc = src[1] + uvoffset;
        vsrc = src[2] + uvoffset;
        
        row_end = ysrc + width;

        do
        {
            up = *usrc++;
            vp = *vsrc++;
            rc = RVFAC * vp              + ROUNDOFFSR;
            gc = GVFAC * vp + GUFAC * up + ROUNDOFFSG;
            bc =              BUFAC * up + ROUNDOFFSB;
            
            /* Pixel 1 -> RGB565 */
            yp = *ysrc++ * RGBYFAC;
            red1   = (yp + rc) >> 9;
            green1 = (yp + gc) >> 8;
            blue1  = (yp + bc) >> 9;

            /* Pixel 2 -> RGB565 */
            yp = *ysrc++ * RGBYFAC;
            red2   = (yp + rc) >> 9;
            green2 = (yp + gc) >> 8;
            blue2  = (yp + bc) >> 9;
            
            /* Since out of bounds errors are relatively rare, we check two
               pixels at once to see if any components are out of bounds, and
               then fix whichever is broken. This works due to high values and
               negative values both being !=0 when bitmasking them.
               We first check for red and blue components (5bit range). */
            if ((red1 | blue1 | red2 | blue2) & ~MAX_5BIT)
            {
                if (red1  & ~MAX_5BIT)
                    red1  = (red1  >> 31) ? 0 : MAX_5BIT;
                if (blue1 & ~MAX_5BIT)
                    blue1 = (blue1 >> 31) ? 0 : MAX_5BIT;
                if (red2  & ~MAX_5BIT)
                    red2  = (red2  >> 31) ? 0 : MAX_5BIT;
                if (blue2 & ~MAX_5BIT)
                    blue2 = (blue2 >> 31) ? 0 : MAX_5BIT;
            }
            /* We second check for green component (6bit range) */
            if ((green1 | green2) & ~MAX_6BIT)
            {
                if (green1 & ~MAX_6BIT)
                    green1 = (green1 >> 31) ? 0 : MAX_6BIT;
                if (green2 & ~MAX_6BIT)
                    green2 = (green2 >> 31) ? 0 : MAX_6BIT;
            }
            
            /* pixel1 */
            outw((red1 << 11) | (green1 << 5) | blue1, 0x30000000);

            /* pixel2 */
            outw((red2 << 11) | (green2 << 5) | blue2, 0x30000000);
        }
        while (ysrc < row_end);

        src_y++;
    }

    /* Top-half of original lcd_bcm_finishup() function */
    outw(0x31, 0x30030000); 

    lcd_bcm_read32(0x1FC);

    finishup_needed = 1;
}
