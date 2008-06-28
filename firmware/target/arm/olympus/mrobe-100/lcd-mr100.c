/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Mark Arigo
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

/* The m:robe 100 display has a register set that is very similar to the
   Solomon SSD1815 */

/*** definitions ***/

#define LCD_SET_LOWER_COLUMN_ADDRESS              ((char)0x00)
#define LCD_SET_HIGHER_COLUMN_ADDRESS             ((char)0x10)
#define LCD_SET_INTERNAL_REGULATOR_RESISTOR_RATIO ((char)0x20)
#define LCD_SET_POWER_CONTROL_REGISTER            ((char)0x28)
#define LCD_SET_DISPLAY_START_LINE                ((char)0x40)
#define LCD_SET_CONTRAST_CONTROL_REGISTER         ((char)0x81)
#define LCD_SET_SEGMENT_REMAP                     ((char)0xA0)
#define LCD_SET_LCD_BIAS                          ((char)0xA2)
#define LCD_SET_ENTIRE_DISPLAY_OFF                ((char)0xA4)
#define LCD_SET_ENTIRE_DISPLAY_ON                 ((char)0xA5)
#define LCD_SET_NORMAL_DISPLAY                    ((char)0xA6)
#define LCD_SET_REVERSE_DISPLAY                   ((char)0xA7)
#define LCD_SET_MULTIPLEX_RATIO                   ((char)0xA8)
#define LCD_SET_BIAS_TC_OSC                       ((char)0xA9)
#define LCD_SET_1OVER4_BIAS_RATIO                 ((char)0xAA)
#define LCD_SET_INDICATOR_OFF                     ((char)0xAC)
#define LCD_SET_INDICATOR_ON                      ((char)0xAD)
#define LCD_SET_DISPLAY_OFF                       ((char)0xAE)
#define LCD_SET_DISPLAY_ON                        ((char)0xAF)
#define LCD_SET_PAGE_ADDRESS                      ((char)0xB0)
#define LCD_SET_COM_OUTPUT_SCAN_DIRECTION         ((char)0xC0)
#define LCD_SET_TOTAL_FRAME_PHASES                ((char)0xD2)
#define LCD_SET_DISPLAY_OFFSET                    ((char)0xD3)
#define LCD_SET_READ_MODIFY_WRITE_MODE            ((char)0xE0)
#define LCD_SOFTWARE_RESET                        ((char)0xE2)
#define LCD_NOP                                   ((char)0xE3)
#define LCD_SET_END_OF_READ_MODIFY_WRITE_MODE     ((char)0xEE)

/* LCD command codes */

#define LCD_CNTL_RESET          0xe2    /* Software reset */
#define LCD_CNTL_POWER          0x2f    /* Power control */
#define LCD_CNTL_CONTRAST       0x81    /* Contrast */
#define LCD_CNTL_OUTSCAN        0xc8    /* Output scan direction */
#define LCD_CNTL_SEGREMAP       0xa1    /* Segment remap */
#define LCD_CNTL_DISPON         0xaf    /* Display on */

#define LCD_CNTL_PAGE           0xb0    /* Page address */
#define LCD_CNTL_HIGHCOL        0x10    /* Upper column address */
#define LCD_CNTL_LOWCOL         0x00    /* Lower column address */

/* send LCD command */

void lcd_write_command(int byte)
{
    while (LCD1_CONTROL & LCD1_BUSY_MASK); /* wait for LCD */
    LCD1_CMD = byte;
}

static int xoffset; /* needed for flip */

/*** hardware configuration ***/

int lcd_default_contrast(void)
{
    return DEFAULT_CONTRAST_SETTING;
}

void lcd_set_contrast(int val)
{
    lcd_write_command(LCD_CNTL_CONTRAST);
    lcd_write_command(val);
}

void lcd_set_invert_display(bool yesno)
{
    if (yesno) 
        lcd_write_command(LCD_SET_REVERSE_DISPLAY);
    else
        lcd_write_command(LCD_SET_NORMAL_DISPLAY);
}

/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
    if (!yesno)
    {
        /* normal */
        lcd_write_command(LCD_SET_COM_OUTPUT_SCAN_DIRECTION | 0xc);
        xoffset = 240 - LCD_WIDTH; /* 240 colums minus the 160 we have */
    }
    else
    {
        /* upside-down */
        lcd_write_command(LCD_SET_COM_OUTPUT_SCAN_DIRECTION);
        xoffset = 0;
    }
}

/* LCD init */
void lcd_init_device(void)
{
    int i;

    DEV_INIT1 &= ~0xfc000000;

    i = DEV_INIT1;
    DEV_INIT1 = i;

    DEV_INIT2 &= ~0x400;
    udelay(10000);

    LCD1_CONTROL &= ~0x4;
    udelay(15);

    LCD1_CONTROL |= 0x4;
    udelay(10);

    LCD1_CONTROL = 0x0094;

    /* OF just reads these */
    LCD1_CONTROL;
    inl(0x70003004);
    LCD1_CMD;
    inl(0x7000300c);

    LCD1_CONTROL |= 0x1;
    udelay(15000);

    lcd_write_command(LCD_SOFTWARE_RESET);                   /* 0xE2 */
    lcd_write_command(LCD_SET_POWER_CONTROL_REGISTER + 7);   /* 0x2F */
               /* power control register: op-amp=1, regulator=1, booster=1 */
    lcd_write_command(LCD_SET_INTERNAL_REGULATOR_RESISTOR_RATIO + 6); /* 0x26 */ 

    lcd_set_flip(false);                                    /* 0xCC */

    lcd_write_command(0xe8);

    lcd_set_contrast(lcd_default_contrast());               /* 0x80, 0x00 */

    lcd_write_command(LCD_SET_DISPLAY_START_LINE + 0);       /* 0x40 */
    lcd_write_command(LCD_SET_NORMAL_DISPLAY);               /* 0xA6 */

    lcd_write_command(0x88);

    lcd_write_command(LCD_SET_PAGE_ADDRESS);                 /* 0xB0 */
    lcd_write_command(LCD_SET_HIGHER_COLUMN_ADDRESS + 0);    /* 0x10 */
    lcd_write_command(LCD_SET_LOWER_COLUMN_ADDRESS + 0);     /* 0x00 */

    lcd_write_command(LCD_SET_DISPLAY_ON);                   /* 0xAF */
}

/*** update functions ***/

/* Performance function that works with an external buffer
   note that by and bheight are in 8-pixel units! */
void lcd_blit_mono(const unsigned char* data, int x, int by, int width,
              int bheight, int stride)
{
    int cmd1, cmd2;

    cmd1 = LCD_CNTL_HIGHCOL | (((x + xoffset) >> 4) & 0xf);
    cmd2 = LCD_CNTL_LOWCOL | ((x + xoffset) & 0xf);

    /* Copy display bitmap to hardware */
    while (bheight--)
    {
        lcd_write_command(LCD_CNTL_PAGE | (by++ & 0xff));
        lcd_write_command(cmd1);
        lcd_write_command(cmd2);

        lcd_write_data(data, width);

        data += stride;
    }
}

/* Helper function for lcd_grey_phase_blit(). */
void lcd_grey_data(unsigned char *values, unsigned char *phases, int count);

/* Performance function that works with an external buffer
   note that by and bheight are in 8-pixel units! */
void lcd_blit_grey_phase(unsigned char *values, unsigned char *phases,
                         int x, int by, int width, int bheight, int stride)
{
    int cmd1, cmd2;

    stride <<= 3;  /* 8 pixels per block */
    cmd1 = LCD_CNTL_HIGHCOL | (((x + xoffset) >> 4) & 0xf);
    cmd2 = LCD_CNTL_LOWCOL | ((x + xoffset) & 0xf);

    while (bheight--)
    {
        lcd_write_command(LCD_CNTL_PAGE | (by++ & 0xff));
        lcd_write_command(cmd1);
        lcd_write_command(cmd2);

        lcd_grey_data(values, phases, width);
        values += stride;
        phases += stride;
    }
}
 
/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void) ICODE_ATTR;
void lcd_update(void)
{
    int y, cmd1, cmd2;

    cmd1 = LCD_CNTL_HIGHCOL | (((xoffset) >> 4) & 0xf);
    cmd2 = LCD_CNTL_LOWCOL | ((xoffset) & 0xf);

    /* Copy display bitmap to hardware */
    for (y = 0; y < LCD_FBHEIGHT; y++)
    {
        lcd_write_command (LCD_CNTL_PAGE | (y & 0xf));
        lcd_write_command(cmd1);
        lcd_write_command(cmd2);

        lcd_write_data (lcd_framebuffer[y], LCD_WIDTH);
    }
}

/* Update a fraction of the display. */
void lcd_update_rect(int, int, int, int) ICODE_ATTR;
void lcd_update_rect(int x, int y, int width, int height)
{
    int ymax, cmd1, cmd2;

    /* The Y coordinates have to work on even 8 pixel rows */
    ymax = (y + height - 1) >> 3;
    y >>= 3;

    if(x + width > LCD_WIDTH)
        width = LCD_WIDTH - x;
    if (width <= 0)
        return; /* nothing left to do, 0 is harmful to lcd_write_data() */
    if(ymax >= LCD_FBHEIGHT)
        ymax = LCD_FBHEIGHT-1;


    cmd1 = LCD_CNTL_HIGHCOL | (((x + xoffset) >> 4) & 0xf);
    cmd2 = LCD_CNTL_LOWCOL | ((x + xoffset) & 0xf);

    /* Copy specified rectange bitmap to hardware */
    for (; y <= ymax; y++)
    {
        lcd_write_command(LCD_CNTL_PAGE | (y & 0xf));
        lcd_write_command(cmd1);
        lcd_write_command(cmd2);

        lcd_write_data (&lcd_framebuffer[y][x], width);
    }
}
