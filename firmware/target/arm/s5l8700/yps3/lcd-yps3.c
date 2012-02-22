/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Bertrik Sikken
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

#include "s5l8700.h"
#include "lcd.h"

/*  LCD driver for the Samsung YP-S3

    It appears that this player can contain two display types.
    Detection of the display type is done by looking at the level of pin P0.4.
    Currently only display "type 2" has been implemented and tested.
    
    This driver could use DMA to do the screen updates, but currently writes
    the data to the LCD using the processor instead.
*/


static int lcd_type = 0;


static void lcd_delay(int delay)
{
    volatile int i;
    for (i = 0; i < delay; i++);
}

static void lcd_reset_delay(void)
{
    lcd_delay(10000);
}

static void lcd_reset(void)
{
    LCD_CON = 0xDB8;
    LCD_PHTIME = 0x22;
    LCD_RST_TIME = 0x7FFF;
    
    lcd_reset_delay();
    LCD_DRV_RST = 0;
    lcd_reset_delay();
    LCD_DRV_RST = 1;
    lcd_reset_delay();
    LCD_DRV_RST = 0;
    lcd_reset_delay();
    LCD_DRV_RST = 1;
    
    LCD_INTCON = 0;
}

static void lcd_wcmd(unsigned int cmd)
{
    while ((LCD_STATUS & 0x10) != 0);
    LCD_WCMD = cmd;
}

static void lcd_wdata(unsigned int data)
{
    while ((LCD_STATUS & 0x10) != 0);
    LCD_WDATA = data;
}

static void lcd_wcmd_data(unsigned int cmd, unsigned int data)
{
    lcd_wcmd(cmd);
    lcd_wdata(data);
}

static void lcd_init1(void)
{
    lcd_wcmd(0x11);
    lcd_delay(10000);
    
    lcd_wcmd(0xF0);
    lcd_wdata(0x5A);
    
    lcd_wcmd(0xC0);
    lcd_wdata(0x05);
    lcd_wdata(0x01);
    
    lcd_wcmd(0xC1);
    lcd_wdata(0x04);
    
    lcd_wcmd(0xC5);
    lcd_wdata(0xB0);
    
    lcd_wcmd(0xC6);
    lcd_wdata(0x0);
    
    lcd_wcmd(0xB1);
    lcd_wdata(0x02);
    lcd_wdata(0x0E);
    lcd_wdata(0x00);
    
    lcd_wcmd(0xF2);
    lcd_wdata(0x01);
    
    lcd_wcmd(0xE0);
    lcd_wdata(0x09);
    lcd_wdata(0x00);
    lcd_wdata(0x06);
    lcd_wdata(0x2E);
    lcd_wdata(0x2B);
    lcd_wdata(0x0B);
    lcd_wdata(0x1A);
    lcd_wdata(0x02);
    lcd_wdata(0x06);
    lcd_wdata(0x0C);
    lcd_wdata(0x0D);
    lcd_wdata(0x00);
    lcd_wdata(0x05);
    lcd_wdata(0x02);
    lcd_wdata(0x05);
    
    lcd_wcmd(0xE1);
    lcd_wdata(0x06);
    lcd_wdata(0x23);
    lcd_wdata(0x25);
    lcd_wdata(0x0F);
    lcd_wdata(0x0A);
    lcd_wdata(0x04);
    lcd_wdata(0x02);
    lcd_wdata(0x1A);
    lcd_wdata(0x05);
    lcd_wdata(0x03);
    lcd_wdata(0x06);
    lcd_wdata(0x01);
    lcd_wdata(0x0C);
    lcd_wdata(0x0B);
    lcd_wdata(0x05);
    lcd_wdata(0x05);
    
    lcd_wcmd(0x3A);
    lcd_wdata(0x05);
    
    lcd_wcmd(0x29);
    
    lcd_wcmd(0x2C);
}

static void lcd_init2(void)
{
    lcd_wcmd_data(0x00, 0x0001);
    lcd_delay(50000);
    
    lcd_wcmd_data(0x07, 0x0000);
    lcd_wcmd_data(0x12, 0x0000);
    lcd_delay(10000);
    
    lcd_wcmd(0);
    lcd_wcmd(0);
    lcd_wcmd(0);
    lcd_wcmd(0);

    lcd_wcmd_data(0xA4, 0x0001);
    lcd_delay(10000);

    lcd_wcmd_data(0x70, 0x1B00);
    lcd_wcmd_data(0x08, 0x030A);
    lcd_wcmd_data(0x30, 0x0000);
    lcd_wcmd_data(0x31, 0x0305);
    lcd_wcmd_data(0x32, 0x0304);
    lcd_wcmd_data(0x33, 0x0107);
    lcd_wcmd_data(0x34, 0x0304);

    lcd_wcmd_data(0x35, 0x0204);
    lcd_wcmd_data(0x36, 0x0707);
    lcd_wcmd_data(0x37, 0x0701);
    lcd_wcmd_data(0x38, 0x1B08);
    lcd_wcmd_data(0x39, 0x030F);
    lcd_wcmd_data(0x3A, 0x0E0E);

    lcd_wcmd_data(0x07, 0x0001);
    lcd_delay(50000);

    lcd_wcmd_data(0x18, 0x0001);
    lcd_wcmd_data(0x10, 0x12B0);
    lcd_wcmd_data(0x11, 0x0001);

    lcd_wcmd_data(0x12, 0x0114);
    lcd_wcmd_data(0x13, 0x8D0F);
    lcd_wcmd_data(0x12, 0x0134);
    lcd_delay(1000);
    lcd_wcmd_data(0x01, 0x0100);
    lcd_wcmd_data(0x02, 0x0700);
    lcd_wcmd_data(0x03, 0x5030);

    lcd_wcmd_data(0x04, 0x0000);
    lcd_wcmd_data(0x09, 0x0000);
    lcd_wcmd_data(0x0C, 0x0000);
    lcd_wcmd_data(0x0F, 0x0000);

    lcd_wcmd_data(0x14, 0x8000);
    lcd_wcmd_data(0x20, 0x0000);
    lcd_wcmd_data(0x21, 0x0000);
    lcd_wcmd_data(0x71, 0x0001);
    lcd_wcmd_data(0x7A, 0x0000);
    lcd_wcmd_data(0x90, 0x0000);
    lcd_wcmd_data(0x91, 0x0100);
    lcd_wcmd_data(0x92, 0x0000);
    lcd_wcmd_data(0x98, 0x0001);
    lcd_wcmd_data(0x99, 0x030C);
    lcd_wcmd_data(0x9A, 0x030C);
    
    lcd_delay(50000);
    lcd_wcmd_data(0x07, 0x0001);
    lcd_delay(30000);
    lcd_wcmd_data(0x07, 0x0021);
    
    lcd_wcmd_data(0x12, 0x1134);
    lcd_delay(10000);
    
    lcd_wcmd_data(0x07, 0x0233);
    lcd_delay(30000);
}


static void lcd_set_window1(int x, int y, int width, int height)
{
    (void)x;
    (void)width;

    lcd_wcmd(0x2A);
    lcd_wdata(0);
    lcd_wdata(y);
    lcd_wdata(0);

    lcd_wcmd(0x2B);
    lcd_wdata(0);
    lcd_wdata(y + height - 1);
    lcd_wdata(0);
}

static void lcd_set_window2(int x, int y, int width, int height)
{
    lcd_wcmd_data(0x50, x);
    lcd_wcmd_data(0x51, x + width - 1);
    lcd_wcmd_data(0x52, y);
    lcd_wcmd_data(0x53, y + height - 1);
}


static void lcd_set_position1(int x, int y)
{
    (void)x;
    (void)y;
}

static void lcd_set_position2(int x, int y)
{
    lcd_wcmd_data(0x20, x);
    lcd_wcmd_data(0x21, y);
    lcd_wcmd(0x22);
}

void lcd_init_device(void)
{
    /* enable LCD clock */
    PWRCON &= ~(1 << 18);

    /* configure LCD pins */
    PCON0 &= ~(3 << 8);
    PCON7 = (PCON7 & ~(0x000000FF)) | 0x00000033;
    PCON_ASRAM = 2;

    lcd_reset();
    
    /* detect LCD type on P0.4 */
    lcd_type = (PDAT0 & (1 << 4)) ? 1 : 2;
    
    /* initialise display */
    if (lcd_type == 1) {
        lcd_init1();
    } else {
        lcd_init2();
    }
}

void lcd_update_rect(int x, int y, int width, int height)
{
    fb_data* p;
    int h, w;
    
    if (lcd_type == 1) {
        /* TODO implement and test */
        lcd_set_window1(x, y, width, height);
        lcd_set_position1(x, y);
    
        for (h = 0; h < height; h++) {
            p = FBADDR(0,y);
            for (w = 0; w < LCD_WIDTH; w++) {
                while (LCD_STATUS & 0x10);
                LCD_WDATA = *p++;
            }
            y++;
        }
    }
    else {
        lcd_set_window2(x, y, width, height);
        lcd_set_position2(x, y);
    
        for (h = 0; h < height; h++) {
            p = FBADDR(x,y);
            for (w = 0; w < width; w++) {
                while (LCD_STATUS & 0x10);
                LCD_WDATA = *p++;
            }
            y++;
        }
    }
}

void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

