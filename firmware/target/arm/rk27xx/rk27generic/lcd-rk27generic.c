/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 Marcin Bukat
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
#include "kernel.h"
#include "lcd.h"
#include "system.h"
#include "cpu.h"
#include "spfd5420a.h"
#include "lcdif-rk27xx.h"

/* TODO: convert to udelay() */
static inline void delay_nop(int cycles)
{
    asm volatile ("1: subs %[n], %[n], #1     \n\t"
                  "   bne  1b"
                  :  
                  : [n] "r" (cycles));
}


/* converts RGB565 pixel into internal lcd bus format */
static unsigned int lcd_pixel_transform(unsigned short rgb565)
{
    unsigned int r, g, b;
    b = rgb565 & 0x1f;
    g = (rgb565 >> 5) & 0x3f;
    r = (rgb565 >> 11) & 0x1f;

    return r<<19 | g<<10 | b<<3;
}

/* not tested */
static void lcd_sleep(bool sleep)
{
    if (sleep)
    {
        /* enter sleep mode */
        lcd_write_reg(DISPLAY_CTRL1, 0x0170);
        delay_nop(50);
        lcd_write_reg(DISPLAY_CTRL1, 0x0000);
        delay_nop(50);
        lcd_write_reg(PWR_CTRL1,     0x14B4);
    }
    else
    {
         /* return to normal operation */
        lcd_write_reg(PWR_CTRL1,     0x14B0);
        delay_nop(50);
        lcd_write_reg(DISPLAY_CTRL1, 0x0173);
    }

    lcd_cmd(GRAM_WRITE);
}

static void lcd_display_init(void)
{
    unsigned int x, y;

    lcd_write_reg(RESET, 0x0001);
    delay_nop(10000);
    lcd_write_reg(RESET, 0x0000);
    delay_nop(10000);
    lcd_write_reg(IF_ENDIAN,       0x0000); /* order of receiving data */
    lcd_write_reg(DRIVER_OUT_CTRL, 0x0000);
    lcd_write_reg(ENTRY_MODE,      0x1038);
    lcd_write_reg(WAVEFORM_CTRL,   0x0100); 
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
    delay_nop(10000);
    lcd_write_reg(PANEL_IF_CTRL2,  0x0202);
    lcd_write_reg(PANEL_IF_CTRL3,  0x0300);
    delay_nop(10000);
    lcd_write_reg(PANEL_IF_CTRL4,  0x021E);
    lcd_write_reg(PANEL_IF_CTRL5,  0x0202);
    lcd_write_reg(PANEL_IF_CTRL6,  0x0100);
    lcd_write_reg(FRAME_MKR_CTRL,  0x0000);
    lcd_write_reg(MDDI_CTRL,       0x0000);

    lcd_write_reg(GAMMA_CTRL1,     0x0101);
    lcd_write_reg(GAMMA_CTRL2,     0x0000);
    lcd_write_reg(GAMMA_CTRL3,     0x0016);
    lcd_write_reg(GAMMA_CTRL4,     0x2913);
    lcd_write_reg(GAMMA_CTRL5,     0x260B);
    lcd_write_reg(GAMMA_CTRL6,     0x0101);
    lcd_write_reg(GAMMA_CTRL7,     0x1204);
    lcd_write_reg(GAMMA_CTRL8,     0x0415);
    lcd_write_reg(GAMMA_CTRL9,     0x0205);
    lcd_write_reg(GAMMA_CTRL10,    0x0303);
    lcd_write_reg(GAMMA_CTRL11,    0x0E05);
    lcd_write_reg(GAMMA_CTRL12,    0x0D01);
    lcd_write_reg(GAMMA_CTRL13,    0x010D);
    lcd_write_reg(GAMMA_CTRL14,    0x050E);
    lcd_write_reg(GAMMA_CTRL15,    0x0303);
    lcd_write_reg(GAMMA_CTRL16,    0x0502);

    /* power on */
    lcd_write_reg(DISPLAY_CTRL1,   0x0001);
    lcd_write_reg(PWR_CTRL6,       0x0001);
    lcd_write_reg(PWR_CTRL7,       0x0060);
    delay_nop(50000);
    lcd_write_reg(PWR_CTRL1,       0x16B0);
    delay_nop(10000);
    lcd_write_reg(PWR_CTRL2,       0x0147);
    delay_nop(10000);
    lcd_write_reg(PWR_CTRL3,       0x0117);
    delay_nop(10000);
    lcd_write_reg(PWR_CTRL4,       0x2F00);
    delay_nop(50000);
    lcd_write_reg(VCOM_HV2,        0x0000); /* src 0x0090 */
    delay_nop(10000);
    lcd_write_reg(VCOM_HV1,        0x0008); /* src 0x000A */
    lcd_write_reg(PWR_CTRL3,       0x01BE);
    delay_nop(10000);

    /* addresses setup */
    lcd_write_reg(WINDOW_H_START,  0x0000);
    lcd_write_reg(WINDOW_H_END,    0x00EF); /* 239 */
    lcd_write_reg(WINDOW_V_START,  0x0000);
    lcd_write_reg(WINDOW_V_END,    0x018F); /* 399 */
    lcd_write_reg(GRAM_H_ADDR,     0x0000);
    lcd_write_reg(GRAM_V_ADDR,     0x0000);

    /* display on */
    lcd_write_reg(DISPLAY_CTRL1,   0x0021);
    delay_nop(40000);
    lcd_write_reg(DISPLAY_CTRL1,   0x0061);
    delay_nop(100000);
    lcd_write_reg(DISPLAY_CTRL1,   0x0173);
    delay_nop(300000);
   
 
    /* clear screen */
    lcd_cmd(GRAM_WRITE);

    for (x=0; x<LCD_WIDTH; x++)
        for(y=0; y<LCD_HEIGHT; y++)
            lcd_data(0x000000);

    lcd_sleep(false);
}

void lcd_init_device(void)
{
    lcdif_init(LCDIF_18BIT);
    lcd_display_init();
}

void lcd_update_rect(int x, int y, int width, int height)
{
    int px = x, py = y;
    int pxmax = x + width, pymax = y + height;

    /* addresses setup */
    lcd_write_reg(WINDOW_H_START,  y);
    lcd_write_reg(WINDOW_H_END,    pymax-1);
    lcd_write_reg(WINDOW_V_START,  x);
    lcd_write_reg(WINDOW_V_END,    pxmax-1);
    lcd_write_reg(GRAM_H_ADDR,     y);
    lcd_write_reg(GRAM_V_ADDR,     x);

    lcd_cmd(GRAM_WRITE);

    for (py=y; py<pymax; py++)
    {
        for (px=x; px<pxmax; px++)
            LCD_DATA = lcd_pixel_transform(*FBADDR(px,py));
    }
}

/* Blit a YUV bitmap directly to the LCD */
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
