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

static inline void slcd_set_data_width(unsigned width)
{
    uint16_t mcfg = SLCD_MCFG;
    mcfg &= ~SLCD_MCFG_DATA_WIDTH_bm;
    mcfg |= SLCD_MCFG_DATA_WIDTH(width);
    SLCD_MCFG = mcfg;
}

static inline void lcd_wait_ready(void)
{
    while(SLCD_MSTATE & SLCD_MSTATE_BUSY) {}
}

static void lcd_send_cmd(unsigned cmd)
{
    lcd_wait_ready();
    SLCD_MDATA = cmd | SLCD_MDATA_RS;
#if 0
    /* out of interest, this the sequence used by the OF (assuming PC18 is setup as gpio */
    lcd_wait_ready();
    /* for some reason the firmware not only sets RS in MDATA but also sets PC2 which is lcd_b2
     * and corresponds to slcd_d0, which does not make much sense */
    jz_gpio_set_output(2, 18, false); /* PC2 low */
    SLCD_MDATA = cmd | SLCD_MDATA_RS;
    lcd_wait_ready();
    jz_gpio_set_output(2, 18, true); /* PC2 high */
#endif
}

/* warning: sends data using whatever width is specified in MCFG! */
static void lcd_send_data(unsigned data)
{
    lcd_wait_ready();
    SLCD_MDATA = data;
}

#ifdef FULL_INIT
#define WAIT    0xfe /* fake register */
#define END     0xff

/* format: (<cmd> <data size> <data0> <dataN>)+ */
/* controller is S6D0171, seems somewhat compatible with ILI9163 and ILI9342 */
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

/* format: (<cmd> <data size> <data0> <dataN>)+ */
/* controller is ILI9342C (warning: not the same as ILI9342) */
static INIT_ATTR uint8_t init_seq_v2[] =
{
/*  cmd  sz  data... */
    0xc8, 3, 0xff, 0x93, 0x42, /* Set EXTC: turn on external commands */
    0x36, 1, 0xd8, /* Memory Access Control: BGR, MX, MY, ML */
    0x3a, 1, 0x66, /* Set Pixel Format: RGB=MCU=18 bits/pix */
    0xc0, 2, 0x15, 0x15, /* Power Control 1 */
    0xc1, 1, 0x01, /* Power Control 2 */
    0xc5, 1, 0xda, /* VCOM Control 1 */
    0xb1, 2, 0x00, 0x1b, /* Frame Rate Control: DIVA=0, RTNA=27 => frate = 63Hz*/
    0xb4, 1, 0x02, /* Display Inversion Control: 2-dot inversion */
    0xe0, 15, 0x0f, 0x13, 0x17, 0x04, 0x13, 0x07, 0x40, 0x39,
              0x4f, 0x06, 0x0d, 0x0a, 0x1f, 0x22, 0x00, /* Positive Gamma Correction */
    0xe1, 15, 0x00, 0x21, 0x24, 0x03, 0x0f, 0x05, 0x38, 0x32,
              0x49, 0x00, 0x09, 0x08, 0x32, 0x35, 0x0f, /* Negative Gamma Correction */
    0x11, 0, /* Sleep Out */
    WAIT, 120, /* wait 120ms */
    0x29, 0, /* Display On */
    END
};

static INIT_ATTR void lcd_send_seq(uint8_t *seq)
{
    /* make sure to use 8-bit data per transfer */
    slcd_set_data_width(SLCD_MCFG_DATA_WIDTH_8BIT_x1);
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
    /* make sure DMA mode is disabled */
    SLCD_MCTRL &= ~SLCD_MCTRL_DMA_EN;
    lcd_wait_ready();
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

    /* the purpose of PC9 is unclear */
    jz_gpio_setup_std_out(2, 9, false);

    if(fiiox1_get_hw_version() == 1)
        lcd_send_seq(init_seq_v1);
    else
        lcd_send_seq(init_seq_v2);
#endif
    /* for some reason, the OF drives slcd_rs manually, revert that and let the LCDC do the job */
    jz_gpio_set_function(2, 18, PIN_FUN(0));
    /* change LCD settings: switch to 16-bit per pixel */
    slcd_set_data_width(SLCD_MCFG_DATA_WIDTH_8BIT_x1);
    lcd_send_cmd(0x3a);
    lcd_send_data(0x55);
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
    slcd_set_data_width(SLCD_MCFG_DATA_WIDTH_8BIT_x1);
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
    slcd_set_data_width(SLCD_MCFG_DATA_WIDTH_8BIT_x2);
    for(int yy = 0; yy < h; yy++)
        for(int xx = 0; xx < w; xx++)
            lcd_send_data(*FBADDR(x + xx, y + yy));
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
