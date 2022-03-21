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

static bool lcd_has_power;
static bool lcd_kind;

static void lcd_send(bool data, uint8_t val)
{
    imx233_lcdif_pio_send(data, 1, &val);
}

void lcd_init_device(void)
{
    // determine power type
    imx233_pinctrl_acquire(3, 16, "lcd power kind");
    imx233_pinctrl_set_function(3, 16, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_enable_gpio(3, 16, false);
    udelay(10);
    lcd_has_power = !imx233_pinctrl_get_gpio(3, 16);
    if(lcd_has_power)
    {
        imx233_pinctrl_acquire(0, 27, "lcd power");
        imx233_pinctrl_set_function(0, 27, PINCTRL_FUNCTION_GPIO);
        imx233_pinctrl_enable_gpio(0, 27, true);
        imx233_pinctrl_set_gpio(0, 27, true);
    }
    // determine kind (don't acquire pin because it's a lcdif pin !)
    imx233_pinctrl_set_function(1, 0, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_enable_gpio(1, 0, false);
    udelay(10);
    lcd_kind = imx233_pinctrl_get_gpio(1, 0);

    imx233_lcdif_init();
    imx233_lcdif_reset_lcd(false);
    imx233_lcdif_reset_lcd(true);
    imx233_lcdif_setup_system_pins(16);
    imx233_lcdif_set_timings(2, 2, 2, 2);
    imx233_lcdif_set_word_length(8);
    // set mux ratio
    lcd_send(false, 0xca); lcd_send(true, 131);
    // set remap, color and depth
    lcd_send(false, 0xa0); lcd_send(true, 0x74);
    // set master contrast
    lcd_send(false, 0xc7); lcd_send(true, 0x8);
    // set V_COMH
    lcd_send(false, 0xbe); lcd_send(true, lcd_kind ? 0x1c : 0x18);
    // set color contrasts
    lcd_send(false, 0xc1); lcd_send(true, 0x7b); lcd_send(true, 0x69); lcd_send(true, lcd_kind ? 0xcf : 0x9f);
    // set timings
    lcd_send(false, 0xb1); lcd_send(true, 0x1f);
    lcd_send(false, 0xb3); lcd_send(true, 0x80);
    // set precharge voltages
    lcd_send(false, 0xbb); lcd_send(true, 0x00); lcd_send(true, 0x00); lcd_send(true, 0x00);
    // set master config
    lcd_send(false, 0xad); lcd_send(true, 0x8a);
    // set power saving mode
    lcd_send(false, 0xb0); lcd_send(true, 0x00);
    // set normal display (seem to be a SSD1338 only command, not present in SS1339 datasheet)
    lcd_send(false, 0xd1); lcd_send(true, 0x02);
    // set LUT
    lcd_send(false, 0xb8);
    static uint8_t lut[32] =
    {
        0x01, 0x15, 0x19, 0x1D, 0x21, 0x25, 0x29, 0x2D, 0x31, 0x35, 0x39, 0x3D,
        0x41, 0x45, 0x49, 0x4D, 0x51, 0x55, 0x59, 0x5D, 0x61, 0x65, 0x69, 0x6D,
        0x71, 0x75, 0x79, 0x7D, 0x81, 0x85, 0x89, 0x8D
    };
    for(int i = 0; i < 32; i++)
        lcd_send(true, lut[i]);
    // set display offset (this lcd is really wired strangely)
    lcd_send(false, 0xa2); lcd_send(true, 128);
    // normal display
    lcd_send(false, 0xa6);
    // sleep mode off
    lcd_send(false, 0xaf);

    // write ram
    lcd_send(false, 0x5c);
    imx233_lcdif_set_word_length(16);
    for(int y = 0; y < LCD_HEIGHT; y++)
        for(int x = 0; x < LCD_WIDTH; x++)
        {
            uint16_t v = 0;
            imx233_lcdif_pio_send(true, 1, &v);
        }
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

void lcd_set_contrast(int val)
{
    // NOTE: this should not interfere with lcd_update_rect
    imx233_lcdif_wait_ready();
    imx233_lcdif_set_word_length(8);
    // set contrast
    lcd_send(false, 0xc7); lcd_send(true, val);
}

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
    imx233_lcdif_set_word_length(8);
    // set column address
    lcd_send(false, 0x15); lcd_send(true, x); lcd_send(true, x + w - 1);
    // set row address
    lcd_send(false, 0x75); lcd_send(true, y); lcd_send(true, y + h - 1);
    lcd_send(false, 0x5c);
    imx233_lcdif_set_word_length(16);
    void* (*fbaddr)(int x, int y) = FB_CURRENTVP_BUFFER->get_address_fn;
    for(int yy = y; yy < y + h; yy++)
        imx233_lcdif_pio_send(true, w, fbaddr(x,yy));
}

#ifndef BOOTLOADER
bool lcd_debug_screen(void)
{
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        int button = get_action(CONTEXT_STD, HZ / 10);
        switch(button)
        {
            case ACTION_STD_NEXT:
            case ACTION_STD_PREV:
            case ACTION_STD_OK:
            case ACTION_STD_MENU:
                lcd_setfont(FONT_UI);
                return true;
            case ACTION_STD_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }

        lcd_clear_display();
        lcd_putsf(0, 0, "has power: %d", lcd_has_power);
        lcd_putsf(0, 1, "lcd kind: %d", lcd_kind);
        lcd_update();
        yield();
    }

    return true;
}
#endif
