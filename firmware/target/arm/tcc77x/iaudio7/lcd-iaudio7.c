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

/*
  Thanks Hein-Pieter van Braam for initial work.

  Mostly based on lcd-h300.c, adapted for the iaudio 7 by Vitja Makarov
 */

#include <config.h>

#include <kernel.h>
#include <cpu.h>
#include <lcd.h>
#include <system-target.h>
#include <panic.h>

#include "hd66789r.h"

static bool display_on = false; /* is the display turned on? */

static inline void lcd_write_reg(int reg, int data)
{
    GPIOA &= ~0x400;
    outw(0, 0x50010000);
    outw(reg << 1, 0x50010000);
    GPIOA |= 0x400;

    outw((data & 0xff00) >> 7, 0x50010008);
    outw((data << 24) >> 23, 0x50010008);
}

static void lcd_write_cmd(int reg)
{
    GPIOA &= ~0x400;
    outw(0, 0x50010000);
    outw(reg << 1, 0x50010000);
    GPIOA |= 0x400;
}

/* Do what OF do */
static void lcd_delay(int x)
{
    int i;

    x *= 0xc35;
    for (i = 0; i < x * 8; i++) {
    }
}


static void _display_on(void)
{
    GPIOA_DIR |= 0x8000 | 0x400;
    GPIOA |= 0x8000;

    /* power setup */
    lcd_write_reg(R_START_OSC, 0x0001);
    lcd_delay(0xf);
    lcd_write_reg(R_DISP_CONTROL1, 0x000);
    lcd_delay(0xa);
    lcd_write_reg(R_POWER_CONTROL2, 0x0002);
    lcd_write_reg(R_POWER_CONTROL3, 0x000a);
    lcd_write_reg(R_POWER_CONTROL4, 0xc5a);
    lcd_write_reg(R_POWER_CONTROL1, 0x0004);
    lcd_write_reg(R_POWER_CONTROL1, 0x0134);
    lcd_write_reg(R_POWER_CONTROL2, 0x0111);
    lcd_write_reg(R_POWER_CONTROL3, 0x001c);
    lcd_delay(0x28);
    lcd_write_reg(R_POWER_CONTROL4, 0x2c40);
    lcd_write_reg(R_POWER_CONTROL1, 0x0510);
    lcd_delay(0x3c);

    /* lcd init 2 */
    lcd_write_reg(R_DRV_OUTPUT_CONTROL, 0x0113);
    lcd_write_reg(R_DRV_WAVEFORM_CONTROL, 0x0700);
    lcd_write_reg(R_ENTRY_MODE, 0x1038);
    lcd_write_reg(R_DISP_CONTROL2, 0x0508);     // 0x3c8, TMM
    lcd_write_reg(R_DISP_CONTROL3, 0x0000);
    lcd_write_reg(R_FRAME_CYCLE_CONTROL, 0x0003);
    lcd_write_reg(R_RAM_ADDR_SET, 0x0000);
    lcd_write_reg(R_GAMMA_FINE_ADJ_POS1, 0x0406);
    lcd_write_reg(R_GAMMA_FINE_ADJ_POS2, 0x0303);
    lcd_write_reg(R_GAMMA_FINE_ADJ_POS3, 0x0000);
    lcd_write_reg(R_GAMMA_GRAD_ADJ_POS, 0x0305);
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG1, 0x0404);
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG2, 0x0000);
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG3, 0x0000);
    lcd_write_reg(R_GAMMA_GRAD_ADJ_NEG, 0x0503);
    lcd_write_reg(R_GAMMA_AMP_ADJ_RES_POS, 0x1d05);
    lcd_write_reg(R_GAMMA_AMP_AVG_ADJ_RES_NEG, 0x1d05);
    lcd_write_reg(R_VERT_SCROLL_CONTROL, 0x0000);
    lcd_write_reg(R_1ST_SCR_DRV_POS, 0x9f00);
    lcd_write_reg(R_2ND_SCR_DRV_POS, 0x9f00);
    lcd_write_reg(R_HORIZ_RAM_ADDR_POS, 0x7f00);
    lcd_write_reg(R_VERT_RAM_ADDR_POS, 0x9f00);

    /* lcd init 3 */
    lcd_write_reg(R_POWER_CONTROL1, 0x4510);
    lcd_write_reg(R_DISP_CONTROL1, 0x0005);
    lcd_delay(0x28);
    lcd_write_reg(R_DISP_CONTROL1, 0x0025);
    lcd_write_reg(R_DISP_CONTROL1, 0x0027);
    lcd_delay(0x28);
    lcd_write_reg(R_DISP_CONTROL1, 0x0037);

    display_on = true;
}

void lcd_init_device(void)
{
    /* Configure external memory banks */
    CSCFG1 = 0x3d500023;

    /* may be reset */
    GPIOA |= 0x8000;

    _display_on();
}

void lcd_enable(bool on)
{
    if (display_on == on)
        return;

    if (on) {
        _display_on();
      lcd_call_enable_hook();
    } else {
        /** Off sequence according to datasheet, p. 130 **/
        lcd_write_reg(R_FRAME_CYCLE_CONTROL, 0x0002);   /* EQ=0, 18 clks/line */
        lcd_write_reg(R_DISP_CONTROL1, 0x0036); /* GON=1, DTE=1, REV=1, D1-0=10 */
        sleep(2);

        lcd_write_reg(R_DISP_CONTROL1, 0x0026); /* GON=1, DTE=0, REV=1, D1-0=10 */
        sleep(2);

        lcd_write_reg(R_DISP_CONTROL1, 0x0000); /* GON=0, DTE=0, D1-0=00 */

        lcd_write_reg(R_POWER_CONTROL1, 0x0000);        /* SAP2-0=000, AP2-0=000 */
        lcd_write_reg(R_POWER_CONTROL3, 0x0000);        /* PON=0 */
        lcd_write_reg(R_POWER_CONTROL4, 0x0000);        /* VCOMG=0 */

        /* datasheet p. 131 */
        lcd_write_reg(R_POWER_CONTROL1, 0x0001);        /* STB=1: standby mode */

        display_on = false;
    }
}

bool lcd_enabled(void)
{
    return display_on;
}


#define RGB(r,g,b) ((((r)&0x3f) << 12)|(((g)&0x3f) << 6)|(((b)&0x3f)))


void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

/* todo: need tests */
void lcd_update_rect(int sx, int sy, int width, int height)
{
    int x, y;

    if (!display_on)
        return;

    if (width <= 0 || height <= 0)      /* nothing to do */
        return;

    width += sx;
    height += sy;

    if (width > LCD_WIDTH)
        width = LCD_WIDTH;
    if (height > LCD_HEIGHT)
        height = LCD_HEIGHT;

    lcd_write_reg(R_ENTRY_MODE, 0x1028);
    /* set update window */
    lcd_write_reg(R_HORIZ_RAM_ADDR_POS, (LCD_HEIGHT - 1) << 8);
    lcd_write_reg(R_VERT_RAM_ADDR_POS, ((width - 1) << 8) | sx);
    lcd_write_reg(R_RAM_ADDR_SET, (sx << 8) | (LCD_HEIGHT - sy - 1));
    lcd_write_cmd(R_WRITE_DATA_2_GRAM);

    for (y = sy; y < height; y++) {
        for (x = sx; x < width; x++) {
            fb_data c;
            unsigned long color;

            c = lcd_framebuffer[y][x];
            color =
                ((c & 0x1f) << 1) | ((c & 0x7e0) << 1) | ((c & 0xf800) <<
                                                          2);

            /* TODO: our color is 18-bit */
            outw((color >> 9) & 0x1ff, 0x50010008);
            outw((color) & 0x1ff, 0x50010008);
        }
    }
}

void lcd_set_contrast(int val)
{
    (void) val;
}

void lcd_set_invert_display(bool yesno)
{
    (void) yesno;
}

void lcd_set_flip(bool yesno)
{
    (void) yesno;
}

/* TODO: implement me */
void lcd_blit_yuv(unsigned char *const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    (void) src;
    (void) src_x;
    (void) src_y;
    (void) stride;
    (void) x;
    (void) y;

    if (!display_on)
        return;

    width &= ~1;                /* stay on the safe side */
    height &= ~1;

    panicf("%s", __func__);
}
