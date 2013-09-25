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
#include "logf.h"

#ifdef HAVE_LCD_ENABLE
static bool lcd_on;
#endif

static void lcd_write_reg(uint8_t reg, uint8_t val)
{
#warning this implement a very simple writing scheme, might change depending on the lcd
    imx233_lcdif_set_data_swizzle(0);
    imx233_lcdif_set_word_length(8);
    imx233_lcdif_pio_send(false, 1, &reg);
    if(reg != 0x22)
        imx233_lcdif_pio_send(true, 1, &val);
}

static void lcd_init_seq(void)
{
#warning implement this
}

void lcd_init_device(void)
{
#warning check the lcd frequency
    /* the LCD seems to work at 24Mhz, so use the xtal clock with no divider */
    imx233_clkctrl_enable(CLK_PIX, false);
    imx233_clkctrl_set_div(CLK_PIX, 1);
    imx233_clkctrl_set_bypass(CLK_PIX, true); /* use XTAL */
    imx233_clkctrl_enable(CLK_PIX, true);
    imx233_lcdif_init();
#warning get correct timing
    imx233_lcdif_set_timings(2, 2, 2, 2);
    imx233_lcdif_setup_system_pins(8);
    imx233_lcdif_set_byte_packing_format(0xf); /* two pixels per 32-bit word */

    // reset device
#warning some devices have reset wired the other way around
    imx233_lcdif_reset_lcd(true);
    mdelay(10);
    imx233_lcdif_reset_lcd(false);
    mdelay(10);
    imx233_lcdif_reset_lcd(true);
    mdelay(150);

    lcd_init_seq();
#ifdef HAVE_LCD_ENABLE
    lcd_on = true;
#endif
}

#ifdef HAVE_LCD_ENABLE
bool lcd_active(void)
{
    return lcd_on;
}

static void lcd_enable_seq(bool enable)
{
#warning implement this at the end
}

void lcd_enable(bool enable)
{
    if(lcd_on == enable)
        return;

    lcd_on = enable;

    if(enable)
        imx233_lcdif_enable(true);
    lcd_enable_seq(enable);
    if(!enable)
        imx233_lcdif_enable(false);
    else
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
    /* make sure the rectangle is included in the screen */
    x = MIN(x, LCD_WIDTH);
    y = MIN(y, LCD_HEIGHT);
    w = MIN(w, LCD_WIDTH - x);
    h = MIN(h, LCD_HEIGHT - y);

    imx233_lcdif_wait_ready();
#warning write update window here using lcd_write_reg

    imx233_lcdif_wait_ready();
    imx233_lcdif_set_data_swizzle(3);
    imx233_lcdif_set_word_length(8);
#warning send data, dma is not implemented for stmp3700 at the moment so use PIO (slow but works)
}

