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
#include "system.h"
#include "backlight-target.h"
#include "lcd.h"

#include "bitmaps/rockboxlogo.h"

/* Power and display status */
static bool power_on   = false; /* Is the power turned on?   */
static bool display_on SHAREDBSS_ATTR = false; /* Is the display turned on? */
static unsigned lcd_yuv_options SHAREDBSS_ATTR = 0;
static int lcd_type;

#define LCD_DATA_OUT_GPIO GPIOH_OUTPUT_VAL
#define LCD_DATA_OUT_PIN 4

#define LCD_CLOCK_GPIO GPIOH_OUTPUT_VAL
#define LCD_CLOCK_PIN 6

#define LCD_CS_GPIO GPIOH_OUTPUT_VAL
#define LCD_CS_PIN 7

#ifdef BOOTLOADER
static void lcd_init_gpio(void)
{
    /* BOOT: 0x5CC8
       OF: 0x88284 */

    outl(inl(0x70000010) | 0xFC000000, 0x70000010);
    outl(inl(0x70000014) | 0xC300000, 0x70000014);

    GPIOE_ENABLE     = 0;
    GPIOM_ENABLE     &= ~0x3;
    GPIOF_ENABLE     = 0;
    GPIOJ_ENABLE     &= ~0x1a;
    GPIOB_ENABLE     &= ~0x8;
    GPIOH_OUTPUT_VAL |= 0x80;
    GPIOH_OUTPUT_EN  |= 0x80;
    GPIOH_ENABLE     |= 0x80;
    GPIOH_OUTPUT_VAL |= 0x40;
    GPIOH_OUTPUT_EN  |= 0x40;
    GPIOH_ENABLE     |= 0x40;
    GPIOH_OUTPUT_VAL |= 0x20;
    GPIOH_OUTPUT_EN  |= 0x20;
    GPIOH_ENABLE     |= 0x20;
    GPIOH_OUTPUT_VAL |= 0x10;
    GPIOH_OUTPUT_EN  |= 0x10;
    GPIOH_ENABLE     |= 0x10;
    GPIOD_OUTPUT_VAL |= 0x1;   /* backlight on */
    GPIOD_ENABLE     |= 0x1;
    GPIOB_OUTPUT_VAL |= 0x4;
    GPIOB_ENABLE     |= 0x4;
    GPIOB_OUTPUT_EN  |= 0x4;
    DEV_INIT2        = 0x40000000;
    GPIOG_ENABLE     |= 0x8;
    GPIOG_OUTPUT_EN  &= ~0x8;

    if (GPIOG_INPUT_VAL & 0x8)
        lcd_type = 1;
    else
        lcd_type = 0;
}
#endif

static void lcd_send_msg(unsigned char count, unsigned int data)
{
    /* BOOT: 0x645C
       OF: 0x88E90 */
    int i;

    LCD_CLOCK_GPIO |= (1 << LCD_CLOCK_PIN);
    LCD_CS_GPIO &= ~(1 << LCD_CS_PIN);

    for (i = count - 1; i >= 0; i--)
    {
        if (data & (1 << i))
        {
            LCD_DATA_OUT_GPIO &= ~(1 << LCD_DATA_OUT_PIN);
        } else {
            LCD_DATA_OUT_GPIO |= (1 << LCD_DATA_OUT_PIN);
        }
        LCD_CLOCK_GPIO &= ~(1 << LCD_CLOCK_PIN);
        udelay(1);
        LCD_CLOCK_GPIO |= (1 << LCD_CLOCK_PIN);
        udelay(2);
    }

    LCD_CS_GPIO |= (1 << LCD_CS_PIN);
    LCD_CLOCK_GPIO |= (1 << LCD_CLOCK_PIN);
    udelay(1);
}

/*
static void lcd_write_reg(unsigned int reg, unsigned int data)
{
//  OF: 0x6278 - referenced from 0x62840

//  So far this function is always called with shift = 0x0 ?
//    If so can remove and simplify 

    unsigned int cmd;
    unsigned int shift = 0;

    cmd = shift << 0x2;
    cmd |= 0x70;
    cmd = cmd << 0x10;
    cmd |= reg;
    lcd_send_msg(0x18, cmd);

    cmd = shift << 0x2;
    cmd |= 0x72;
    cmd = cmd << 0x10;
    cmd |= data;
    lcd_send_msg(0x18, cmd);

//     lcd_send_msg(0x70, reg);
//     lcd_send_msg(0x72, data);
}
*/

static void lcd_write_cmd(unsigned int cmd)
{
    lcd_send_msg(0x18, (0x700000 | cmd));
}

static void lcd_write_info(unsigned int data)
{
    lcd_send_msg(0x18, (0x720000 | data));
}

static void lcd_write_reg(unsigned int cmd, unsigned int data)
{
    lcd_write_cmd(cmd);
    lcd_write_info(data);
}

void lcd_reset(void)
{
    /* BOOT: 0x623C
       OF: 0x88BDC */

    GPIOB_OUTPUT_VAL |= 0x4;
    udelay(1000);
    GPIOB_OUTPUT_VAL &= ~0x4;
    udelay(10000);
    GPIOB_OUTPUT_VAL |= 0x4;
    udelay(50000);
}

/* Run the powerup sequence for the driver IC */
static void lcd_power_on(void)
{
    lcd_reset();
    /* OF: 0x5DC0 *
    * r2: cmd    *
    * r3: data   */
    lcd_write_reg(0xE5, 0x8000);
    lcd_write_reg(0x0, 0x1);
    lcd_write_reg(0x1, 0x100);
    lcd_write_reg(0x2, 0x700);
    lcd_write_reg(0x3, 0x1230);
    lcd_write_reg(0x4, 0x0);
    lcd_write_reg(0x8, 0x408);
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

    /* BOOT: BNE 0x5fb2 */

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
        lcd_write_reg(0x39, 0x9);
    }

    /* BOOT: 0x6066 */
    lcd_write_reg(0x3c, 0x700);
    lcd_write_reg(0x3d, 0x700);

    lcd_write_reg(0x50, 0x0);
    lcd_write_reg(0x51, 0xef);  /* 239 - LCD_WIDTH */
    lcd_write_reg(0x52, 0x0);
    lcd_write_reg(0x53, 0x13f); /* 319 - LCD_HEIGHT */

    /* BOOT: b 0x6114 */
    lcd_write_reg(0x60, 0x2700);
    lcd_write_reg(0x61, 0x1);
    lcd_write_reg(0x6a, 0x0);

    lcd_write_reg(0x80, 0x0);
    lcd_write_reg(0x81, 0x0);
    lcd_write_reg(0x82, 0x0);
    lcd_write_reg(0x83, 0x0);
    lcd_write_reg(0x84, 0x0);
    lcd_write_reg(0x85, 0x0);

    /* BOOT: 0x61A8 */
    lcd_write_reg(0x90, 0x10);
    lcd_write_reg(0x92, 0x0);
    lcd_write_reg(0x93, 0x3);
    lcd_write_reg(0x95, 0x110);
    lcd_write_reg(0x97, 0x0);
    lcd_write_reg(0x98, 0x0);

    lcd_write_reg(0xc, 0x110);
    lcd_write_reg(0x7, 0x173);
    sleep(HZ/10);

    power_on = true;
}

/* unknown 01 and 02 - sleep or enable on and off funcs? */
void unknown01(void)
{
    /* BOOT: 0x62C4
       OF: 0x88CA0 */

    lcd_write_reg(0x10, 0x17B0);
    udelay(100);
    lcd_write_reg(0x7, 0x173);
}

void unknown02(void)
{
    /* BOOT: 0x6308
       OF: 0x88D0C */

    lcd_write_reg(0x7, 0x160);
    lcd_write_reg(0x10, 0x17B1);
}

void unknown03(bool r0)
{
    /* BOOT: 0x6410
       OF: 0x88E30 */

    if (r0)
        GPIOJ_ENABLE &= ~0x2;
    else
    {
        GPIOJ_ENABLE |= 0x2;
        GPIOJ_OUTPUT_EN |= 0x2;
        GPIOJ_OUTPUT_VAL &= ~0x02;
    }
}

/* Run the display on sequence for the driver IC */
static void lcd_display_on(void)
{
    display_on = true;
}


#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
bool lcd_active(void)
{
    return display_on;
}

/* Turn off visible display operations */
static void lcd_display_off(void)
{
    display_on = false;
}
#endif

void lcd_init_device(void)
{

#ifdef BOOTLOADER /* Bother at all to do this again? */
/* Init GPIO ports */
    lcd_init_gpio();
    lcd_power_on();
    lcd_display_on();
#else

    power_on = true;
    display_on = true;

    lcd_set_invert_display(false);
    lcd_set_flip(false);
#endif
}

#if defined(HAVE_LCD_ENABLE)
void lcd_enable(bool on)
{
    (void)on;
}
#endif

#if defined(HAVE_LCD_SLEEP)
void lcd_sleep(void)
{

}
#endif

void lcd_update(void)
{
    const fb_data *addr;

    addr = FBADDR(LCD_WIDTH,LCD_HEIGHT);

    lcd_write_reg(0x20, 0x0);
    lcd_write_reg(0x21, 0x0);

    int i,j;
    for(i=0; i < LCD_HEIGHT; i++)
    {
        for(j=0; j < LCD_WIDTH; j++)
        {
            lcd_write_reg(0x22, *addr);
            addr++;
        }
    }
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

/* Blitting functions */

void lcd_yuv_set_options(unsigned options)
{
    lcd_yuv_options = options;
}

/* Line write helper function for lcd_yuv_blit. Write two lines of yuv420. */
extern void lcd_write_yuv420_lines(fb_data *dst,
                                   unsigned char const * const src[3],
                                   int width,
                                   int stride);
extern void lcd_write_yuv420_lines_odither(fb_data *dst,
                                           unsigned char const * const src[3],
                                           int width,
                                           int stride,
                                           int x_screen, /* To align dither pattern */
                                           int y_screen);

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
