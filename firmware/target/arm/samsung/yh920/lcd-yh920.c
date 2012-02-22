/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 by Linus Nielsen Feltzing
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

#include "system.h"
#include "kernel.h"
#include "lcd.h"

/*** definitions ***/

/* LCD command codes */
#define LCD_CNTL_POWER_CONTROL          0x25
#define LCD_CNTL_VOLTAGE_SELECT         0x2b
#define LCD_CNTL_LINE_INVERT_DRIVE      0x36
#define LCD_CNTL_GRAY_SCALE_PATTERN     0x39
#define LCD_CNTL_TEMP_GRADIENT_SELECT   0x4e
#define LCD_CNTL_OSC_FREQUENCY          0x5f
#define LCD_CNTL_ON_OFF                 0xae
#define LCD_CNTL_OSC_ON_OFF             0xaa
#define LCD_CNTL_OFF_MODE               0xbe
#define LCD_CNTL_REVERSE                0xa6
#define LCD_CNTL_ALL_LIGHTING           0xa4
#define LCD_CNTL_COMMON_OUTPUT_STATUS   0xc4
#define LCD_CNTL_COLUMN_ADDRESS_DIR     0xa0
#define LCD_CNTL_NLINE_ON_OFF           0xe4
#define LCD_CNTL_DISPLAY_MODE           0x66
#define LCD_CNTL_DUTY_SET               0x6d
#define LCD_CNTL_ELECTRONIC_VOLUME      0x81
#define LCD_CNTL_DATA_INPUT_DIR         0x84
#define LCD_CNTL_DISPLAY_START_LINE     0x8a
#define LCD_CNTL_AREA_SCROLL            0x10

#define LCD_CNTL_PAGE                   0xb1
#define LCD_CNTL_COLUMN                 0x13
#define LCD_CNTL_DATA_WRITE             0x1d

/*** shared semi-private declarations ***/
extern const unsigned char lcd_dibits[16] ICONST_ATTR;

/* lcd commands */
static void lcd_send_data(unsigned byte)
{
    while (LCD1_CONTROL & LCD1_BUSY_MASK); /* wait for LCD */
    LCD1_DATA = byte;
}

static void lcd_send_cmd(unsigned byte)
{
    while (LCD1_CONTROL & LCD1_BUSY_MASK); /* wait for LCD */
    LCD1_CMD = byte;
}

static void lcd_write_reg(unsigned cmd, unsigned data)
{
    lcd_send_cmd(cmd);
    lcd_send_data(data);
}

static void lcd_write_reg_ex(unsigned cmd, unsigned data0, unsigned data1)
{
    lcd_send_cmd(cmd);
    lcd_send_data(data0);
    lcd_send_data(data1);
}

/*** hardware configuration ***/
int lcd_default_contrast(void)
{
    return DEFAULT_CONTRAST_SETTING;
}

void lcd_set_contrast(int val)
{
    /* Keep val in acceptable hw range */
    if (val < 0)
        val = 0;
    else if (val > 127)
        val = 127;

    lcd_write_reg_ex(LCD_CNTL_ELECTRONIC_VOLUME, val, -1);
}

void lcd_set_invert_display(bool yesno)
{
    lcd_send_cmd(LCD_CNTL_REVERSE | (yesno?1:0));
}

/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
    if (yesno) 
    {
        lcd_send_cmd(LCD_CNTL_COLUMN_ADDRESS_DIR | 1);
        lcd_send_cmd(LCD_CNTL_COMMON_OUTPUT_STATUS | 0);
        lcd_write_reg_ex(LCD_CNTL_DUTY_SET, 0x20, 0);
    }
    else
    {
        lcd_send_cmd(LCD_CNTL_COLUMN_ADDRESS_DIR | 0);
        lcd_send_cmd(LCD_CNTL_COMMON_OUTPUT_STATUS | 1);
        lcd_write_reg_ex(LCD_CNTL_DUTY_SET, 0x20, 1);
    }
}

void lcd_init_device(void)
{
#if 0
    /* This is the init sequence from the yh820 OF bootloader */
    unsigned long tmp;

    DEV_INIT2 |= 0x400;

    DEV_INIT1 &= ~0x3000;
    tmp = DEV_INIT1;
    DEV_INIT1 = tmp;
    DEV_INIT2 &= ~0x400;

    LCD1_CONTROL &= ~0x4;
    udelay(15);
    LCD1_CONTROL |= 0x4;

    LCD1_CONTROL = 0x680;
    LCD1_CONTROL = 0x684;

    LCD1_CONTROL |= 0x1;

    lcd_send_cmd(LCD_CNTL_RESET);
    lcd_send_cmd(LCD_CNTL_DISCHARGE_ON_OFF | 0);
    lcd_send_cmd(LCD_CNTL_ON_OFF | 0); /* LCD OFF */
    lcd_send_cmd(LCD_CNTL_COLUMN_ADDRESS_DIR | 0);   /* Normal */
    lcd_send_cmd(LCD_CNTL_COMMON_OUTPUT_STATUS | 0); /* Normal */
    lcd_send_cmd(LCD_CNTL_REVERSE | 0); /* Reverse OFF */
    lcd_send_cmd(LCD_CNTL_ALL_LIGHTING | 0); /* Normal */
    lcd_write_reg_ex(LCD_CNTL_DUTY_SET, 0x1f, 0x00);
    lcd_send_cmd(LCD_CNTL_OFF_MODE | 1); /* OFF -> VCC on drivers */
    lcd_write_reg(LCD_CNTL_VOLTAGE_SELECT, 0x03);
    lcd_write_reg(LCD_CNTL_ELECTRONIC_VOLUME, 0x40);
    lcd_write_reg(LCD_CNTL_TEMP_GRADIENT_SELECT, 0x00);
    lcd_write_reg(LCD_CNTL_LINE_INVERT_DRIVE, 0x1f);
    lcd_send_cmd(LCD_CNTL_NLINE_ON_OFF | 1); /* N-line ON */
    lcd_write_reg(LCD_CNTL_OSC_FREQUENCY, 0x00);
    lcd_send_cmd(LCD_CNTL_OSC_ON_OFF | 1); /* Oscillator ON */
    lcd_write_reg(LCD_CNTL_STEPUP_CK_FREQ, 0x03);
    lcd_write_reg(LCD_CNTL_POWER_CONTROL, 0x1c);
    lcd_write_reg(LCD_CNTL_POWER_CONTROL, 0x1e);
    lcd_write_reg(LCD_CNTL_DISPLAY_START_LINE, 0x00);
    lcd_send_cmd(LCD_CNTL_DATA_INPUT_DIR | 0); /* Column mode */
    lcd_send_cmd(LCD_CNTL_DISPLAY_MODE, 0); /* Greyscale mode */
    lcd_write_reg(LCD_CNTL_GRAY_SCALE_PATTERN, 0x52);
    lcd_send_cmd(LCD_CNTL_PARTIAL_DISPLAY_ON_OFF | 0);
    lcd_write_reg(LCD_CNTL_POWER_CONTROL, 0x1f);
    lcd_send_cmd(LCD_CNTL_ON_OFF | 1); /* LCD ON */
#endif
}

/*** update functions ***/

/* Performance function that works with an external buffer
   note that by and bheight are in 8-pixel units! */
void lcd_blit_mono(const unsigned char *data, int x, int by, int width,
                   int bheight, int stride)
{
    const unsigned char *src, *src_end;
    unsigned char *dst_u, *dst_l;
    static unsigned char upper[LCD_WIDTH] IBSS_ATTR;
    static unsigned char lower[LCD_WIDTH] IBSS_ATTR;
    unsigned int byte;

    by *= 2;

    while (bheight--)
    {
        src = data;
        src_end = data + width;
        dst_u = upper;
        dst_l = lower;
        do
        {
            byte = *src++;
            *dst_u++ = lcd_dibits[byte & 0x0F];
            byte >>= 4;
            *dst_l++ = lcd_dibits[byte & 0x0F];
        }
        while (src < src_end);

        lcd_write_reg_ex(LCD_CNTL_PAGE, by++, -1);
        lcd_write_reg_ex(LCD_CNTL_COLUMN, x, -1);
        lcd_send_cmd(LCD_CNTL_DATA_WRITE);
        lcd_write_data(upper, width);

        lcd_write_reg_ex(LCD_CNTL_PAGE, by++, -1);
        lcd_write_reg_ex(LCD_CNTL_COLUMN, x, -1);
        lcd_send_cmd(LCD_CNTL_DATA_WRITE);
        lcd_write_data(lower, width);

        data += stride;
    }
}

/* Helper function for lcd_grey_phase_blit(). */
void lcd_grey_data(unsigned char *values, unsigned char *phases, int count);

/* Performance function that works with an external buffer
   note that by and bheight are in 4-pixel units! */
void lcd_blit_grey_phase(unsigned char *values, unsigned char *phases,
                         int x, int by, int width, int bheight, int stride)
{
    stride <<= 2; /* 4 pixels per block */
    while (bheight--)
    {
        lcd_write_reg_ex(LCD_CNTL_PAGE, by++, -1);
        lcd_write_reg_ex(LCD_CNTL_COLUMN, x, -1);
        lcd_send_cmd(LCD_CNTL_DATA_WRITE);
        lcd_grey_data(values, phases, width);
        values += stride;
        phases += stride;
    }
}

/* Update a fraction of the display. */
/* void lcd_update_rect(int, int, int, int) ICODE_ATTR; */
void lcd_update_rect(int x, int y, int width, int height)
{
    const fb_data *addr;
    int ymax;

    /* The Y coordinates have to work on even 8 pixel rows */
    ymax = (y + height-1) >> 2;
    y >>= 2;

    if(x + width > LCD_WIDTH)
        width = LCD_WIDTH - x;
    if (width <= 0)
        return; /* nothing left to do, 0 is harmful to lcd_write_data() */
    if(ymax >= LCD_FBHEIGHT)
        ymax = LCD_FBHEIGHT-1;

    /* Copy specified rectange bitmap to hardware */
    for (; y <= ymax; y++)
    {
        lcd_write_reg(LCD_CNTL_PAGE, y);
        lcd_write_reg(LCD_CNTL_COLUMN, x);

        addr = FBADDR(x,y);

        lcd_send_cmd(LCD_CNTL_DATA_WRITE);
        lcd_write_data(addr, width);
    }
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}
