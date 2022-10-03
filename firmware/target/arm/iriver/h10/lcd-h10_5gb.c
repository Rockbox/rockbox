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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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
#ifndef BOOTLOADER
    /* The OF won't boot if this is done in the bootloader - ideally we should 
       tweak the lcd controller speed settings but this will do for now */
    CLCD_CLOCK_SRC |= 0xc0000000; /* Set LCD interface clock to PLL */
#endif
    /* H10 LCD is initialised by the bootloader */
}

/*** update functions ***/

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

    addr = (unsigned long*)FBADDR(x0,y0);

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
