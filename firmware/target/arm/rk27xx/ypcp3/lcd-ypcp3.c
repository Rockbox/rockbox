/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014, 2026 by Marcin Bukat
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

/* Samsung YP-CP3 LCD: a 240x400 panel with an SPFD5420A-compatible
 * controller on the rk27xx LCD interface's 18-bit MCU bus, used in landscape.
 *
 * The init sequence is the one worked out over hwstub
 * (utils/hwstub/tools/lua/samsungypcp3.lua). It differs from the rk27xx
 * generic panel's in the gamma curve and in VCOM_HV1. The
 * interface side - pins, LCDC, DMA, updates - is lcdif-rk27xx.c. */

#include "config.h"
#include "kernel.h"
#include "lcd.h"
#include "system.h"
#include "cpu.h"
#include "spfd5420a.h"
#include "lcdif-rk27xx.h"

static bool display_on = false;

static void lcd_sleep(bool sleep)
{
    if (sleep)
    {
        lcd_write_reg(DISPLAY_CTRL1, 0x0170);
        udelay(50);
        lcd_write_reg(DISPLAY_CTRL1, 0x0000);
        udelay(50);
        lcd_write_reg(PWR_CTRL1,     0x14B4);
    }
    else
    {
        /* out of sleep, the step-up circuits restart: let them settle,
         * then turn the display on in the steps lcd_display_init() takes.
         * Straight to 0x0173 50 us after, the panel flashed white. */
        lcd_write_reg(PWR_CTRL1,     0x14B0);
        udelay(10000);
        lcd_write_reg(DISPLAY_CTRL1, 0x0021);
        udelay(400);
        lcd_write_reg(DISPLAY_CTRL1, 0x0061);
        udelay(1000);
        lcd_write_reg(DISPLAY_CTRL1, 0x0173);
    }

    lcd_cmd(GRAM_WRITE);
}

void lcd_display_init(void)
{
    int i;

    lcd_write_reg(RESET, 0x0001);
    udelay(100000);
    lcd_write_reg(RESET, 0x0000);
    udelay(100000);

    lcd_write_reg(IF_ENDIAN,       0x0000);
    lcd_write_reg(DRIVER_OUT_CTRL, 0x0000);
    lcd_write_reg(WAVEFORM_CTRL,   0x0100);
    lcd_write_reg(ENTRY_MODE,      0x1038);     /* landscape */
    lcd_write_reg(SHAPENING_CTRL,  0x0000);
    lcd_write_reg(DISPLAY_CTRL2,   0x0808);
    lcd_write_reg(LOW_PWR_CTRL1,   0x0001);
    lcd_write_reg(LOW_PWR_CTRL2,   0x0010);
    lcd_write_reg(EXT_DISP_CTRL1,  0x0000);
    lcd_write_reg(EXT_DISP_CTRL2,  0x0000);
    lcd_write_reg(BASE_IMG_SIZE,   0x3100);
    lcd_write_reg(BASE_IMG_CTRL,   0x0001);
    lcd_write_reg(VSCROLL_CTRL,    0x0000);
    lcd_write_reg(PART1_POS,       0x0000);
    lcd_write_reg(PART1_START,     0x0000);
    lcd_write_reg(PART1_END,       0x018F);
    lcd_write_reg(PART2_POS,       0x0000);
    lcd_write_reg(PART2_START,     0x0000);
    lcd_write_reg(PART2_END,       0x0000);

    lcd_write_reg(PANEL_IF_CTRL1,  0x0011);
    udelay(100);
    lcd_write_reg(PANEL_IF_CTRL2,  0x0202);
    lcd_write_reg(PANEL_IF_CTRL3,  0x0300);
    udelay(100);
    lcd_write_reg(PANEL_IF_CTRL4,  0x021E);
    lcd_write_reg(PANEL_IF_CTRL5,  0x0202);
    lcd_write_reg(PANEL_IF_CTRL6,  0x0100);
    lcd_write_reg(FRAME_MKR_CTRL,  0x0000);
    lcd_write_reg(MDDI_CTRL,       0x0000);

    lcd_write_reg(GAMMA_CTRL1,     0x0101);
    lcd_write_reg(GAMMA_CTRL2,     0x0024);
    lcd_write_reg(GAMMA_CTRL3,     0x1321);
    lcd_write_reg(GAMMA_CTRL4,     0x2613);
    lcd_write_reg(GAMMA_CTRL5,     0x2400);
    lcd_write_reg(GAMMA_CTRL6,     0x0100);
    lcd_write_reg(GAMMA_CTRL7,     0x1704);
    lcd_write_reg(GAMMA_CTRL8,     0x0417);
    lcd_write_reg(GAMMA_CTRL9,     0x0007);
    lcd_write_reg(GAMMA_CTRL10,    0x0101);
    lcd_write_reg(GAMMA_CTRL11,    0x0F05);
    lcd_write_reg(GAMMA_CTRL12,    0x0F01);
    lcd_write_reg(GAMMA_CTRL13,    0x010F);
    lcd_write_reg(GAMMA_CTRL14,    0x050F);
    lcd_write_reg(GAMMA_CTRL15,    0x0501);
    lcd_write_reg(GAMMA_CTRL16,    0x0700);

    /* power on */
    lcd_write_reg(DISPLAY_CTRL1,   0x0001);
    lcd_write_reg(PWR_CTRL6,       0x0001);
    lcd_write_reg(PWR_CTRL7,       0x0060);
    udelay(500);
    lcd_write_reg(PWR_CTRL1,       0x16B0);
    udelay(100);
    lcd_write_reg(PWR_CTRL2,       0x0147);
    udelay(100);
    lcd_write_reg(PWR_CTRL3,       0x0117);
    udelay(100);
    lcd_write_reg(PWR_CTRL4,       0x2F00);
    udelay(500);
    lcd_write_reg(VCOM_HV2,        0x0000);
    udelay(100);
    /* VCOMH: at the reset value, fine stripes crawled over smooth
     * gradients. Over hwstub they were gone from 0x07 to 0x0C */
    lcd_write_reg(VCOM_HV1,        0x000A);
    lcd_write_reg(PWR_CTRL3,       0x01BE);
    udelay(100);

    /* the whole panel */
    lcd_write_reg(WINDOW_H_START,  0x0000);
    lcd_write_reg(WINDOW_H_END,    0x00EF);     /* 239 */
    lcd_write_reg(WINDOW_V_START,  0x0000);
    lcd_write_reg(WINDOW_V_END,    0x018F);     /* 399 */
    lcd_write_reg(GRAM_H_ADDR,     0x0000);
    lcd_write_reg(GRAM_V_ADDR,     0x0000);

    /* display on */
    lcd_write_reg(DISPLAY_CTRL1,   0x0021);
    udelay(400);
    lcd_write_reg(DISPLAY_CTRL1,   0x0061);
    udelay(1000);
    lcd_write_reg(DISPLAY_CTRL1,   0x0173);
    udelay(3000);

    /* clear */
    lcd_cmd(GRAM_WRITE);
    for (i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++)
    {
        lcd_data(0x000000);
    }

    lcd_sleep(false);
    display_on = true;
}

/* Called with the backlight (backlight-rk27xx.c). The panel is put to
 * sleep, and the LCDC's bus clock turned off; on the way back the panel
 * wakes with its GRAM kept, and is redrawn. */
void lcd_enable(bool on)
{
    if (on == display_on)
        return;

    if (on)
        lcdif_clock(true);

    lcdctrl_bypass(1);
    LCDC_CTRL |= RGB24B;
    lcd_sleep(!on);
    LCDC_CTRL &= ~RGB24B;

    display_on = on;

    if (on)
    {
        /* the backlight comes on as this returns: the panel needs a while
         * to show its picture. Over hwstub it flashed white up to 120 ms,
         * not from 150 ms; 200 ms leaves a margin */
        sleep(HZ / 5);
        send_event(LCD_EVENT_ACTIVATION, NULL);
    }
    else
        lcdif_clock(false);
}

bool lcd_active(void)
{
    return display_on;
}

void lcd_set_gram_area(int x_start, int y_start, int x_end, int y_end)
{
    lcdctrl_bypass(1);
    LCDC_CTRL |= RGB24B;

    /* the panel is portrait: its horizontal is our vertical */
    lcd_write_reg(WINDOW_H_START,  y_start);
    lcd_write_reg(WINDOW_H_END,    y_end);
    lcd_write_reg(WINDOW_V_START,  x_start);
    lcd_write_reg(WINDOW_V_END,    x_end);
    lcd_write_reg(GRAM_H_ADDR,     y_start);
    lcd_write_reg(GRAM_V_ADDR,     x_start);

    lcd_cmd(GRAM_WRITE);
    LCDC_CTRL &= ~RGB24B;
}
