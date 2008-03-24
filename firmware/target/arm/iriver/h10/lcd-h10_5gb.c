/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Barry Wardell
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

/* register defines for TL1771 */
#define R_START_OSC             0x00
#define R_DEVICE_CODE_READ      0x00
#define R_DRV_OUTPUT_CONTROL    0x01
#define R_DRV_AC_CONTROL        0x02
#define R_ENTRY_MODE            0x03
#define R_DISP_CONTROL1         0x07
#define R_DISP_CONTROL2         0x08
#define R_FRAME_CYCLE_CONTROL   0x0b
#define R_POWER_CONTROL1        0x10
#define R_POWER_CONTROL2        0x11
#define R_POWER_CONTROL3        0x12
#define R_POWER_CONTROL4        0x13
#define R_POWER_CONTROL5        0x14
#define R_RAM_ADDR_SET          0x21
#define R_WRITE_DATA_2_GRAM     0x22
#define R_GAMMA_FINE_ADJ_POS1   0x30
#define R_GAMMA_FINE_ADJ_POS2   0x31
#define R_GAMMA_FINE_ADJ_POS3   0x32
#define R_GAMMA_GRAD_ADJ_POS    0x33
#define R_GAMMA_FINE_ADJ_NEG1   0x34
#define R_GAMMA_FINE_ADJ_NEG2   0x35
#define R_GAMMA_FINE_ADJ_NEG3   0x36
#define R_GAMMA_GRAD_ADJ_NEG    0x37
#define R_POWER_CONTROL6        0x38
#define R_GATE_SCAN_START_POS   0x40
#define R_1ST_SCR_DRV_POS       0x42
#define R_2ND_SCR_DRV_POS       0x43
#define R_HORIZ_RAM_ADDR_POS    0x44
#define R_VERT_RAM_ADDR_POS     0x45

static inline void lcd_wait_write(void)
{
    while (LCD2_PORT & LCD2_BUSY_MASK);
}

/* Send command */
static inline void lcd_send_cmd(unsigned v)
{
    lcd_wait_write();
    LCD2_PORT = LCD2_CMD_MASK;
    LCD2_PORT = LCD2_CMD_MASK | v;
}

/* Send 16-bit data */
static inline void lcd_send_data(unsigned v)
{
    lcd_wait_write();
    LCD2_PORT = LCD2_DATA_MASK | (v >> 8);    /* Send MSB first */
    LCD2_PORT = LCD2_DATA_MASK | (v & 0xff);
}

/* Write value to register */
static void lcd_write_reg(int reg, int val)
{
    lcd_send_cmd(reg);
    lcd_send_data(val);
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
    CLCD_CLOCK_SRC |= 0xc0000000; /* Set LCD interface clock to PLL */
    /* H10 LCD is initialised by the bootloader */
}

/*** update functions ***/

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
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    int y0, x0, y1, x1;
    int ymax;

    width = (width + 1) & ~1;

    /* calculate the drawing region */
    x0 = x;
    x1 = x + width - 1;
    y0 = y;
    y1 = y + height - 1;

    /* max horiz << 8 | start horiz */
    lcd_write_reg(R_HORIZ_RAM_ADDR_POS, (x1 << 8) | x0);

    /* max vert << 8 | start vert */
    lcd_write_reg(R_VERT_RAM_ADDR_POS, (y1 << 8) | y0);

    /* start vert << 8 | start horiz */
    lcd_write_reg(R_RAM_ADDR_SET, (y0 << 8) | x0);

    /* start drawing */
    lcd_send_cmd(R_WRITE_DATA_2_GRAM);

    ymax = y + height - 1 ;

    const int stride_div_csub_x = stride/CSUB_X;

    for (; y <= ymax ; y++)
    {
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
            lcd_send_data((rbits << 11) | (gbits << 5) | bbits);

            rbits = red2 >> 16 ;
            gbits = green2 >> 15 ;
            bbits = blue2 >> 16 ;
            lcd_send_data((rbits << 11) | (gbits << 5) | bbits);
        }
        while (ysrc < row_end);

        src_y++;
    }
}


/* Update a fraction of the display. */
void lcd_update_rect(int x0, int y0, int width, int height)
{
    int x1, y1;
    int newx,newwidth;
    unsigned long *addr;

    /* Ensure x and width are both even - so we can read 32-bit aligned 
       data from lcd_framebuffer */
    newx=x0&~1;
    newwidth=width&~1;
    if (newx+newwidth < x0+width) { newwidth+=2; }
    x0=newx; width=newwidth;

    /* calculate the drawing region */
    y1 = (y0 + height) - 1;           /* max vert */
    x1 = (x0 + width) - 1;          /* max horiz */


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
    lcd_write_reg(R_HORIZ_RAM_ADDR_POS, (x1 << 8) | x0);

    /* max vert << 8 | start vert */
    lcd_write_reg(R_VERT_RAM_ADDR_POS, (y1 << 8) | y0);

    /* start vert << 8 | start horiz */
    lcd_write_reg(R_RAM_ADDR_SET, (y0 << 8) | x0);

    /* start drawing */
    lcd_send_cmd(R_WRITE_DATA_2_GRAM);

    addr = (unsigned long*)&lcd_framebuffer[y0][x0];

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
