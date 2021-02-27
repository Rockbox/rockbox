/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include "lcd.h"
#include "lcd-x1000.h"
#include "gpio-x1000.h"
#include "system.h"

#define CS_PIN (1 << 18)
#define RD_PIN (1 << 16)

static const unsigned fiio_lcd_cmd_on[] = {
    /* Software reset */
    LCD_INSTR_CMD,      0x01,
    LCD_INSTR_UDELAY,   120000,
    /* Sleep out */
    LCD_INSTR_CMD,      0x11,
    LCD_INSTR_UDELAY,   5000,
    /* Memory access order */
    LCD_INSTR_CMD,      0x36,
    LCD_INSTR_DAT,      0x00,
    /* Row and column address set */
    LCD_INSTR_CMD,      0x2a,
    LCD_INSTR_DAT,      0x00,
    LCD_INSTR_DAT,      0x00,
    LCD_INSTR_DAT,      (LCD_WIDTH >> 8) & 0xff,
    LCD_INSTR_DAT,      (LCD_WIDTH & 0xff),
    LCD_INSTR_CMD,      0x2b,
    LCD_INSTR_DAT,      0x00,
    LCD_INSTR_DAT,      0x00,
    LCD_INSTR_DAT,      (LCD_HEIGHT >> 8) & 0xff,
    LCD_INSTR_DAT,      (LCD_HEIGHT & 0xff),
    /* Interface pixel format */
    LCD_INSTR_CMD,      0x3a,
    LCD_INSTR_DAT,      0x05,
    /* Enable display inversion */
    LCD_INSTR_CMD,      0x21,
    /* Porch setting */
    LCD_INSTR_CMD,      0xb2,
    LCD_INSTR_DAT,      0x0c,
    LCD_INSTR_DAT,      0x0c,
    LCD_INSTR_DAT,      0x00,
    LCD_INSTR_DAT,      0x33,
    LCD_INSTR_DAT,      0x33,
    /* Gate control */
    LCD_INSTR_CMD,      0xb7,
    LCD_INSTR_DAT,      0x35,
    /* VCOM setting */
    LCD_INSTR_CMD,      0xbb,
    LCD_INSTR_DAT,      0x1f,
    /* Backlight control 5 */
    LCD_INSTR_CMD,      0xbc,
    LCD_INSTR_DAT,      0xec,
    /* Backlight control 6 */
    LCD_INSTR_CMD,      0xbd,
    LCD_INSTR_DAT,      0xfe,
    /* Voltage settings */
    LCD_INSTR_CMD,      0xc2,
    LCD_INSTR_DAT,      0x01,
    LCD_INSTR_CMD,      0xc3,
    LCD_INSTR_DAT,      0x19,
    LCD_INSTR_CMD,      0xc4,
    LCD_INSTR_DAT,      0x20,
    /* Frame rate control */
    LCD_INSTR_CMD,      0xc6,
    LCD_INSTR_DAT,      0x0f,
    /* Power control 1 */
    LCD_INSTR_CMD,      0xd0,
    LCD_INSTR_DAT,      0xa4,
    LCD_INSTR_DAT,      0xa1,
    /* d6 Unknown */
    LCD_INSTR_CMD,      0xd6,
    LCD_INSTR_DAT,      0xa1,
    /* Positive gamma correction */
    LCD_INSTR_CMD,      0xe0,
    LCD_INSTR_DAT,      0xd0,
    LCD_INSTR_DAT,      0x06,
    LCD_INSTR_DAT,      0x0c,
    LCD_INSTR_DAT,      0x0a,
    LCD_INSTR_DAT,      0x09,
    LCD_INSTR_DAT,      0x0a,
    LCD_INSTR_DAT,      0x32,
    LCD_INSTR_DAT,      0x33,
    LCD_INSTR_DAT,      0x49,
    LCD_INSTR_DAT,      0x19,
    LCD_INSTR_DAT,      0x14,
    LCD_INSTR_DAT,      0x15,
    LCD_INSTR_DAT,      0x2b,
    LCD_INSTR_DAT,      0x34,
    /* Negative gamma correction */
    LCD_INSTR_CMD,      0xe1,
    LCD_INSTR_DAT,      0xd0,
    LCD_INSTR_DAT,      0x06,
    LCD_INSTR_DAT,      0x0c,
    LCD_INSTR_DAT,      0x0a,
    LCD_INSTR_DAT,      0x09,
    LCD_INSTR_DAT,      0x11,
    LCD_INSTR_DAT,      0x37,
    LCD_INSTR_DAT,      0x33,
    LCD_INSTR_DAT,      0x49,
    LCD_INSTR_DAT,      0x19,
    LCD_INSTR_DAT,      0x14,
    LCD_INSTR_DAT,      0x15,
    LCD_INSTR_DAT,      0x2d,
    LCD_INSTR_DAT,      0x34,
    /* Display ON */
    LCD_INSTR_CMD,      0x29,
    /* Framebuffer write */
    LCD_INSTR_CMD,      0x2c,
    LCD_INSTR_END,
};

static const unsigned fiio_lcd_cmd_sleep[] = {
    /* Display OFF */
    LCD_INSTR_CMD,      0x28,
    /* Sleep IN */
    LCD_INSTR_CMD,      0x10,
    LCD_INSTR_UDELAY,   5000,
    LCD_INSTR_END,
};

static const unsigned fiio_lcd_cmd_wake[] = {
    /* Sleep OUT */
    LCD_INSTR_CMD,      0x11,
    LCD_INSTR_UDELAY,   5000,
    /* Display ON */
    LCD_INSTR_CMD,      0x29,
    /* Framebuffer write */
    LCD_INSTR_CMD,      0x2c,
    LCD_INSTR_END,
};

static unsigned fiio_lcd_cmd_set_fb_addr[] = {
    /* Row and column address set */
    LCD_INSTR_CMD,      0x2a,
    LCD_INSTR_DAT,      0x00,
    LCD_INSTR_DAT,      0x00,
    LCD_INSTR_DAT,      (LCD_WIDTH >> 8) & 0xff,
    LCD_INSTR_DAT,      (LCD_WIDTH & 0xff),
    LCD_INSTR_CMD,      0x2b,
    LCD_INSTR_DAT,      0x00,
    LCD_INSTR_DAT,      0x00,
    LCD_INSTR_DAT,      (LCD_HEIGHT >> 8) & 0xff,
    LCD_INSTR_DAT,      (LCD_HEIGHT & 0xff),
    /* Framebuffer write */
    LCD_INSTR_CMD,      0x2c,
    LCD_INSTR_END,
};

void lcd_tgt_power(bool on)
{
    if(on) {
        gpio_config(GPIO_A, 0xffff, GPIO_DEVICE(1));
        gpio_config(GPIO_B, 0x1f << 16, GPIO_DEVICE(1));
        gpio_config(GPIO_B, CS_PIN|RD_PIN, GPIO_OUTPUT(1));
        mdelay(5);
        gpio_out_level(GPIO_B, CS_PIN, 0);
        lcd_set_clock(30000000);
    } else {
        gpio_out_level(GPIO_B, CS_PIN|RD_PIN, 0);
    }
}

void lcd_tgt_ctl_on(bool on)
{
    if(on)
        lcd_exec_commands(&fiio_lcd_cmd_on[0]);
    else {
        lcd_exec_commands(&fiio_lcd_cmd_sleep[0]);
        mdelay(115); /* wait 120ms before power off */
    }
}

void lcd_tgt_ctl_sleep(bool sleep)
{
    if(sleep)
        lcd_exec_commands(&fiio_lcd_cmd_sleep[0]);
    else
        lcd_exec_commands(&fiio_lcd_cmd_wake[0]);
}

void lcd_tgt_set_fb_addr(int x1, int y1, int x2, int y2)
{
    fiio_lcd_cmd_set_fb_addr[3] = x1 >> 8;
    fiio_lcd_cmd_set_fb_addr[5] = x1 & 0xff;
    fiio_lcd_cmd_set_fb_addr[7] = x2 >> 8;
    fiio_lcd_cmd_set_fb_addr[9] = x2 & 0xff;
    fiio_lcd_cmd_set_fb_addr[13] = y1 >> 8;
    fiio_lcd_cmd_set_fb_addr[15] = y1 & 0xff;
    fiio_lcd_cmd_set_fb_addr[17] = y2 >> 8;
    fiio_lcd_cmd_set_fb_addr[19] = y2 & 0xff;
    lcd_exec_commands(&fiio_lcd_cmd_set_fb_addr[0]);
}
