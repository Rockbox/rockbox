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

/* check if number of useconds has past */
static inline bool timer_check(int clock_start, int usecs)
{
    return ((int)(USEC_TIMER - clock_start)) >= usecs;
}

/* Hardware address of LCD. Bits are:
 * 31   - set to write, poll for completion.
 * 24   - 0 for command, 1 for data 
 * 7..0 - command/data to send
 * Commands/Data are always sent in 16-bits, msb first.
 */
#define LCD_BASE        *(volatile unsigned int *)0x70008a0c 
#define LCD_BUSY_MASK   0x80000000  
#define LCD_CMD         0x80000000
#define LCD_DATA        0x81000000

/* register defines for the Renesas HD66773R */
#define R_START_OSC             0x00
#define R_DEVICE_CODE_READ      0x00
#define R_DRV_OUTPUT_CONTROL    0x01
#define R_DRV_AC_CONTROL        0x02
#define R_POWER_CONTROL1        0x03
#define R_POWER_CONTROL2        0x04
#define R_ENTRY_MODE            0x05
#define R_COMPARE_REG           0x06
#define R_DISP_CONTROL          0x07
#define R_FRAME_CYCLE_CONTROL   0x0b
#define R_POWER_CONTROL3        0x0c
#define R_POWER_CONTROL4        0x0d
#define R_POWER_CONTROL5        0x0e
#define R_GATE_SCAN_START_POS   0x0f
#define R_VERT_SCROLL_CONTROL   0x11
#define R_1ST_SCR_DRV_POS       0x14
#define R_2ND_SCR_DRV_POS       0x15
#define R_HORIZ_RAM_ADDR_POS    0x44
#define R_VERT_RAM_ADDR_POS     0x45
#define R_RAM_WRITE_DATA_MASK   0x20
#define R_RAM_ADDR_SET          0x21
#define R_WRITE_DATA_2_GRAM     0x22
#define R_RAM_READ_DATA         0x22
#define R_GAMMA_FINE_ADJ_POS1   0x30
#define R_GAMMA_FINE_ADJ_POS2   0x31
#define R_GAMMA_FINE_ADJ_POS3   0x32
#define R_GAMMA_GRAD_ADJ_POS    0x33
#define R_GAMMA_FINE_ADJ_NEG1   0x34
#define R_GAMMA_FINE_ADJ_NEG2   0x35
#define R_GAMMA_FINE_ADJ_NEG3   0x36
#define R_GAMMA_GRAD_ADJ_NEG    0x37
#define R_GAMMA_AMP_ADJ_POS     0x3a
#define R_GAMMA_AMP_ADJ_NEG     0x3b


static void lcd_wait_write(void)
{
    if ((LCD_BASE & LCD_BUSY_MASK) != 0) {
        int start = USEC_TIMER;

        do {
            if ((LCD_BASE & LCD_BUSY_MASK) == 0) break;
        } while (timer_check(start, 1000) == 0);
    }
}

/* Send command */
static void lcd_send_cmd(int v)
{
    lcd_wait_write();
    LCD_BASE =   0x00000000 | LCD_CMD;
    LCD_BASE =            v | LCD_CMD;
}

/* Send 16-bit data */
static void lcd_send_data(int v)
{
    lcd_wait_write();
    LCD_BASE = (     v & 0xff) | LCD_DATA;  /* Send MSB first */
    LCD_BASE = ((v>>8) & 0xff) | LCD_DATA;
}

/* Send two 16-bit data */
static void lcd_send_data2(int v)
{
    unsigned int vsr = v;
    lcd_send_data(vsr);
    vsr = v >> 16;
    lcd_send_data(vsr);
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
    /* H10 LCD is initialised by the bootloader */
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


/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
    int y0, x0, y1, x1;
    /*int newx,newwidth;*/

    unsigned long *addr = (unsigned long *)lcd_framebuffer;

    /* Ensure x and width are both even - so we can read 32-bit aligned 
       data from lcd_framebuffer */
    /*newx=x&~1;
    newwidth=width&~1;
    if (newx+newwidth < x+width) { newwidth+=2; }
    x=newx; width=newwidth;*/

    /* calculate the drawing region */
    y0 = x;                         /* start horiz */
    x0 = y;                         /* start vert */
    y1 = (x + width) - 1;           /* max horiz */
    x1 = (y + height) - 1;          /* max vert */


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
    lcd_send_cmd(R_HORIZ_RAM_ADDR_POS);
    lcd_send_data((y0 << 8) | y1);
    /* max vert << 8 | start vert */
    lcd_send_cmd(R_VERT_RAM_ADDR_POS);
    lcd_send_data((x0 << 8) | x1);

    /* position cursor (set AD0-AD15) */
    /* start vert << 8 | start horiz */
    lcd_send_cmd(R_RAM_ADDR_SET);
    lcd_send_data(((x0 << 8) | y0));

    /* start drawing */
    lcd_send_cmd(R_WRITE_DATA_2_GRAM);

    addr = (unsigned long*)&lcd_framebuffer[y][x];

    int c, r;

    /* for each row */
    for (r = 0; r < height; r++) {
        /* for each column */
        for (c = 0; c < width; c += 2) {
            /* output 2 pixels */
            lcd_send_data2(*(addr++));
        }

        addr += (LCD_WIDTH - width)/2;
    }
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}
