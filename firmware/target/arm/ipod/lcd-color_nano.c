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
#include "hwcompat.h"

/* LCD command codes for HD66789R */
#define LCD_CNTL_RAM_ADDR_SET           0x21
#define LCD_CNTL_WRITE_TO_GRAM          0x22
#define LCD_CNTL_HORIZ_RAM_ADDR_POS     0x44
#define LCD_CNTL_VERT_RAM_ADDR_POS      0x45

/*** globals ***/
int lcd_type = 1; /* 0 = "old" Color/Photo, 1 = "new" Color & Nano */

static inline void lcd_wait_write(void)
{
    while (LCD2_PORT & LCD2_BUSY_MASK);
}

static void lcd_cmd_data(unsigned cmd, unsigned data)
{
    if (lcd_type == 0) {  /* 16 bit transfers */
        lcd_wait_write();
        LCD2_PORT = LCD2_CMD_MASK | cmd;
        lcd_wait_write();
        LCD2_PORT = LCD2_CMD_MASK | data;
    } else {
        lcd_wait_write();
        LCD2_PORT = LCD2_CMD_MASK;
        LCD2_PORT = LCD2_CMD_MASK | cmd;
        lcd_wait_write();
        LCD2_PORT = LCD2_DATA_MASK | (data >> 8);
        LCD2_PORT = LCD2_DATA_MASK | (data & 0xff);
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

/* LCD init */
void lcd_init_device(void)
{  
#if CONFIG_LCD == LCD_IPODCOLOR
    if (IPOD_HW_REVISION == 0x60000) {
        lcd_type = 0;
    } else {
        int gpio_a01, gpio_a04;

        /* A01 */
        gpio_a01 = (GPIOA_INPUT_VAL & 0x2) >> 1;
        /* A04 */
        gpio_a04 = (GPIOA_INPUT_VAL & 0x10) >> 4;

        if (((gpio_a01 << 1) | gpio_a04) == 0 || ((gpio_a01 << 1) | gpio_a04) == 2) {
            lcd_type = 0;
        } else {
            lcd_type = 1;
        }
    }

    GPIOB_ENABLE |= 0x4; /* B02 enable */
    GPIOB_ENABLE |= 0x8; /* B03 enable */
    outl(inl(0x70000084) | 0x2000000, 0x70000084); /* D01 enable */
    outl(inl(0x70000080) | 0x2000000, 0x70000080); /* D01 =1 */

    DEV_EN |= 0x20000;   /* PWM enable */

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

#define CSUB_X 2
#define CSUB_Y 2

#define RYFAC (31*257)
#define GYFAC (31*257)
#define BYFAC (31*257)
#define RVFAC 11170     /* 31 * 257 *  1.402    */
#define GVFAC (-5690)  /* 31 * 257 * -0.714136 */
#define GUFAC (-2742)   /* 31 * 257 * -0.344136 */
#define BUFAC 14118     /* 31 * 257 *  1.772    */

#define ROUNDOFFS (127*257)
#define ROUNDOFFSG (63*257)

/* Performance function to blit a YUV bitmap directly to the LCD */
void lcd_yuv_blit(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    int h;
    int y0, x0, y1, x1;

    width = (width + 1) & ~1;

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
        lcd_wait_write();
        LCD2_PORT = LCD2_CMD_MASK;
        LCD2_PORT = (LCD2_CMD_MASK|LCD_CNTL_WRITE_TO_GRAM);
    }

    const int stride_div_csub_x = stride/CSUB_X;

    h=0;
    while (1) {
        /* upsampling, YUV->RGB conversion and reduction to RGB565 in one go */
        const unsigned char *ysrc = src[0] + stride * src_y + src_x;

        const int uvoffset = stride_div_csub_x * (src_y/CSUB_Y) +
                             (src_x/CSUB_X);

        const unsigned char *usrc = src[1] + uvoffset;
        const unsigned char *vsrc = src[2] + uvoffset;
        const unsigned char *row_end = ysrc + width;

        int y, u, v;
        int red1, green1, blue1;
        int red2, green2, blue2;
        unsigned rbits, gbits, bbits;

        int rc, gc, bc;
        int pixels_to_write;
        fb_data pixel1,pixel2;

        if (h==0) {
            while (!(LCD2_BLOCK_CTRL & LCD2_BLOCK_READY));
            LCD2_BLOCK_CONFIG = 0;

            if (height == 0) break;

            pixels_to_write = (width * height) * 2;
            h = height;

            /* calculate how much we can do in one go */
            if (pixels_to_write > 0x10000) {
                h = (0x10000/2) / width;
                pixels_to_write = (width * h) * 2;
            }

            height -= h;
            LCD2_BLOCK_CTRL = 0x10000080;
            LCD2_BLOCK_CONFIG = 0xc0010000 | (pixels_to_write - 1);
            LCD2_BLOCK_CTRL = 0x34000000;
        }

        do
        {
            u = *usrc++ - 128;
            v = *vsrc++ - 128;
            rc = RVFAC * v + ROUNDOFFS;
            gc = GVFAC * v + GUFAC * u + ROUNDOFFSG;
            bc = BUFAC * u + ROUNDOFFS;

            /* Pixel 1 */
            y = *ysrc++;

            red1   = RYFAC * y + rc;
            green1 = GYFAC * y + gc;
            blue1  = BYFAC * y + bc;

            /* Pixel 2 */
            y = *ysrc++;
            red2   = RYFAC * y + rc;
            green2 = GYFAC * y + gc;
            blue2  = BYFAC * y + bc;

            /* Since out of bounds errors are relatively rare, we check two
               pixels at once to see if any components are out of bounds, and
               then fix whichever is broken. This works due to high values and
               negative values both becoming larger than the cutoff when
               casted to unsigned.  And ORing them together checks all of them
               simultaneously.  */
            if (((unsigned)(red1 | green1 | blue1 |
                     red2 | green2 | blue2)) > (RYFAC*255+ROUNDOFFS)) {
                if (((unsigned)(red1 | green1 | blue1)) > 
                    (RYFAC*255+ROUNDOFFS)) {
                    if ((unsigned)red1 > (RYFAC*255+ROUNDOFFS))
                    {
                        if (red1 < 0)
                            red1 = 0;
                        else
                            red1 = (RYFAC*255+ROUNDOFFS);
                    }
                    if ((unsigned)green1 > (GYFAC*255+ROUNDOFFSG))
                    {
                        if (green1 < 0)
                            green1 = 0;
                        else
                            green1 = (GYFAC*255+ROUNDOFFSG);
                    }
                    if ((unsigned)blue1 > (BYFAC*255+ROUNDOFFS))
                    {
                        if (blue1 < 0)
                            blue1 = 0;
                        else
                            blue1 = (BYFAC*255+ROUNDOFFS);
                    }
                }

                if (((unsigned)(red2 | green2 | blue2)) > 
                    (RYFAC*255+ROUNDOFFS)) {
                    if ((unsigned)red2 > (RYFAC*255+ROUNDOFFS))
                    {
                        if (red2 < 0)
                            red2 = 0;
                        else
                            red2 = (RYFAC*255+ROUNDOFFS);
                    }
                    if ((unsigned)green2 > (GYFAC*255+ROUNDOFFSG))
                    {
                        if (green2 < 0)
                            green2 = 0;
                        else
                            green2 = (GYFAC*255+ROUNDOFFSG);
                    }
                    if ((unsigned)blue2 > (BYFAC*255+ROUNDOFFS))
                    {
                        if (blue2 < 0)
                            blue2 = 0;
                        else
                            blue2 = (BYFAC*255+ROUNDOFFS);
                    }
                }
            }
                
            rbits = red1 >> 16 ;
            gbits = green1 >> 15 ;
            bbits = blue1 >> 16 ;

            pixel1 = swap16((rbits << 11) | (gbits << 5) | bbits);

            rbits = red2 >> 16 ;
            gbits = green2 >> 15 ;
            bbits = blue2 >> 16 ;
            pixel2 = swap16((rbits << 11) | (gbits << 5) | bbits);

            while (!(LCD2_BLOCK_CTRL & LCD2_BLOCK_TXOK));

            /* output 2 pixels */
            LCD2_BLOCK_DATA = (pixel2 << 16) | pixel1;
        }
        while (ysrc < row_end);

        src_y++;
        h--;
    }

    while (!(LCD2_BLOCK_CTRL & LCD2_BLOCK_READY));
    LCD2_BLOCK_CONFIG = 0;
}


/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
    int y0, x0, y1, x1;
    int newx,newwidth;
    unsigned long *addr;

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
        lcd_wait_write();
        LCD2_PORT = LCD2_CMD_MASK;
        LCD2_PORT = (LCD2_CMD_MASK|LCD_CNTL_WRITE_TO_GRAM);
    }

    addr = (unsigned long*)&lcd_framebuffer[y][x];

    while (height > 0) {
        int c, r;
        int h, pixels_to_write;

        pixels_to_write = (width * height) * 2;
        h = height;

        /* calculate how much we can do in one go */
        if (pixels_to_write > 0x10000) {
            h = (0x10000/2) / width;
            pixels_to_write = (width * h) * 2;
        }

        LCD2_BLOCK_CTRL = 0x10000080;
        LCD2_BLOCK_CONFIG = 0xc0010000 | (pixels_to_write - 1);
        LCD2_BLOCK_CTRL = 0x34000000;

        /* for each row */
        for (r = 0; r < h; r++) {
            /* for each column */
            for (c = 0; c < width; c += 2) {
                while (!(LCD2_BLOCK_CTRL & LCD2_BLOCK_TXOK));

                /* output 2 pixels */
                LCD2_BLOCK_DATA = *addr++;
            }
            addr += (LCD_WIDTH - width)/2;
        }

        while (!(LCD2_BLOCK_CTRL & LCD2_BLOCK_READY));
        LCD2_BLOCK_CONFIG = 0;

        height -= h;
    }
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}
