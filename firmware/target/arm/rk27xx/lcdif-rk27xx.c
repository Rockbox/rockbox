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
#include "lcdif-rk27xx.h"


unsigned int lcd_data_transform(unsigned int data)
{
    unsigned int r, g, b;

#if defined(RK27_GENERIC)
    /* 18 bit interface */
    r = (data & 0x0000fc00)<<8;
    /* g = ((data & 0x00000300) >> 2) | ((data & 0x000000e0) >> 3); */
    g = ((data & 0x00000300) << 6) | ((data & 0x000000e0) << 5);
    b = (data & 0x00000001f) << 3;
#elif defined(HM60X) || defined(HM801)
    /* 16 bit interface */
    r = (data & 0x0000f800) << 8;
    g = (data & 0x000007e0) << 5;
    b = (data & 0x0000001f) << 3;
#else
#error "Unknown target"
#endif

    return (r | g | b);
}

void lcd_cmd(unsigned int cmd)
{
    LCD_COMMAND = lcd_data_transform(cmd);
}

void lcd_data(unsigned int data)
{
    LCD_DATA = lcd_data_transform(data);
}

void lcd_write_reg(unsigned int reg, unsigned int val)
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

    HOR_ACT = LCD_WIDTH + 3;    /* define horizonatal active region */
    VERT_ACT = LCD_HEIGHT;       /* define vertical active region */
    VERT_PERIOD = 0xfff;  /* CSn/WEn/RDn signal timings */

    LINE0_YADDR = LINE_ALPHA_EN | 0x7fe;
    LINE1_YADDR = LINE_ALPHA_EN | ((1 * LCD_WIDTH) - 2);
    LINE2_YADDR = LINE_ALPHA_EN | ((2 * LCD_WIDTH) - 2);
    LINE3_YADDR = LINE_ALPHA_EN | ((3 * LCD_WIDTH) - 2);

    LINE0_UVADDR = 0x7fe + 1;
    LINE1_UVADDR = ((1 * LCD_WIDTH) - 2 + 1);
    LINE2_UVADDR = ((2 * LCD_WIDTH) - 2 + 1);
    LINE3_UVADDR = ((3 * LCD_WIDTH) - 2 + 1);

#if 0
    LINE0_YADDR = 0;
    LINE1_YADDR = (1 * LCD_WIDTH);
    LINE2_YADDR = (2 * LCD_WIDTH);
    LINE3_YADDR = (3 * LCD_WIDTH);

    LINE0_UVADDR = 1;
    LINE1_UVADDR = (1 * LCD_WIDTH) + 1;
    LINE2_UVADDR = (2 * LCD_WIDTH) + 1;
    LINE3_UVADDR = (3 * LCD_WIDTH) + 1;

    START_X = 0;
    START_Y = 0;
    DELTA_X = 0x200; /* no scaling */
    DELTA_Y = 0x200; /* no scaling */
#endif
    LCDC_INTR_MASK = INTR_MASK_LINE; /* INTR_MASK_EVENLINE; */
}

/* configure pins to drive lcd in 18bit mode (16bit mode for HiFiMAN's) */
static void iomux_lcd(enum lcdif_mode_t mode)
{
    unsigned long muxa;

    muxa = SCU_IOMUXA_CON & ~(IOMUX_LCD_VSYNC|IOMUX_LCD_DEN|0xff);

    if (mode == LCDIF_18BIT)
    {
        muxa |= IOMUX_LCD_D18|IOMUX_LCD_D20|IOMUX_LCD_D22|IOMUX_LCD_D17|IOMUX_LCD_D16;
    }

    SCU_IOMUXA_CON = muxa;
    SCU_IOMUXB_CON |= IOMUX_LCD_D815;
}

void lcdif_init(enum lcdif_mode_t mode)
{
    iomux_lcd(mode);   /* setup pins for lcd interface */
    lcdctrl_init();    /* basic lcdc module configuration */
    lcdctrl_bypass(1); /* run in bypass mode - all writes goes directly to lcd controller */
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
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}
