/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Mark Arigo
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
#include "cpu.h"
#include "lcd.h"
#include "kernel.h"
#include "system.h"

/* The SA9200 controller closely matches the register defines for the
   Samsung S6D0151 */
#define R_START_OSC             0x00
#define R_DRV_OUTPUT_CONTROL    0x01
#define R_INVERSION_CONTROL     0x02
#define R_ENTRY_MODE            0x03
#define R_DISP_CONTROL          0x07
#define R_BLANK_PERIOD_CONTROL  0x08
#define R_FRAME_CYCLE_CONTROL   0x0b
#define R_EXT_INTERFACE_CONTROL 0x0c
#define R_POWER_CONTROL1        0x10
#define R_GAMMA_CONTROL1        0x11
#define R_POWER_CONTROL2        0x12
#define R_POWER_CONTROL3        0x13
#define R_POWER_CONTROL4        0x14
#define R_RAM_ADDR_SET          0x21
#define R_WRITE_DATA_2_GRAM     0x22
#define R_RAM_READ_DATA         0x22
#define R_GAMMA_FINE_ADJ_POS1   0x30
#define R_GAMMA_FINE_ADJ_POS2   0x31
#define R_GAMMA_FINE_ADJ_POS3   0x32
#define R_GAMMA_GRAD_ADJ_POS    0x33
#define R_GAMMA_FINE_ADJ_NEG1   0x34
#define R_GAMMA_FINE_ADJ_NEG2   0x35
#define R_GAMMA_FINE_ADJ_NEG3   0x36
#define R_GAMMA_GRAD_ADJ_NEG    0x37
#define R_GAMMA_CONTROL3        0x38
#define R_GATE_SCAN_START_POS   0x40
#define R_1ST_SCR_DRV_POS       0x42
#define R_2ND_SCR_DRV_POS       0x43
#define R_HORIZ_RAM_ADDR_POS    0x44
#define R_VERT_RAM_ADDR_POS     0x45
#define R_OSC_CONTROL           0x61
#define R_LOW_POWER_MODE        0x69
#define R_PRE_DRIVING_PERIOD    0x70
#define R_GATE_OUT_PERIOD_CTRL  0x71
#define R_SOFTWARE_RESET        0x72

/* Display status */
static unsigned lcd_yuv_options SHAREDBSS_ATTR = 0;

/* wait for LCD */
static inline void lcd_wait_write(void)
{
    while (LCD1_CONTROL & LCD1_BUSY_MASK);
}

/* send LCD data */
static void lcd_send_data(unsigned data)
{
    lcd_wait_write();
    LCD1_DATA = data >> 8;
    lcd_wait_write();
    LCD1_DATA = data & 0xff;
}

/* send LCD command */
static void lcd_send_command(unsigned cmd)
{
    lcd_wait_write();
    LCD1_CMD = cmd >> 8;
    lcd_wait_write();
    LCD1_CMD = cmd & 0xff;
}

static void lcd_write_reg(unsigned reg, unsigned data)
{
    lcd_send_command(reg);
    lcd_send_data(data);
}

void lcd_init_device(void)
{
#if 0
    /* This is the init done by the OF bootloader.
       Re-initializing the lcd causes it to flash
       a white screen, so for now disable this. */
    DEV_INIT1 &= ~0x3000;
    DEV_INIT1 = DEV_INIT1;
    DEV_INIT2 &= ~0x400;

    LCD1_CONTROL = 0x4680;
    udelay(1500);
    LCD1_CONTROL = 0x4684;

    outl(1, 0x70003018);

    LCD1_CONTROL &= ~0x200;
    LCD1_CONTROL &= ~0x800;
    LCD1_CONTROL &= ~0x400;
    udelay(30000);

    LCD1_CONTROL |= 0x1;

    lcd_write_reg(R_START_OSC, 0x0001);
    udelay(50000);

    lcd_write_reg(R_GAMMA_CONTROL1, 0x171f);
    lcd_write_reg(R_POWER_CONTROL2, 0x0001);
    lcd_write_reg(R_POWER_CONTROL3, 0x08cd);
    lcd_write_reg(R_POWER_CONTROL4, 0x0416);
    lcd_write_reg(R_POWER_CONTROL1, 0x1208);
    udelay(50000);

    lcd_write_reg(R_POWER_CONTROL3, 0x081c);
    udelay(200000);

    lcd_write_reg(R_DRV_OUTPUT_CONTROL, 0x0a0c);
    lcd_write_reg(R_INVERSION_CONTROL, 0x0200);
    lcd_write_reg(R_ENTRY_MODE, 0x1030);
    lcd_write_reg(R_DISP_CONTROL, 0x0005);
    lcd_write_reg(R_BLANK_PERIOD_CONTROL, 0x030a);
    lcd_write_reg(R_FRAME_CYCLE_CONTROL, 0x0000);
    lcd_write_reg(R_EXT_INTERFACE_CONTROL, 0x0000);

    lcd_write_reg(R_GAMMA_FINE_ADJ_POS1, 0x0000);
    lcd_write_reg(R_GAMMA_FINE_ADJ_POS2, 0x0204);
    lcd_write_reg(R_GAMMA_FINE_ADJ_POS3, 0x0001);
    lcd_write_reg(R_GAMMA_GRAD_ADJ_POS, 0x0600);
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG1, 0x0607);
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG2, 0x0305);
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG3, 0x0707);
    lcd_write_reg(R_GAMMA_GRAD_ADJ_NEG, 0x0006);
    lcd_write_reg(R_GAMMA_CONTROL3, 0x0400);

    lcd_write_reg(R_GATE_SCAN_START_POS, 0x0000);
    lcd_write_reg(R_1ST_SCR_DRV_POS, 0x9f00);
    lcd_write_reg(R_2ND_SCR_DRV_POS, 0x0000);
    lcd_write_reg(R_HORIZ_RAM_ADDR_POS, 0x7f00);
    lcd_write_reg(R_VERT_RAM_ADDR_POS, 0x9f00);

    lcd_write_reg(0x00a8, 0x0125);
    lcd_write_reg(0x00a9, 0x0014);
    lcd_write_reg(0x00a7, 0x0022);

    lcd_write_reg(R_DISP_CONTROL, 0x0021);
    udelay(40000);
    lcd_write_reg(R_DISP_CONTROL, 0x0023);
    udelay(40000);
    lcd_write_reg(R_DISP_CONTROL, 0x1037);

    lcd_write_reg(R_RAM_ADDR_SET, 0x0000);
#endif
}

/*** hardware configuration ***/
#if 0
int lcd_default_contrast(void)
{
    return DEFAULT_CONTRAST_SETTING;
}
#endif

void lcd_set_contrast(int val)
{
    (void)val;
}

void lcd_set_invert_display(bool yesno)
{
    (void)yesno;
}

/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
    (void)yesno;
}

void lcd_yuv_set_options(unsigned options)
{
    lcd_yuv_options = options;
}

/* Performance function to blit a YUV bitmap directly to the LCD */
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
 
/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
    const fb_data *addr;
    
    if (x + width >= LCD_WIDTH)
        width = LCD_WIDTH - x;
    if (y + height >= LCD_HEIGHT)
        height = LCD_HEIGHT - y;
        
    if ((width <= 0) || (height <= 0))
        return; /* Nothing left to do. */

    addr = &lcd_framebuffer[y][x];

    do {
        lcd_write_reg(R_RAM_ADDR_SET, ((y++ & 0xff) << 8) | (x & 0xff));
        lcd_send_command(R_WRITE_DATA_2_GRAM);

        int w = width;
        do {
            lcd_send_data(*addr++);
        } while (--w > 0);
        addr += LCD_WIDTH - width;
    } while (--height > 0);
}
