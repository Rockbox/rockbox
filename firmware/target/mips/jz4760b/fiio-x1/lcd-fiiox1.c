/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2017 by Will Robertson
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
#include "system.h"
#include "lcd.h"
#include "lcd-target.h"
#include "gpio-jz4760b.h"
#include "system-fiiox1.h"

/* the OF bootloader initializes the LCD for us so we do not generalise run a full setup sequence
 * but we keep the code there for reference or in case it is needed */
//#define FULL_INIT

#ifdef HAVE_LCD_ENABLE
bool lcd_on;
#endif

static inline void lcd_wait_ready(void)
{
    while(SLCD_MSTATE & SLCD_MSTATE_BUSY) {}
}

static void lcd_send_cmd(unsigned cmd)
{
    lcd_wait_ready();
    /* for some reason the firmware not only sets RS in MDATA but also sets PC2 which is lcd_b2
     * and corresponds to slcd_d0, which does not make much sense */
    jz_gpio_set_output(2, 2, false); /* PC2 low */
    SLCD_MDATA = cmd | SLCD_MDATA_RS;
    lcd_wait_ready();
    jz_gpio_set_output(2, 2, true); /* PC2 high */
}

static void lcd_send_data(unsigned data)
{
    lcd_wait_ready();
    SLCD_MDATA = data;
}

#ifdef FULL_INIT
#define WAIT    0xfe /* fake register */
#define END     0xff

/* format: (<cmd> <data size> <data0> <dataN>)+ */
/* Seems somewhat compatible with ILI9163 and ILI9342 */
static INIT_ATTR uint8_t init_seq_v1[] =
{
/*  cmd  sz  data... */
    0x01, 0, /* Gamma Set */
    WAIT, 50, /* 50ms */
    0x11, 0, /* Sleep Out */
    WAIT, 100, /* 100 ms */
    0x13, 0, /* Normal Display */
    WAIT, 100, /* 100 ms */
    0xb0, 21, 0x3c, 0x3c, 0x3c, 0x3c, 0x08, 0x08, 0x08, 0x08,
              0x08, 0x08, 0x08, 0x08, 0x3c, 0x20, 0x08, 0x00,
              0x01, 0x1c, 0x06, 0x66, 0x66, /* Display Control */
    0xb1, 30, 0x14, 0x38, 0x00, 0x19, 0x57, 0x1f, 0x00, 0x80,
              0x14, 0x38, 0x80, 0x19, 0x57, 0x1f, 0x00, 0x80,
              0x14, 0x08, 0x19, 0x57, 0x1f, 0x00, 0x00, 0x34,
              0x08, 0x19, 0x57, 0x1f, 0x00, 0x00, /* Power Control */
    0xd2, 1, 0x01, /* Unknown */
    0xd3, 2, 0x00, 0x57, /* Unknown */
    0xe0, 13, 0x10, 0x00, 0x00, 0x0f, 0x1b, 0x23, 0x25, 0x2d,
              0x37, 0x38, 0x3f, 0x47, 0x39, /* Gamma Setting */
    0xe1, 13, 0x10, 0x00, 0x00, 0x0f, 0x1b, 0x23, 0x25, 0x2d,
              0x37, 0x38, 0x3f, 0x47, 0x39, /* Gamma Setting */
    0xe2, 13, 0x10, 0x00, 0x00, 0x0f, 0x1b, 0x23, 0x25, 0x2d,
              0x37, 0x38, 0x3f, 0x47, 0x39, /* Gamma Setting */
    0xe3, 13, 0x10, 0x00, 0x00, 0x0f, 0x1b, 0x23, 0x25, 0x2d,
              0x37, 0x38, 0x3f, 0x47, 0x39, /* Gamma Setting */
    0xe4, 13, 0x10, 0x00, 0x00, 0x0f, 0x1b, 0x23, 0x25, 0x2d,
              0x37, 0x38, 0x3f, 0x47, 0x39, /* Gamma Setting */
    0xe5, 13, 0x10, 0x00, 0x00, 0x0f, 0x1b, 0x23, 0x25, 0x2d,
              0x37, 0x38, 0x3f, 0x47, 0x39, /* Gamma Setting */
    0x2a, 4, 0x00, 0x00, 0x01, 0x3f, /* Column Address Set */
    0x2b, 4, 0x00, 0x00, 0x00, 0xef, /* Page Address Set */
    0x3a, 1, 0x06, /* Color Mode Setting (262k colors) */
    0x36, 1, 0xd8,
    0xb1, 2, 0x00, 0x15,
    0xf2, 1, 0x00,
    0x26, 1, 0x01,
    0x35, 1, 0x00, /* Tearing Effect Output On */
    0x44, 2, 0x00, 0x3f,
    0xb5, 4, 0x02, 0x02, 0xff, 0xff,
    0x11, 0, /* Sleep Out */
    WAIT, 120, /* 120ms */
    0x29, 0, /* Display On */
    0x2c, 0, /* GRAM Write */
    END
};

static INIT_ATTR void lcd_send_seq(uint8_t *seq)
{
    while(seq[0] != END)
    {
        if(seq[0] == WAIT)
        {
            mdelay(seq[1]);
            seq += 2;
        }
        else
        {
            lcd_send_cmd(seq[0]);
            for(unsigned i = 0; i < seq[1]; i++)
                lcd_send_data(i + 2);
            seq += 2 + seq[1];
        }
    }
}
#endif

/* LCD init */
void INIT_ATTR lcd_init_device(void)
{
#ifdef FULL_INIT
    /* PC[27:22,19:12,9:2] => lcd_{b2-b7, pclk, de, g2-g7, vsync, hsync, r2-r7}, all use function 0
     * note that in SLCD mode, we have the following mapping:
     * - slcd_dat0-17 => lcd_{b2-b7,g2-g7,r2-r7}
     * - slcd_clk => lcd_pclk
     * - slcd_cs => lcd_vsync
     * - slcd_rs => lcd_hsync
     * not sure why lcd_de is setup... */
    jz_gpio_set_function_mask(2, 0xfcff3fc, PIN_FUN(0));
    jz_gpio_enable_pullup_mask(2, 0xfcff3fc, false);

    /* PE4 is reset */
    jz_gpio_setup_std_out(4, 4, true);
    mdelay(50);
    jz_gpio_set_output(4, 4, false);
    mdelay(50);
    jz_gpio_set_output(4, 4, true);
    mdelay(150);

    /* PC9 and PC2, unure what they do but PC2 is used for command select it seems */
    jz_gpio_setup_std_out(3, 9, false);
    jz_gpio_setup_std_out(3, 2, true);

    if(fiiox1_get_hw_version() == 1)
        lcd_send_seq(init_seq_v1);
#endif
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
    lcd_send_cmd(0x2a);
    lcd_send_data(x >> 8);
    lcd_send_data(x & 0xff);
    lcd_send_data((x + w - 1) >> 8);
    lcd_send_data((x + w - 1) & 0xff);
    lcd_send_cmd(0x2b);
    lcd_send_data(y >> 8);
    lcd_send_data(y & 0xff);
    lcd_send_data((y + h - 1) >> 8);
    lcd_send_data((y + h - 1) & 0xff);

    lcd_send_cmd(0x2c);
    for(int yy = 0; yy < h; yy++)
        for(int xx = 0; xx < w; xx++)
        {
            unsigned pix = *FBADDR(x + xx, y + yy);
            lcd_send_data(pix >> 16);
            lcd_send_data((pix >> 8) & 0xff);
            lcd_send_data(pix & 0xff);
        }
}

#ifdef HAVE_LCD_ENABLE
bool lcd_active(void)
{
    return lcd_on;
}

void lcd_enable(bool state)
{
    if(state == lcd_on)
        return;

    if(state)
    {
        lcd_on = true;
        lcd_update();
        send_event(LCD_EVENT_ACTIVATION, NULL);
    }
    else
    {
        lcd_on = false;
    }
}
#endif
