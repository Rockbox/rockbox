/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Dave Chapman
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

#include "hwcompat.h"
#include "kernel.h"
#include "lcd.h"
#include "system.h"
#include "cpu.h"


/* The Nano 2G has two different LCD types.  What we call "type 0"
   appears to be similar to the ILI9320 and "type 1" is similar to the
   LDS176.
*/

/* LCD type 0 register defines */

#define R_ENTRY_MODE              0x03
#define R_HORIZ_GRAM_ADDR_SET     0x20
#define R_VERT_GRAM_ADDR_SET      0x21
#define R_WRITE_DATA_TO_GRAM      0x22
#define R_HORIZ_ADDR_START_POS    0x50
#define R_HORIZ_ADDR_END_POS      0x51
#define R_VERT_ADDR_START_POS     0x52
#define R_VERT_ADDR_END_POS       0x53


/* LCD type 1 register defines */

#define R_COLUMN_ADDR_SET         0x2a
#define R_ROW_ADDR_SET            0x2b
#define R_MEMORY_WRITE            0x2c


/** globals **/

static int lcd_type;
static int xoffset; /* needed for flip */

/** hardware access functions */

static inline void s5l_lcd_write_cmd_data(int cmd, int data)
{
    while (LCD_STATUS & 0x10);
    LCD_WCMD = cmd >> 8;
    while (LCD_STATUS & 0x10);
    LCD_WCMD = cmd & 0xff;

    while (LCD_STATUS & 0x10);
    LCD_WDATA = data >> 8;
    while (LCD_STATUS & 0x10);
    LCD_WDATA = data & 0xff;
}

static inline void s5l_lcd_write_cmd(unsigned short cmd)
{
    while (LCD_STATUS & 0x10);
    LCD_WCMD = cmd;
}

static inline void s5l_lcd_write_data(int data)
{
    while (LCD_STATUS & 0x10);
    LCD_WDATA = data >> 8;
    while (LCD_STATUS & 0x10);
    LCD_WDATA = data & 0xff;
}

/*** hardware configuration ***/

int lcd_default_contrast(void)
{
    return 0x1f;
}

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
    /* TODO: flip mode isn't working.  The commands in the else part of
       this function are how the original firmware inits the LCD */

    if (yesno)
    {
        xoffset = 132 - LCD_WIDTH; /* 132 colums minus the 128 we have */
    }
    else 
    {
        xoffset = 0;
    }
}



void lcd_off(void)
{
}

void lcd_on(void)
{
}

/* LCD init */
void lcd_init_device(void)
{
    /* Detect lcd type */

    PCON13 &= ~0xf;    /* Set pin 0 to input */
    PCON14 &= ~0xf0;   /* Set pin 1 to input */

    if (((PDAT13 & 1) == 0) && ((PDAT14 & 2) == 2))
        lcd_type = 0;  /* Similar to ILI9320 - aka "type 2" */
    else
        lcd_type = 1;  /* Similar to LDS176 - aka "type 7" */

    /* Now init according to lcd type */
    if (lcd_type == 0) {
        /* TODO */

        /* Entry Mode: AM=0, I/D1=1, I/D0=1, ORG=0, HWM=1, BGR=1 */
        s5l_lcd_write_cmd_data(R_ENTRY_MODE, 0x1230); 
    } else {
        /* TODO */
    }
}


/*** Update functions ***/

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void) ICODE_ATTR;
void lcd_update(void)
{
    int x,y;
    fb_data* p = &lcd_framebuffer[0][0];
    fb_data pixel;

    if (lcd_type==0) {
        s5l_lcd_write_cmd_data(R_HORIZ_ADDR_START_POS, 0);
        s5l_lcd_write_cmd_data(R_HORIZ_ADDR_END_POS,   LCD_WIDTH-1);
        s5l_lcd_write_cmd_data(R_VERT_ADDR_START_POS,  0);
        s5l_lcd_write_cmd_data(R_VERT_ADDR_END_POS,    LCD_HEIGHT-1);

        s5l_lcd_write_cmd_data(R_HORIZ_GRAM_ADDR_SET,  0);
        s5l_lcd_write_cmd_data(R_VERT_GRAM_ADDR_SET,   0);

        s5l_lcd_write_cmd(0);
        s5l_lcd_write_cmd(R_WRITE_DATA_TO_GRAM);
    } else {
        s5l_lcd_write_cmd(R_COLUMN_ADDR_SET);
        s5l_lcd_write_data(0);            /* Start column */
        s5l_lcd_write_data(LCD_WIDTH-1);  /* End column */

        s5l_lcd_write_cmd(R_ROW_ADDR_SET);
        s5l_lcd_write_data(0);            /* Start row */
        s5l_lcd_write_data(LCD_HEIGHT-1); /* End row */

        s5l_lcd_write_cmd(R_MEMORY_WRITE);
    }


    /* Copy display bitmap to hardware */
    for (y = 0; y < LCD_HEIGHT; y++) {
        for (x = 0; x < LCD_WIDTH; x++) {
            pixel = *(p++);

            while (LCD_STATUS & 0x10);
            LCD_WDATA = (pixel & 0xff00) >> 8;
            while (LCD_STATUS & 0x10);
            LCD_WDATA = pixel & 0xff;
        }
    }
}

/* Update a fraction of the display. */
void lcd_update_rect(int, int, int, int) ICODE_ATTR;
void lcd_update_rect(int x, int y, int width, int height)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;

    /* TODO.  For now, just do a full-screen update */
    lcd_update();
}

/* Performance function to blit a YUV bitmap directly to the LCD */
void lcd_blit_yuv(unsigned char * const src[3],
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
