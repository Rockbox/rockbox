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

static inline void delay_nop(int cycles)
{
    asm volatile ("1: subs %[n], %[n], #1     \n\t"
                  "   bne  1b"
                  :  
                  : [n] "r" (cycles));
}

static unsigned int lcd_data_transform(unsigned int data)
{
    /* 18 bit interface */
    unsigned int r, g, b;
    r = (data & 0x0000fc00)<<8;
    /* g = ((data & 0x00000300) >> 2) | ((data & 0x000000e0) >> 3); */
    g = ((data & 0x00000300) << 6) | ((data & 0x000000e0) << 5);
    b = (data & 0x00000001f) << 3;

    return (r | g | b);
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

static void lcd_cmd(unsigned int cmd)
{
    LCD_COMMAND = lcd_data_transform(cmd);
}

static void lcd_data(unsigned int data)
{
    LCD_DATA = lcd_data_transform(data);
}

static void lcd_write_reg(unsigned int reg, unsigned int val)
{
    lcd_cmd(reg);
    lcd_data(val);
}

static void lcdctrl_bypass(unsigned int on_off)
{
    while (!(LCDC_STA & LCDC_MCU_IDLE));

    if (on_off)
        MCU_CTRL |= MCU_CTRL_BYPASS;
    else
        MCU_CTRL &= ~MCU_CTRL_BYPASS;
}

/* This part is unclear. I am unable to use buffered/FIFO based writes
 * to lcd. Depending on settings of IF I get various patterns on display
 * but not what I want to display apparently.
 */
static void lcdctrl_init(void)
{
    /* alpha b111
     * stop at current frame complete
     * MCU mode
     * 24b RGB
     */
    LCDC_CTRL = ALPHA(7) | LCDC_STOP | LCDC_MCU | RGB24B;
    MCU_CTRL = ALPHA_BASE(0x3f) | MCU_CTRL_BYPASS;

    HOR_ACT = 400 + 3;    /* define horizonatal active region */
    VERT_ACT = 240;       /* define vertical active region */
    VERT_PERIOD = 0xfff;  /* CSn/WEn/RDn signal timings */

    LINE0_YADDR = LINE_ALPHA_EN | 0x7fe;
    LINE1_YADDR = LINE_ALPHA_EN | ((1 * 400) - 2);
    LINE2_YADDR = LINE_ALPHA_EN | ((2 * 400) - 2);
    LINE3_YADDR = LINE_ALPHA_EN | ((3 * 400) - 2);

    LINE0_UVADDR = 0x7fe + 1;
    LINE1_UVADDR = ((1 * 400) - 2 + 1);
    LINE2_UVADDR = ((2 * 400) - 2 + 1);
    LINE3_UVADDR = ((3 * 400) - 2 + 1);

#if 0
    LINE0_YADDR = 0;
    LINE1_YADDR = (1 * 400);
    LINE2_YADDR = (2 * 400);
    LINE3_YADDR = (3 * 400);

    LINE0_UVADDR = 1;
    LINE1_UVADDR = (1 * 400) + 1;
    LINE2_UVADDR = (2 * 400) + 1;
    LINE3_UVADDR = (3 * 400) + 1;

    START_X = 0;
    START_Y = 0;
    DELTA_X = 0x200; /* no scaling */
    DELTA_Y = 0x200; /* no scaling */
#endif
    LCDC_INTR_MASK = INTR_MASK_LINE; /* INTR_MASK_EVENLINE; */
}

/* configure pins to drive lcd in 18bit mode */
static void iomux_lcd(void)
{
    unsigned long muxa;

    muxa = SCU_IOMUXA_CON & ~(IOMUX_LCD_VSYNC|IOMUX_LCD_DEN|0xff);
    muxa |= IOMUX_LCD_D18|IOMUX_LCD_D20|IOMUX_LCD_D22|IOMUX_LCD_D17|IOMUX_LCD_D16;

    SCU_IOMUXA_CON = muxa;
    SCU_IOMUXB_CON |= IOMUX_LCD_D815;
}

/* not tested */
static void lcd_sleep(unsigned int sleep)
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

void lcd_init_device()
{
    unsigned int x, y;

    iomux_lcd();       /* setup pins for 18bit lcd interface */
    lcdctrl_init();    /* basic lcdc module configuration */

    lcdctrl_bypass(1); /* run in bypass mode - all writes goes directly to lcd controller */

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

    for (x=0; x<400; x++)
        for(y=0; y<240; y++)
            lcd_data(0x000000);

    lcd_sleep(0);
}

/* This is ugly hack. We drive lcd in bypass mode
 * where all writes goes directly to lcd controller.
 * This is suboptimal at best. IF module povides
 * FIFO, internal sram buffer, hardware scaller,
 * DMA signals, hardware alpha blending and more.
 * BUT the fact is that I have no idea how to use
 * this modes. Datasheet floating around is very
 * unclean in this regard and OF uses ackward
 * lcd update routines which are hard to understand.
 * Moreover OF sets some bits in IF module registers
 * which are referred as reseved in datasheet.
 */
void lcd_update()
{
    unsigned int x,y;

    for (y=0; y<240; y++)
        for (x=0; x<400; x++)
            LCD_DATA = lcd_pixel_transform(lcd_framebuffer[y][x]);
}

/* not implemented yet */
void lcd_update_rect(int x, int y, int width, int height)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    lcd_update();
}
