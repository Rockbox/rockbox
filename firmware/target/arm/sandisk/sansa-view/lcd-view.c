/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Rockbox driver for Sansa View LCDs
 *
 * Copyright (c) 2009 Robert Keevil
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
#include <string.h>
#include "cpu.h"
#include "kernel.h"
#include "system.h"
#include "backlight-target.h"
#include "lcd.h"

static bool display_on SHAREDBSS_ATTR = true;
static int lcd_type;

#define LCD_DATA_OUT_GPIO GPIOH_OUTPUT_VAL
#define LCD_DATA_OUT_PIN 4

#define LCD_CLOCK_GPIO GPIOH_OUTPUT_VAL
#define LCD_CLOCK_PIN 6

#define LCD_CS_GPIO GPIOH_OUTPUT_VAL
#define LCD_CS_PIN 7



static void lcd_init_gpio(void)
{
    DEV_INIT1 |= 0xfc000000;
    DEV_INIT3 |= 0xc300000;

    GPIOE_ENABLE = 0;
    GPIOM_ENABLE &= ~0x3;
    GPIOF_ENABLE = 0;
    GPIOJ_ENABLE &= ~0x1a;
    // reset low
    GPIOB_ENABLE &= ~0x8;
    // SPI chip select
    GPIOH_OUTPUT_VAL |= 0x80;
    GPIOH_OUTPUT_EN |= 0x80;
    GPIOH_ENABLE |= 0x80;
    // SPI clock
    GPIOH_OUTPUT_VAL |= 0x40;
    GPIOH_OUTPUT_EN |= 0x40;
    GPIOH_ENABLE |= 0x40;
    // unk
    GPIOH_OUTPUT_VAL |= 0x20;
    GPIOH_OUTPUT_EN  |= 0x20;
    GPIOH_ENABLE |= 0x20;
    // SPI data
    GPIOH_OUTPUT_VAL |= 0x10;
    GPIOH_OUTPUT_EN |= 0x10;
    GPIOH_ENABLE |= 0x10;
    // LCD reset
    GPIOB_OUTPUT_VAL |= 0x4;
    GPIOB_ENABLE |= 0x4;
    GPIOB_OUTPUT_EN |= 0x4;
    DEV_INIT2 = 0x40000000;
    // LCD type
    GPIOG_ENABLE |= 0x8;
    GPIOG_OUTPUT_EN &= ~0x8;

    if(GPIOG_INPUT_VAL & 0x8)
        lcd_type = 0;
    else
        lcd_type = 1;
}

static void lcd_send_msg(unsigned char count, unsigned int data)
{
    GPIO_SET_BITWISE(LCD_CLOCK_GPIO, 1 << LCD_CLOCK_PIN);
    GPIO_CLEAR_BITWISE(LCD_CS_GPIO, 1 << LCD_CS_PIN);

    for(int i = count - 1; i >= 0; i--)
    {
        if(data & (1 << i))
            GPIO_SET_BITWISE(LCD_DATA_OUT_GPIO, 1 << LCD_DATA_OUT_PIN);
        else
            GPIO_CLEAR_BITWISE(LCD_DATA_OUT_GPIO, 1 << LCD_DATA_OUT_PIN);
        GPIO_CLEAR_BITWISE(LCD_CLOCK_GPIO, 1 << LCD_CLOCK_PIN);
        udelay(1);
        GPIO_SET_BITWISE(LCD_CLOCK_GPIO, 1 << LCD_CLOCK_PIN);
        udelay(2);
    }

    GPIO_SET_BITWISE(LCD_CS_GPIO, 1 << LCD_CS_PIN);
}

static void lcd_send_cmd(unsigned int cmd)
{
    lcd_send_msg(24, (0x700000 | cmd));
}

static void lcd_send_data(unsigned int data)
{
    lcd_send_msg(24, (0x720000 | data));
}

static void lcd_write_reg(unsigned int cmd, unsigned int data)
{
    lcd_send_cmd(cmd);
    lcd_send_data(data);
}

void lcd_reset(void)
{
    GPIO_SET_BITWISE(GPIOB_OUTPUT_VAL, 0x4);
    udelay(1000);
    GPIO_CLEAR_BITWISE(GPIOB_OUTPUT_VAL, 0x4);
    udelay(10000);
    GPIO_SET_BITWISE(GPIOB_OUTPUT_VAL, 0x4);
    udelay(50000);
}

/* Run the powerup sequence for the driver IC */
static void lcd_power_on(void)
{
    lcd_init_gpio();
    lcd_reset();

    lcd_write_reg(0xE5, 0x8000);
    lcd_write_reg(0x0, 0x1);
    lcd_write_reg(0x1, 0x100);
    lcd_write_reg(0x2, 0x700);
    lcd_write_reg(0x3, 0x5230);
    lcd_write_reg(0x4, 0x0);
    lcd_write_reg(0x8, 0x408); /* back porch = 8, front porch = 4 */
    lcd_write_reg(0x9, 0x0);
    lcd_write_reg(0xa, 0x0);
    lcd_write_reg(0xd, 0x0);
    lcd_write_reg(0xf, 0x2);
    lcd_write_reg(0x10, 0x0);
    lcd_write_reg(0x11, 0x0);
    lcd_write_reg(0x12, 0x0);
    lcd_write_reg(0x13, 0x0);
    sleep(HZ/5);
    lcd_write_reg(0x10, 0x17B0);
    lcd_write_reg(0x11, 0x7);
    sleep(HZ/20);
    lcd_write_reg(0x12, 0x13c);
    sleep(HZ/20);

    if (lcd_type == 0)
    {
        lcd_write_reg(0x13, 0x1700);
        lcd_write_reg(0x29, 0x10);
        sleep(HZ/10);
        lcd_write_reg(0x20, 0x0);
        lcd_write_reg(0x21, 0x0);

        lcd_write_reg(0x30, 0x7);
        lcd_write_reg(0x31, 0x403);
        lcd_write_reg(0x32, 0x400);
        lcd_write_reg(0x35, 0x3);
        lcd_write_reg(0x36, 0xF07);
        lcd_write_reg(0x37, 0x606);
        lcd_write_reg(0x38, 0x106);
        lcd_write_reg(0x39, 0x7);
    }
    else
    {
        lcd_write_reg(0x13, 0x1800);
        lcd_write_reg(0x29, 0x13);
        sleep(HZ/10);
        lcd_write_reg(0x20, 0x0);
        lcd_write_reg(0x21, 0x0);

        lcd_write_reg(0x30, 0x2);
        lcd_write_reg(0x31, 0x606);
        lcd_write_reg(0x32, 0x501);
        lcd_write_reg(0x35, 0x206);
        lcd_write_reg(0x36, 0x504);
        lcd_write_reg(0x37, 0x707);
        lcd_write_reg(0x38, 0x306);
        lcd_write_reg(0x39, 0x7);
    }

    lcd_write_reg(0x3c, 0x700);
    lcd_write_reg(0x3d, 0x700);

    lcd_write_reg(0x50, 0x0);
    lcd_write_reg(0x51, 0xef);
    lcd_write_reg(0x52, 0x0);
    lcd_write_reg(0x53, 0x13f);

    lcd_write_reg(0x60, 0x2700);
    lcd_write_reg(0x61, 0x1);
    lcd_write_reg(0x6a, 0x0);

    lcd_write_reg(0x80, 0x0);
    lcd_write_reg(0x81, 0x0);
    lcd_write_reg(0x82, 0x0);
    lcd_write_reg(0x83, 0x0);
    lcd_write_reg(0x84, 0x0);
    lcd_write_reg(0x85, 0x0);

    lcd_write_reg(0x90, 0x10);
    lcd_write_reg(0x92, 0x0);
    lcd_write_reg(0x93, 0x3);
    lcd_write_reg(0x95, 0x110);
    lcd_write_reg(0x97, 0x0);
    lcd_write_reg(0x98, 0x0);

    lcd_write_reg(0xc, 0x000); // SYSTEM mode
    lcd_write_reg(0x7, 0x173);
    sleep(HZ/10);
}

{

    lcd_power_on();

    lcd_write_reg(0x7, 0x160);
    lcd_write_reg(0x10, 0x17B1);
}

void unknown03(bool r0)
{
    if (r0)
        GPIOJ_ENABLE &= ~0x2;
    else
    {
        GPIOJ_ENABLE |= 0x2;
        GPIOJ_OUTPUT_EN |= 0x2;
        GPIOJ_OUTPUT_VAL &= ~0x02;
    }
}

void lcd_init_device(void)
{
    lcd_power_on();
}

#if defined(HAVE_LCD_ENABLE)
bool lcd_active(void)
{
    return display_on;
}

void lcd_enable(bool on)
{
    display_on = on;
    if(on)
    {
        lcd_write_reg(0x10, 0x17B0);
        udelay(100);
        lcd_write_reg(0x7, 0x173);
    }
    else
    {
        lcd_write_reg(0x7, 0x160);
        lcd_write_reg(0x10, 0x17B1);
    }
}
#endif

static inline void lcd_send_frame_byte(unsigned char data)
{
    for(int bit = 0; bit < 8; bit++)
    {
        if(data & 0x80)
            GPIO_SET_BITWISE(LCD_DATA_OUT_GPIO, 1 << LCD_DATA_OUT_PIN);
        else
            GPIO_CLEAR_BITWISE(LCD_DATA_OUT_GPIO, 1 << LCD_DATA_OUT_PIN);
        data <<= 1;
        GPIO_CLEAR_BITWISE(LCD_CLOCK_GPIO, 1 << LCD_CLOCK_PIN);
        GPIO_SET_BITWISE(LCD_CLOCK_GPIO, 1 << LCD_CLOCK_PIN);
    }
}

static void lcd_send_frame(const fb_data *addr, int count)
{
    GPIO_SET_BITWISE(LCD_CLOCK_GPIO, 1 << LCD_CLOCK_PIN);
    GPIO_CLEAR_BITWISE(LCD_CS_GPIO, 1 << LCD_CS_PIN);

    lcd_send_frame_byte(0x72);
    while(count-- > 0)
    {
        lcd_send_frame_byte(*addr >> 8);
        lcd_send_frame_byte(*addr++ & 0xff);
    }

    GPIO_SET_BITWISE(LCD_CS_GPIO, 1 << LCD_CS_PIN);
}

void lcd_update(void)
{
    if(!display_on)
        return;
    lcd_write_reg(0x20, 0x0);
    lcd_write_reg(0x21, 0x0);
    lcd_send_cmd(0x22);
    lcd_send_frame(FBADDR(0, 0), LCD_WIDTH * LCD_HEIGHT);
}

/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    lcd_update();
}


/*** hardware configuration ***/

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
    (void)yesno;
}
