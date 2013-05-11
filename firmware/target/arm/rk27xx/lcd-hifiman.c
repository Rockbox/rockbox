/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C)  2011 Andrew Ryabinin
 *
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
#include "kernel.h"
#include "lcd.h"
#include "system.h"
#include "cpu.h"
#include "lcdif-rk27xx.h"
#include "lcd-target.h"

static bool display_on = false;

static void reset_lcd(void)
{
    GPIO_PCCON |= (1<<7);
    GPIO_PCDR &= ~(1<<7);
    udelay(10);
    GPIO_PCDR |= (1<<7);
    udelay(5000);
}

void lcd_v1_display_init(void)
{
    unsigned int x, y;

    /* Driving ability setting */
    lcd_write_reg(0x60, 0x00);
    lcd_write_reg(0x61, 0x06);
    lcd_write_reg(0x62, 0x00);
    lcd_write_reg(0x63, 0xC8);

    /* Gamma 2.2 Setting */
    lcd_write_reg(0x40, 0x00);
    lcd_write_reg(0x41, 0x40);
    lcd_write_reg(0x42, 0x45);
    lcd_write_reg(0x43, 0x01);
    lcd_write_reg(0x44, 0x60);
    lcd_write_reg(0x45, 0x05);
    lcd_write_reg(0x46, 0x0C);
    lcd_write_reg(0x47, 0xD1);
    lcd_write_reg(0x48, 0x05);

    lcd_write_reg(0x50, 0x75);
    lcd_write_reg(0x51, 0x01);
    lcd_write_reg(0x52, 0x67);
    lcd_write_reg(0x53, 0x14);
    lcd_write_reg(0x54, 0xF2);
    lcd_write_reg(0x55, 0x07);
    lcd_write_reg(0x56, 0x03);
    lcd_write_reg(0x57, 0x49);

    /* Power voltage setting */
    lcd_write_reg(0x1F, 0x03);
    lcd_write_reg(0x20, 0x00);
    lcd_write_reg(0x24, 0x28);
    lcd_write_reg(0x25, 0x45);

    lcd_write_reg(0x23, 0x2F);

    /* Power on setting */
    lcd_write_reg(0x18, 0x44);
    lcd_write_reg(0x21, 0x01);
    lcd_write_reg(0x01, 0x00);
    lcd_write_reg(0x1C, 0x03);
    lcd_write_reg(0x19, 0x06);
    udelay(5);

    /* Display on setting */
    lcd_write_reg(0x26, 0x84);
    udelay(40);
    lcd_write_reg(0x26, 0xB8);
    udelay(40);
    lcd_write_reg(0x26, 0xBC);
    udelay(40);

    /* Memmory access setting */
    lcd_write_reg(0x16, 0x68);
    /* Setup 16bit mode */
    lcd_write_reg(0x17, 0x05);

    /* Set GRAM area */
    lcd_write_reg(0x02, 0x00);
    lcd_write_reg(0x03, 0x00);
    lcd_write_reg(0x04, 0x00);
    lcd_write_reg(0x05, LCD_WIDTH - 1);
    lcd_write_reg(0x06, 0x00);
    lcd_write_reg(0x07, 0x00);
    lcd_write_reg(0x08, 0x00);
    lcd_write_reg(0x09, LCD_HEIGHT - 1);

    /* Start GRAM write */
    lcd_cmd(0x22);

    for (x=0; x<LCD_WIDTH; x++)
        for(y=0; y<LCD_HEIGHT; y++)
            lcd_data(0x00);

    display_on = true;
}

static void lcd_v1_enable (bool on)
{
    if (on == display_on)
        return;

    lcdctrl_bypass(1);
    LCDC_CTRL |= RGB24B;

    if (on)
    {
        lcd_write_reg(0x18, 0x44);
        lcd_write_reg(0x21, 0x01);
        lcd_write_reg(0x01, 0x00);
        lcd_write_reg(0x1C, 0x03);
        lcd_write_reg(0x19, 0x06);
        udelay(5);
        lcd_write_reg(0x26, 0x84);
        udelay(40);
        lcd_write_reg(0x26, 0xB8);
        udelay(40);
        lcd_write_reg(0x26, 0xBC);
    }
    else
    {
        lcd_write_reg(0x26, 0xB8);
        udelay(40);
        lcd_write_reg(0x19, 0x01);
        udelay(40);
        lcd_write_reg(0x26, 0xA4);
        udelay(40);
        lcd_write_reg(0x26, 0x84);
        udelay(40);
        lcd_write_reg(0x1C, 0x00);
        lcd_write_reg(0x01, 0x02);
        lcd_write_reg(0x21, 0x00);
    }
    display_on = on;

    LCDC_CTRL &= ~RGB24B;
}

static void lcd_v1_set_gram_area(int x_start, int y_start,
                                 int x_end, int y_end)
{
    lcdctrl_bypass(1);
    LCDC_CTRL |= RGB24B;
    
    lcd_write_reg(0x03, x_start);
    lcd_write_reg(0x05, x_end);
    lcd_write_reg(0x07, y_start);
    lcd_write_reg(0x09, y_end);
    
    lcd_cmd(0x22);
    LCDC_CTRL &= ~RGB24B;
}

#ifdef HM60X

enum lcd_type_t lcd_type;

static void identify_lcd(void)
{
    SCU_IOMUXB_CON &= ~(1<<2);

    if (GPIO_PCDR & (1<<4))
    {
        lcd_type = LCD_V1;
    }
    else
    {
        lcd_type = LCD_v2;
    }
}

static void lcd_v2_display_init(void)
{
    unsigned int x, y;

    lcd_write_reg(0xD0, 0x0003);
    lcd_write_reg(0xEB, 0x0B00);
    lcd_write_reg(0xEC, 0x00CF);
    lcd_write_reg(0xC7, 0x030F);

    lcd_write_reg(0x01, 0x001C);
    lcd_write_reg(0x02, 0x0100);
    lcd_write_reg(0x03, 0x1038);
    lcd_write_reg(0x07, 0x0000);
    lcd_write_reg(0x08, 0x0808);
    lcd_write_reg(0x0F, 0x0901);
    lcd_write_reg(0x10, 0x0000);
    lcd_write_reg(0x11, 0x1B41);
    lcd_write_reg(0x12, 0x2010);
    lcd_write_reg(0x13, 0x0009);
    lcd_write_reg(0x14, 0x4C65);

    lcd_write_reg(0x30, 0x0000);
    lcd_write_reg(0x31, 0x00DB);
    lcd_write_reg(0x32, 0x0000);
    lcd_write_reg(0x33, 0x0000);
    lcd_write_reg(0x34, 0x00DB);
    lcd_write_reg(0x35, 0x0000);
    lcd_write_reg(0x36, 0x00AF);
    lcd_write_reg(0x37, 0x0000);
    lcd_write_reg(0x38, 0x00DB);
    lcd_write_reg(0x39, 0x0000);

    lcd_write_reg(0x50, 0x0000);
    lcd_write_reg(0x51, 0x0705);
    lcd_write_reg(0x52, 0x0C0A);
    lcd_write_reg(0x53, 0x0401);
    lcd_write_reg(0x54, 0x040C);
    lcd_write_reg(0x55, 0x0608);
    lcd_write_reg(0x56, 0x0000);
    lcd_write_reg(0x57, 0x0104);
    lcd_write_reg(0x58, 0x0E06);
    lcd_write_reg(0x59, 0x060E);

    lcd_write_reg(0x20, 0x0000);
    lcd_write_reg(0x21, 0x0000);

    lcd_write_reg(0x07, 0x1017);

    lcd_cmd(0x22);

    for (x=0; x<LCD_WIDTH; x++)
        for(y=0; y<LCD_HEIGHT; y++)
            lcd_data(0x00);

    display_on = true;
}

static void lcd_v2_enable (bool on)
{
    if (on == display_on)
        return;

    lcdctrl_bypass(1);
    LCDC_CTRL |= RGB24B;

    if (on)
    {
        lcd_write_reg(0x10, 0x0000);
        lcd_write_reg(0x11, 0x1B41);
        udelay(50000);
        lcd_write_reg(0x07, 0x1017);
        udelay(50000);
    }
    else
    {
        lcd_write_reg(0x07, 0x0000);
        udelay(50000);
        lcd_write_reg(0x11, 0x0001);
        udelay(50000);
        lcd_write_reg(0x10, 0x0001);
    }
    display_on = on;

    LCDC_CTRL &= ~RGB24B;

}

static void lcd_v2_set_gram_area(int x_start, int y_start,
                                 int x_end, int y_end)
{
    lcdctrl_bypass(1);
    LCDC_CTRL |= RGB24B;

    lcd_write_reg(0x36, y_end);
    lcd_write_reg(0x37, y_start);
    lcd_write_reg(0x38, x_end);
    lcd_write_reg(0x39, x_start);

    /* set GRAM address */
    lcd_write_reg(0x20, y_start);
    lcd_write_reg(0x21, x_start);

    lcd_cmd(0x22);
    LCDC_CTRL &= ~RGB24B;
}

void lcd_display_init(void)
{
    reset_lcd();
    identify_lcd();
    if (lcd_type == LCD_V1)
        lcd_v1_display_init();
    else
        lcd_v2_display_init();
}

void lcd_enable (bool on)
{
    if (lcd_type == LCD_V1)
        lcd_v1_enable(on);
    else
        lcd_v2_enable(on);
}

void lcd_set_gram_area(int x_start, int y_start,
                       int x_end, int y_end)
{
   if (lcd_type == LCD_V1)
        lcd_v1_set_gram_area(x_start, y_start, x_end, y_end);
    else
        lcd_v2_set_gram_area(x_start, y_start, x_end, y_end);
}

#else /* HM801 */

void lcd_display_init(void)
{
    reset_lcd();
    lcd_v1_display_init();
}

void lcd_enable (bool on)
{
    lcd_v1_enable(on);
}

void lcd_set_gram_area(int x_start, int y_start,
                       int x_end, int y_end)
{
    lcd_v1_set_gram_area(x_start, y_start, x_end, y_end);
}
#endif

bool lcd_active()
{
    return display_on;
}

/* Blit a YUV bitmap directly to the LCD
 * provided by generic fallback in lcd-16bit-common.c
 */
#if 0
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
#endif
