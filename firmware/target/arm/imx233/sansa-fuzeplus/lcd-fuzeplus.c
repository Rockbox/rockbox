/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2011 by Amaury Pouly
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
#include "kernel.h"
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
static int lcd_dcp_channel = -1;
#ifdef HAVE_LCD_INVERT
static int lcd_reg_0x61_val = 1; /* used to invert display */
#endif
#ifdef HAVE_LCD_FLIP
static int lcd_reg_3_val = 0x1030; /* controls to flip display */
#endif

static enum lcd_kind_t
{
    LCD_KIND_7783 = 0x7783,
    LCD_KIND_9325 = 0x9325,
} lcd_kind = LCD_KIND_7783;

static void setup_parameters(void)
{
    imx233_lcdif_init();
    imx233_lcdif_set_lcd_databus_width(18);
    imx233_lcdif_set_word_length(18);
    imx233_lcdif_set_timings(1, 2, 2, 2);
    imx233_lcdif_enable_underflow_recover(true);
}

static void setup_lcd_pins(bool use_lcdif)
{
    /* WARNING
     * the B1P22 and B1P24 pins are used by the tuner i2c! Do NOT drive
     * them as lcd_dotclk and lcd_hsync or it will break the tuner! */
    imx233_pinctrl_acquire(1, 18, "lcd reset");
    imx233_pinctrl_acquire(1, 19, "lcd rs");
    imx233_pinctrl_acquire(1, 20, "lcd wr");
    imx233_pinctrl_acquire(1, 21, "lcd cs");
    imx233_pinctrl_acquire(1, 23, "lcd enable");
    imx233_pinctrl_acquire(1, 25, "lcd vsync");
    //imx233_pinctrl_acquire_mask(1, 0x3ffff, "lcd data");
    if(use_lcdif)
    {
        imx233_pinctrl_set_function(1, 25, PINCTRL_FUNCTION_GPIO); /* lcd_vsync */
        imx233_pinctrl_set_function(1, 21, PINCTRL_FUNCTION_MAIN); /* lcd_cs */
        imx233_pinctrl_set_function(1, 23, PINCTRL_FUNCTION_GPIO); /* lcd_enable */
        imx233_pinctrl_set_function(1, 18, PINCTRL_FUNCTION_MAIN); /* lcd_reset */
        imx233_pinctrl_set_function(1, 19, PINCTRL_FUNCTION_MAIN); /* lcd_rs */
        imx233_pinctrl_set_function(1, 16, PINCTRL_FUNCTION_MAIN); /* lcd_d16 */
        imx233_pinctrl_set_function(1, 17, PINCTRL_FUNCTION_MAIN); /* lcd_d17 */
        imx233_pinctrl_set_function(1, 20, PINCTRL_FUNCTION_MAIN); /* lcd_wr */
        HW_PINCTRL_MUXSELn_CLR(2) = 0xffffffff; /* lcd_d{0-15} */
    }
    else
    {
        HW_PINCTRL_MUXSELn_SET(2) = 0xffffffff; /* lcd_d{0-15} */
        HW_PINCTRL_DOEn_CLR(1) = 0x2bfffff;
        imx233_pinctrl_set_function(1, 16, PINCTRL_FUNCTION_GPIO); /* lcd_d16 */
        imx233_pinctrl_set_function(1, 17, PINCTRL_FUNCTION_GPIO); /* lcd_d17 */
        imx233_pinctrl_set_function(1, 19, PINCTRL_FUNCTION_GPIO); /* lcd_rs */
        imx233_pinctrl_set_function(1, 20, PINCTRL_FUNCTION_GPIO); /* lcd_wr */
        imx233_pinctrl_set_function(1, 21, PINCTRL_FUNCTION_GPIO); /* lcd_cs */
        imx233_pinctrl_set_function(1, 23, PINCTRL_FUNCTION_GPIO); /* lcd_enable */
        imx233_pinctrl_set_function(1, 25, PINCTRL_FUNCTION_GPIO); /* lcd_vsync */
    }
}

static void setup_lcd_pins_i80(bool i80)
{
    if(i80)
    {
        imx233_pinctrl_set_drive(1, 19, PINCTRL_DRIVE_12mA); /* lcd_rs */
        imx233_pinctrl_set_drive(1, 20, PINCTRL_DRIVE_12mA); /* lcd_wr */
        imx233_pinctrl_set_drive(1, 21, PINCTRL_DRIVE_12mA); /* lcd_cs */
        imx233_pinctrl_set_drive(1, 23, PINCTRL_DRIVE_12mA); /* lcd_enable */
        imx233_pinctrl_set_function(1, 19, PINCTRL_FUNCTION_GPIO); /* lcd_rs */
        imx233_pinctrl_set_function(1, 20, PINCTRL_FUNCTION_GPIO); /* lcd_wr */
        imx233_pinctrl_set_function(1, 21, PINCTRL_FUNCTION_GPIO); /* lcd_cs */
        imx233_pinctrl_set_function(1, 23, PINCTRL_FUNCTION_GPIO); /* lcd_enable */
        /* lcd_{rs,wr,cs,enable} */
        HW_PINCTRL_DOEn_SET(1) = (1 << 19) | (1 << 20) | (1 << 21) | (1 << 23);
        HW_PINCTRL_DOUTn_SET(1) = (1 << 19) | (1 << 20) | (1 << 21) | (1 << 23);

        HW_PINCTRL_DOEn_CLR(1) = 0x3ffff; /* lcd_d{0-17} */
        HW_PINCTRL_MUXSELn_SET(2) = 0xffffffff; /* lcd_d{0-15} as GPIO */
        imx233_pinctrl_set_function(1, 16, PINCTRL_FUNCTION_GPIO); /* lcd_d16 */
        imx233_pinctrl_set_function(1, 17, PINCTRL_FUNCTION_GPIO); /* lcd_d17 */
        imx233_pinctrl_set_function(1, 18, PINCTRL_FUNCTION_GPIO); /* lcd_reset */
        imx233_pinctrl_set_function(1, 19, PINCTRL_FUNCTION_GPIO); /* lcd_rs */
    }
    else
    {
        HW_PINCTRL_DOUTn_SET(1) = (1 << 19) | (1 << 20) | (1 << 21) | (1 << 23);
        
        imx233_pinctrl_set_drive(1, 19, PINCTRL_DRIVE_4mA); /* lcd_rs */
        imx233_pinctrl_set_drive(1, 20, PINCTRL_DRIVE_4mA); /* lcd_wr */
        imx233_pinctrl_set_drive(1, 21, PINCTRL_DRIVE_4mA); /* lcd_cs */
        imx233_pinctrl_set_drive(1, 23, PINCTRL_DRIVE_4mA); /* lcd_enable */
        imx233_pinctrl_set_function(1, 19, PINCTRL_FUNCTION_MAIN); /* lcd_rs */
        imx233_pinctrl_set_function(1, 20, PINCTRL_FUNCTION_MAIN); /* lcd_wr */
        imx233_pinctrl_set_function(1, 21, PINCTRL_FUNCTION_MAIN); /* lcd_cs */
        HW_PINCTRL_DOEn_CLR(1) = 0x3ffff; /* lcd_d{0-17} */
        HW_PINCTRL_MUXSELn_CLR(2) = 0xffffffff; /* lcd_d{0-15} as lcd_d{0-15} */
        imx233_pinctrl_set_function(1, 16, PINCTRL_FUNCTION_MAIN); /* lcd_d16 */
        imx233_pinctrl_set_function(1, 17, PINCTRL_FUNCTION_MAIN); /* lcd_d17 */
        imx233_pinctrl_set_function(1, 18, PINCTRL_FUNCTION_MAIN); /* lcd_reset */
        imx233_pinctrl_set_function(1, 19, PINCTRL_FUNCTION_MAIN); /* lcd_rs */
    }
}

static void common_lcd_enable(bool enable)
{
    imx233_lcdif_enable(enable);
    setup_lcd_pins(enable); /* use GPIO pins when disable */
}

static void setup_lcdif(void)
{
    setup_parameters();
    common_lcd_enable(true);
    imx233_lcdif_enable_bus_master(true);
}

static inline uint32_t encode_16_to_18(uint32_t a)
{
    return ((a & 0xff) << 1) | (((a >> 8) & 0xff) << 10);
}

static inline uint32_t decode_18_to_16(uint32_t a)
{
    return ((a >> 1) & 0xff) | ((a >> 2) & 0xff00);
}

static void setup_lcdif_clock(void)
{
    /* the LCD seems to work at 24Mhz, so use the xtal clock with no divider */
    imx233_clkctrl_enable(CLK_PIX, false);
    imx233_clkctrl_set_div(CLK_PIX, 1);
    imx233_clkctrl_set_bypass(CLK_PIX, true); /* use XTAL */
    imx233_clkctrl_enable(CLK_PIX, true);
}

static uint32_t i80_read_register(uint32_t data_out)
{
    imx233_lcdif_wait_ready();
    /* lcd_enable is mapped to the RD pin of the controller */
    imx233_pinctrl_set_gpio(1, 21, true); /* lcd_cs */
    imx233_pinctrl_set_gpio(1, 19, true); /* lcd_rs */
    imx233_pinctrl_set_gpio(1, 23, true); /* lcd_enable */
    imx233_pinctrl_set_gpio(1, 20, true); /* lcd_wr */
    HW_PINCTRL_DOEn_SET(1) = 0x3ffff; /* lcd_d{0-17} */
    udelay(2);
    imx233_pinctrl_set_gpio(1, 19, false); /* lcd_rs */
    udelay(1);
    imx233_pinctrl_set_gpio(1, 21, false); /* lcd_cs */
    udelay(1);
    imx233_pinctrl_set_gpio(1, 20, false); /* lcd_wr */
    udelay(1);
    HW_PINCTRL_DOUTn_SET(1) = data_out & 0x3ffff; /* lcd_d{0-17} */
    udelay(1);
    imx233_pinctrl_set_gpio(1, 20, true); /* lcd_wr */
    udelay(3);
    HW_PINCTRL_DOEn_CLR(1) = 0x3ffff; /* lcd_d{0-17} */
    udelay(2);
    imx233_pinctrl_set_gpio(1, 23, false); /* lcd_enable */
    udelay(1);
    imx233_pinctrl_set_gpio(1, 19, true); /* lcd_rs */
    udelay(1);
    imx233_pinctrl_set_gpio(1, 23, true); /* lcd_enable */
    udelay(3);
    imx233_pinctrl_set_gpio(1, 23, false); /* lcd_enable */
    udelay(2);
    uint32_t data_in = HW_PINCTRL_DINn(1) & 0x3ffff; /* lcd_d{0-17} */
    udelay(1);
    imx233_pinctrl_set_gpio(1, 23, true); /* lcd_enable */
    udelay(1);
    imx233_pinctrl_set_gpio(1, 21, true); /* lcd_cs */
    udelay(1);
    return data_in;
}

static void lcd_write_reg(uint32_t reg, uint32_t data)
{
    uint32_t old_reg = reg;
    imx233_lcdif_wait_ready();
    /* get back to 18-bit word length */
    imx233_lcdif_set_word_length(18);
    reg = encode_16_to_18(reg);
    data = encode_16_to_18(data);
    
    imx233_lcdif_pio_send(false, 1, &reg);
    if(old_reg != 0x22)
        imx233_lcdif_pio_send(true, 1, &data);
}

static uint32_t lcd_read_reg(uint32_t reg)
{
    setup_lcd_pins_i80(true);
    uint32_t data_in = i80_read_register(encode_16_to_18(reg));
    setup_lcd_pins_i80(false);
    lcd_write_reg(0x22, 0);
    return decode_18_to_16(data_in);
}

#define REG_MDELAY  0xffffffff
struct lcd_sequence_entry_t
{
    uint32_t reg, data;
};

static void lcd_send_sequence(struct lcd_sequence_entry_t *seq, unsigned count)
{
    for(;count-- > 0; seq++)
    {
        if(seq->reg == REG_MDELAY)
            mdelay(seq->data);
        else
            lcd_write_reg(seq->reg, seq->data);
    }
}

#define _begin_seq() static struct lcd_sequence_entry_t __seq[] = {
#define _mdelay(a) {REG_MDELAY, a},
#define _lcd_write_reg(a, b) {a, b},
#define _end_seq() }; lcd_send_sequence(__seq, sizeof(__seq) / sizeof(__seq[0]));

static void lcd_init_seq_7783(void)
{
    _begin_seq()
    _mdelay(200)
    _lcd_write_reg(1, 0x100)
    _lcd_write_reg(2, 0x700)
    _lcd_write_reg(3, 0x1030)
    _lcd_write_reg(7, 0x121)
    _lcd_write_reg(8, 0x302)
    _lcd_write_reg(9, 0x200)
    _lcd_write_reg(0xa, 0)
    _lcd_write_reg(0x10, 0x790)
    _lcd_write_reg(0x11, 5)
    _lcd_write_reg(0x12, 0)
    _lcd_write_reg(0x13, 0)
    _mdelay(100)
    _lcd_write_reg(0x10, 0x12b0)
    _mdelay(100)
    _lcd_write_reg(0x11, 7)
    _mdelay(100)
    _lcd_write_reg(0x12, 0x89)
    _lcd_write_reg(0x13, 0x1d00)
    _lcd_write_reg(0x29, 0x2f)
    _mdelay(50)
    _lcd_write_reg(0x30, 0)
    _lcd_write_reg(0x31, 0x505)
    _lcd_write_reg(0x32, 0x205)
    _lcd_write_reg(0x35, 0x206)
    _lcd_write_reg(0x36, 0x408)
    _lcd_write_reg(0x37, 0)
    _lcd_write_reg(0x38, 0x504)
    _lcd_write_reg(0x39, 0x206)
    _lcd_write_reg(0x3c, 0x206)
    _lcd_write_reg(0x3d, 0x408)
    _lcd_write_reg(0x50, 0) /* left X ? */ 
    _lcd_write_reg(0x51, 0xef) /* right X ? */
    _lcd_write_reg(0x52, 0) /* top Y ? */
    _lcd_write_reg(0x53, 0x13f) /* bottom Y ? */
    _lcd_write_reg(0x20, 0) /* left X ? */ 
    _lcd_write_reg(0x21, 0) /* top Y ? */
    _lcd_write_reg(0x60, 0xa700)
    _lcd_write_reg(0x61, 1)
    _lcd_write_reg(0x90, 0x33)
    _lcd_write_reg(0x2b, 0xa)
    _lcd_write_reg(9, 0)
    _lcd_write_reg(7, 0x133)
    _mdelay(50)
    _lcd_write_reg(0x22, 0)
    _end_seq()
}

static void lcd_init_seq_9325(void)
{
    _begin_seq()
    _lcd_write_reg(0xe5, 0x78f0)
    _lcd_write_reg(0xe3, 0x3008)
    _lcd_write_reg(0xe7, 0x12)
    _lcd_write_reg(0xef, 0x1231)
    _lcd_write_reg(0, 1)
    _lcd_write_reg(1, 0x100)
    _lcd_write_reg(2, 0x700)
    _lcd_write_reg(3, 0x1030)
    _lcd_write_reg(4, 0)
    _lcd_write_reg(8, 0x207)
    _lcd_write_reg(9, 0)
    _lcd_write_reg(0xa, 0)
    _lcd_write_reg(0xc, 0)
    _lcd_write_reg(0xd, 0)
    _lcd_write_reg(0xf, 0)
    _lcd_write_reg(0x10, 0)
    _lcd_write_reg(0x11, 7)
    _lcd_write_reg(0x12, 0)
    _lcd_write_reg(0x13, 0)
    _mdelay(20)
    _lcd_write_reg(0x10, 0x1290)
    _lcd_write_reg(0x11, 7)
    _mdelay(50)
    _lcd_write_reg(0x12, 0x19)
    _mdelay(50)
    _lcd_write_reg(0x13, 0x1700)
    _lcd_write_reg(0x29, 0x14)
    _mdelay(50)
    _lcd_write_reg(0x20, 0)
    _lcd_write_reg(0x21, 0)
    _lcd_write_reg(0x30, 0x504)
    _lcd_write_reg(0x31, 7)
    _lcd_write_reg(0x32, 6)
    _lcd_write_reg(0x35, 0x106)
    _lcd_write_reg(0x36, 0x202)
    _lcd_write_reg(0x37, 0x504)
    _lcd_write_reg(0x38, 0x500)
    _lcd_write_reg(0x39, 0x706)
    _lcd_write_reg(0x3c, 0x204)
    _lcd_write_reg(0x3d, 0x202)
    _lcd_write_reg(0x50, 0)
    _lcd_write_reg(0x51, 0xef)
    _lcd_write_reg(0x52, 0)
    _lcd_write_reg(0x53, 0x13f)
    _lcd_write_reg(0x60, 0xa700)
    _lcd_write_reg(0x61, 1)
    _lcd_write_reg(0x6a, 0)
    _lcd_write_reg(0x2b, 0xd)
    _mdelay(50)
    _lcd_write_reg(0x90, 0x11)
    _lcd_write_reg(0x92, 0x600)
    _lcd_write_reg(0x93, 3)
    _lcd_write_reg(0x95, 0x110)
    _lcd_write_reg(0x97, 0)
    _lcd_write_reg(0x98, 0)
    _lcd_write_reg(7, 0x173)
    _lcd_write_reg(0x22, 0)
    _end_seq()
}

static void lcd_sync_settings(void)
{
#ifdef HAVE_LCD_INVERT
    lcd_write_reg(0x61, lcd_reg_0x61_val);
#endif
#ifdef HAVE_LCD_FLIP
    lcd_write_reg(3, lcd_reg_3_val);
#endif
}

void lcd_init_device(void)
{
    lcd_dcp_channel = imx233_dcp_acquire_channel(TIMEOUT_NOBLOCK);
    if(lcd_dcp_channel < 0)
        panicf("imx233_framebuffer_init: imx233_dcp_acquire_channel failed!");
    setup_lcdif_clock();
    setup_lcdif();

    for(int i = 0; i < 10; i++)
    {
        lcd_kind = lcd_read_reg(0);
        mdelay(5);
        if(lcd_kind == LCD_KIND_7783 || lcd_kind == LCD_KIND_9325)
            break;
    }
    // reset device
    imx233_lcdif_reset_lcd(true);
    mdelay(50);
    imx233_lcdif_reset_lcd(false);
    mdelay(10);
    imx233_lcdif_reset_lcd(true);

    switch(lcd_kind)
    {
        case LCD_KIND_7783: lcd_init_seq_7783(); break;
        case LCD_KIND_9325: lcd_init_seq_9325(); break;
        default:
            lcd_kind = LCD_KIND_7783;
            lcd_init_seq_7783(); break;
    }

    lcd_sync_settings();

#ifdef HAVE_LCD_ENABLE
    lcd_on = true;
#endif
}

#ifdef HAVE_LCD_ENABLE
bool lcd_active(void)
{
    return lcd_on;
}

static void lcd_enable_7783(bool enable)
{
    if(!enable)
    {
        _begin_seq()
        _lcd_write_reg(7, 0x131)
        _mdelay(50)
        _lcd_write_reg(7, 0x20)
        _mdelay(50)
        _lcd_write_reg(0x10, 0x82)
        _mdelay(50)
        _end_seq()
    }
    else
    {
        _begin_seq()
        _lcd_write_reg(0x11, 5)
        _lcd_write_reg(0x10, 0x12b0)
        _mdelay(50)
        _lcd_write_reg(7, 0x11)
        _mdelay(50)
        _lcd_write_reg(0x12, 0x89)
        _mdelay(50)
        _lcd_write_reg(0x13, 0x1d00)
        _mdelay(50)
        _lcd_write_reg(0x29, 0x2f)
        _mdelay(50)
        _lcd_write_reg(0x2b, 0xa)
        _lcd_write_reg(7, 0x133)
        _mdelay(50)
        _lcd_write_reg(0x22, 0)
        _end_seq()
    }
}

static void lcd_enable_9325(bool enable)
{
    if(!enable)
    {
        _begin_seq()
        _lcd_write_reg(7, 0x131)
        _mdelay(10)
        _lcd_write_reg(7, 0x130)
        _mdelay(10)
        _lcd_write_reg(7, 0)
        _lcd_write_reg(0x10, 0x80)
        _lcd_write_reg(0x11, 0)
        _lcd_write_reg(0x12, 0)
        _lcd_write_reg(0x13, 0)
        _mdelay(200)
        _lcd_write_reg(0x10, 0x82)
        _end_seq()
    }
    else
    {
        _begin_seq()
        _lcd_write_reg(0x10, 0x80)
        _lcd_write_reg(0x11, 0)
        _lcd_write_reg(0x12, 0)
        _lcd_write_reg(0x13, 0)
        _lcd_write_reg(7, 1)
        _mdelay(200)
        _lcd_write_reg(0x10, 0x1290)
        _lcd_write_reg(0x11, 7)
        _mdelay(50)
        _lcd_write_reg(0x12, 0x19)
        _mdelay(50)
        _lcd_write_reg(0x13, 0x1700)
        _lcd_write_reg(0x29, 0x10)
        _mdelay(50)
        _lcd_write_reg(7, 0x133)
        _lcd_write_reg(0x22, 0)
        _end_seq()
    }
}

void lcd_enable(bool enable)
{
    if(lcd_on == enable)
        return;

    lcd_on = enable;
    
    if(enable)
        common_lcd_enable(true);
    switch(lcd_kind)
    {
        case LCD_KIND_7783: lcd_enable_7783(enable); break;
        case LCD_KIND_9325: lcd_enable_9325(enable); break;
    }
    if(!enable)
        common_lcd_enable(false);
    else
    {
        lcd_sync_settings();
        send_event(LCD_EVENT_ACTIVATION, NULL);
    }
}
#endif

#ifdef HAVE_LCD_INVERT
void lcd_set_invert_display(bool yesno)
{
    lcd_reg_0x61_val = yesno ? 0 : 1;
    #ifdef HAVE_LCD_ENABLE
    if(!lcd_on)
        return;
    #endif
    /* same for both kinds */
    lcd_write_reg(0x61, lcd_reg_0x61_val);
}
#endif

#ifdef HAVE_LCD_FLIP
void lcd_set_flip(bool yesno)
{
    lcd_reg_3_val = yesno ? 0x1080 : 0x1030;
    #ifdef HAVE_LCD_ENABLE
    if(!lcd_on)
        return;
    #endif
    /* same for both kinds */
    lcd_write_reg(3, lcd_reg_3_val);
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
    /* make sure the rectangle is bounded in the screen */
    if (w > LCD_WIDTH - x)/* Clip right */
        w = LCD_WIDTH - x;
    if (x < 0)/* Clip left */
    {
        w += x;
        x = 0;
    }
    if (w <= 0)
        return; /* nothing left to do */

    if (h > LCD_HEIGHT - y) /* Clip bottom */
        h = LCD_HEIGHT - y;
    if (y < 0) /* Clip top */
    {
        h += y;
        y = 0;
    }
    if (h <= 0)
        return; /* nothing left to do */


    imx233_lcdif_wait_ready();
#ifdef HAVE_LCD_FLIP
    if(!(lcd_reg_3_val&0x10))
    {
        int xr = LCD_WIDTH - x;
        lcd_write_reg(0x50, xr-w);
        lcd_write_reg(0x51, xr-1);
        lcd_write_reg(0x20, xr-w);
    }else
#endif
    {
        lcd_write_reg(0x50, x);
        lcd_write_reg(0x51, x + w - 1);
        lcd_write_reg(0x20, x);
    }
#ifdef HAVE_LCD_FLIP
    if(!(lcd_reg_3_val&0x20))
    {
        int yr = LCD_HEIGHT - y;
        lcd_write_reg(0x52, yr-h);
        lcd_write_reg(0x53, yr-1);
        lcd_write_reg(0x21, yr-h);
    }else
#endif
    {
        lcd_write_reg(0x52, y);
        lcd_write_reg(0x53, y + h - 1);
        lcd_write_reg(0x21, y);
    }
    lcd_write_reg(0x22, 0);
    imx233_lcdif_wait_ready();
    imx233_lcdif_set_word_length(16);
    imx233_lcdif_set_byte_packing_format(0xf); /* two pixels per 32-bit word */
    /* there are two cases here:
     * - either width = LCD_WIDTH and we can directly memcopy a part of lcd_framebuffer to FRAME
     *   and send it
     * - either width != LCD_WIDTH and we have to build a contiguous copy of the rectangular area
     *   into FRAME before sending it (which is slower and doesn't use the hardware)
     * In all cases, FRAME just acts as a temporary buffer.
     * NOTE It's more interesting to do a copy to FRAME in all cases since in system mode
     * the clock runs at 24MHz which provides barely 10MB/s bandwidth compared to >100MB/s
     * for memcopy operations
     */
    if(w == LCD_WIDTH)
    {
        memcpy((void *)FRAME, FBADDR(x,y), w * h * sizeof(fb_data));
    }
    else
    {
        for(int i = 0; i < h; i++)
            memcpy((fb_data *)FRAME + i * w, FBADDR(x,y + i), w * sizeof(fb_data));
    }
    /* WARNING The LCDIF has a limitation on the vertical count ! In 16-bit packed mode
     * (which we used, ie 16-bit per pixel, 2 pixels per 32-bit words), the v_count
     * field must be a multiple of 2. Furthermore, it seems the lcd controller doesn't
     * really like when both w and h are even, probably because the writes to the GRAM
     * are done on several words and the controller requires dummy writes.
     * The workaround is to always make sure that we send a number of pixels which is
     * a multiple of 4 so that both the lcdif and the controller are happy. If any
     * of w or h is odd, we will send a copy of the first pixels as dummy writes. We will
     * send at most 3 bytes. We then send (w * h + 3) / 4 x 4 bytes.
     */
    if(w % 4 || (h & 1) == 1)
    {
        /* copy three pixel after the last one */
        for(int i = 0; i < 3; i++)
            *((fb_data *)FRAME + w * h + i) = *((fb_data *)FRAME + i);
        /* WARNING we need to update w and h to reflect the pixel count BUT it
         * has no relation to w * h (it can even be 2 * prime). Hopefully, w <= 240 and
         * h <= 320 so w * h <= 76800 and (w * h + 3) / 4 <= 38400 which fits into
         * a 16-bit integer (horizontal count). */
        h = (w * h + 3) / 4;
        w = 4;
    }
    imx233_lcdif_dma_send((void *)FRAME_PHYS_ADDR, w, h);
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
        lcd_putsf(0, 0, "lcd kind: %s",
            lcd_kind == LCD_KIND_7783 ? "st7783" :
            lcd_kind == LCD_KIND_9325 ? "ili9325" : "unknown");
        lcd_update();
        yield();
    }

    return true;
}
#endif
