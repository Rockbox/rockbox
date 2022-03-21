/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2013 by Amaury Pouly
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
#include <sys/types.h> /* off_t */
#include <string.h>
#include "cpu.h"
#include "system.h"
#include "backlight-target.h"
#include "lcd.h"
#include "lcdif-imx233.h"
#include "clkctrl-imx233.h"
#include "pinctrl-imx233.h"
#include "dcp-imx233.h"
#include "logf.h"
#ifndef BOOTLOADER
#include "button.h"
#include "font.h"
#include "action.h"
#endif

#ifdef HAVE_LCD_ENABLE
static bool lcd_on;
#endif

static void lcd_write_reg(uint16_t reg, uint16_t value)
{
    imx233_lcdif_set_data_swizzle(3);
    imx233_lcdif_pio_send(false, 2, &reg);
    if(reg != 0x22)
        imx233_lcdif_pio_send(true, 2, &value);
}

void lcd_init_device(void)
{
    /* clock at 24MHZ */
    imx233_clkctrl_enable(CLK_PIX, false);
    imx233_clkctrl_set_div(CLK_PIX, 3);
    imx233_clkctrl_set_bypass(CLK_PIX, true); /* use XTAL */
    imx233_clkctrl_enable(CLK_PIX, true);
    imx233_lcdif_init();
    imx233_lcdif_setup_system_pins(8);
    imx233_lcdif_set_timings(2, 2, 2, 2);
    imx233_lcdif_set_word_length(8);

    lcd_write_reg(0, 1);
    lcd_write_reg(3, 0);

    lcd_write_reg(3, 0x510);
    lcd_write_reg(9, 8);
    lcd_write_reg(0xc, 0);
    lcd_write_reg(0xd, 0);
    lcd_write_reg(0xe, 0);
    lcd_write_reg(0x5b, 4);
    lcd_write_reg(0xd, 0x10);
    lcd_write_reg(9, 0);
    lcd_write_reg(3, 0x10);
    lcd_write_reg(0xd, 0x14);
    lcd_write_reg(0xe, 0x2b12);
    lcd_write_reg(1, 0x21f);
    lcd_write_reg(2, 0x700);
    lcd_write_reg(5, 0x30);
    lcd_write_reg(6, 0);
    lcd_write_reg(8, 0x202);
    lcd_write_reg(0xa, 0x3); // OF uses 0xc0003 with 3 transfers/pixels
    lcd_write_reg(0xb, 0);
    lcd_write_reg(0xf, 0);
    lcd_write_reg(0x10, 0);
    lcd_write_reg(0x11, 0);
    lcd_write_reg(0x14, 0x9f00);
    lcd_write_reg(0x15, 0x9f00);
    lcd_write_reg(0x16, 0x7f00);
    lcd_write_reg(0x17, 0x9f00);
    lcd_write_reg(0x20, 0);
    lcd_write_reg(0x21, 0);
    lcd_write_reg(0x23, 0);
    lcd_write_reg(0x24, 0);
    lcd_write_reg(0x25, 0);
    lcd_write_reg(0x26, 0);
    lcd_write_reg(0x30, 0x707);
    lcd_write_reg(0x31, 0x504);
    lcd_write_reg(0x32, 7);
    lcd_write_reg(0x33, 0x307);
    lcd_write_reg(0x34, 7);
    lcd_write_reg(0x35, 0x400);
    lcd_write_reg(0x36, 0x607);
    lcd_write_reg(0x37, 0x703);
    lcd_write_reg(0x3a, 0x1a0d);
    lcd_write_reg(0x3b, 0x1309);

    lcd_write_reg(9, 4);
    lcd_write_reg(7, 5);
    lcd_write_reg(7, 0x25);
    lcd_write_reg(7, 0x27);
    lcd_write_reg(0x5b, 0);
    lcd_write_reg(7, 0x37);
#ifdef HAVE_LCD_ENABLE
    lcd_on = true;
#endif
}

#ifdef HAVE_LCD_ENABLE
bool lcd_active(void)
{
    return lcd_on;
}

void lcd_enable(bool enable)
{
    if(lcd_on == enable)
        return;

    lcd_on = enable;
    if(enable)
        send_event(LCD_EVENT_ACTIVATION, NULL);
}
#endif

void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

void lcd_update_rect(int x, int y, int w, int h)
{
    #ifdef HAVE_LCD_ENABLE
    if(!lcd_on)
        return;
    #endif

    imx233_lcdif_wait_ready();
    lcd_write_reg(0x16, x | (x + w - 1) << 8);
    lcd_write_reg(0x17, y | (y + h - 1) << 8);
    lcd_write_reg(0x21, y * LCD_WIDTH + x);
    lcd_write_reg(0x22, 0);

    void* (*fbaddr)(int x, int y) = FB_CURRENTVP_BUFFER->get_address_fn;
    for(int yy = y; yy < y + h; yy++)
        imx233_lcdif_pio_send(true, 2 * w, fbaddr(x,yy));
}
