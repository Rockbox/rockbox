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

/* Settings to remember when display is turned off */
static bool invert;
static bool flip;
static int  contrast;

static bool power_on;
static bool display_on;

/* Forward declarations */
#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
static void lcd_display_off(void);
#endif

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

/* LCD init */
void lcd_init_device(void)
{
#if 0
    /* This is done by the OF bootloader, no need to redo */
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
#endif

    power_on = true;
    display_on = true;
    invert = false;
    flip = false;
    contrast = DEFAULT_CONTRAST_SETTING;
}

#ifdef HAVE_LCD_SLEEP
static void lcd_power_on(void)
{
    LCD1_CONTROL |= 0x1;

    /** Power ON Sequence **/

    /* Start Oscillation */
    lcd_write_reg(R_START_OSC, 0x0001);
    
    sleep(HZ/20); /* 50ms or more */

    /* DSTB=0, SAP2-0=001, BT2-0=101, DC2-0=000, AP2-0=001, SLP=0, STB=0 */
    lcd_write_reg(R_POWER_CONTROL1, 0x0d04);

    /* VR1C=0, VRN14-10=10111, VRP14-10=11111 */
    lcd_write_reg(R_GAMMA_CONTROL1, 0x171f);

    /* SVC3-0=0000, VRH5-4=01 */
    lcd_write_reg(R_POWER_CONTROL2, 0x0001);

    /* VCMR=1, PON=0, VRH3-0=1101 */
    lcd_write_reg(R_POWER_CONTROL3, 0x080d);

    /* VDV6-0=0000100, VCOMG=0, VCM6-0=xxxxxxx */
    lcd_write_reg(R_POWER_CONTROL4, 0x0400 | contrast);

    /* DSTB=0, SAP2-0=010, BT2-0=010, DC2-0=000, AP2-0=010, SLP=0, STB=0 */
    lcd_write_reg(R_POWER_CONTROL1, 0x1208);

    sleep(HZ/20); /* 50ms or more */

    /* VCMR=1, PON=1, VRH3-0=1100 */
    lcd_write_reg(R_POWER_CONTROL3, 0x081c);

    sleep(HZ/20); /* OF bootlaoder uses 200ms, no delay in OF firmware */

    /* Instructions for other mode settings (in register order). */

    lcd_write_reg(R_DRV_OUTPUT_CONTROL, flip ? 0x090c : 0x0a0c);

    /* FL1-0=10, FDL=0 */
    lcd_write_reg(R_INVERSION_CONTROL, 0x0200);

    /* BGR=1, MDT1-0=00, I/D1-0=11, AM=0 */
    lcd_write_reg(R_ENTRY_MODE, 0x1030);

    /* PT1-0=00, SPT=0, GON=0, DTE=0, CL=0, REV=1, D1-0=01 */
    lcd_write_reg(R_DISP_CONTROL, 0x0005);

    /* FP3-0=0011, BT3-0=1010 */
    lcd_write_reg(R_BLANK_PERIOD_CONTROL, 0x030a);

    /* DIV1-0=00, RTN3-0=0000 */
    lcd_write_reg(R_FRAME_CYCLE_CONTROL, 0x0000);

    /* RM=0, DM1-0=00, RIM1-0=00 */
    lcd_write_reg(R_EXT_INTERFACE_CONTROL, 0x0000);

    /* PKP1=0x0, PKP0=0x0 */
    lcd_write_reg(R_GAMMA_FINE_ADJ_POS1, 0x0000);

    /* PKP3=0x2, PKP2=0x4 */
    lcd_write_reg(R_GAMMA_FINE_ADJ_POS2, 0x0204);

    /* PKP5=0x0, PKP4=0x1 */
    lcd_write_reg(R_GAMMA_FINE_ADJ_POS3, 0x0001);

    /* PRP1=0x6, PRP0=0x0 */
    lcd_write_reg(R_GAMMA_GRAD_ADJ_POS, 0x0600);

    /* PKN1=0x6, PKN0=0x7 */
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG1, 0x0607);

    /* PKN3=0x3, PKN2=0x5 */
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG2, 0x0305);

    /* PKN5=0x7, PKN4=0x7 */
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG3, 0x0707);

    /* PRN1=0x0, PRN0=0x6 */
    lcd_write_reg(R_GAMMA_GRAD_ADJ_NEG, 0x0006);

    /* VRN0=0x4, VRP=0x0 */
    lcd_write_reg(R_GAMMA_CONTROL3, 0x0400);

    /* SCN=0x0 */
    lcd_write_reg(R_GATE_SCAN_START_POS, 0x0000);

    /* SE1=LCD_HEIGHT-1, SS1=0x0 */
    lcd_write_reg(R_1ST_SCR_DRV_POS, 0x9f00);

    /* SE2=0x0, SS2=0x0 */
    lcd_write_reg(R_2ND_SCR_DRV_POS, 0x0000);

    /* HEA=LCD_WIDTH-1, HSA=0x0 */
    lcd_write_reg(R_HORIZ_RAM_ADDR_POS, 0x7f00);

    /* VEA=LCD_HEIGHT-1, VSA=0x0 */
    lcd_write_reg(R_VERT_RAM_ADDR_POS, 0x9f00);

    /* Unknown registers */
    lcd_write_reg(0x00a8, 0x0125);
    lcd_write_reg(0x00a9, 0x0014);
    lcd_write_reg(0x00a7, 0x0022);

    power_on = true;
}

static void lcd_power_off(void)
{
    /* Display must be off first */
    if (display_on)
        lcd_display_off();

    power_on = false;

    /** Power OFF sequence **/

    /* DSTB=0, SAP2-0=000, BT2-0=001, DC2-0=000, AP2-0=000, SLP=0, STB=0 */
    lcd_write_reg(R_POWER_CONTROL1, 0x0100);

    /* VCMR=1, PON=0, VRH3-0=1101 */
    lcd_write_reg(R_POWER_CONTROL3, 0x080d);
}

void lcd_sleep(void)
{
    if (power_on)
        lcd_power_off();

    /* Set standby mode */

    /* PT1-0=00, SPT=0, GON=1, DTE=1, CL=0, REV=1, D1-0=10 */
    lcd_write_reg(R_DISP_CONTROL, 0x0036);

    /* DSTB=0, SAP2-0=000, BT2-0=101, DC2-0=000, AP2-0=000, SLP=0, STB=1 */
    lcd_write_reg(R_POWER_CONTROL1, 0x0501);

    LCD1_CONTROL &= ~0xffff0001;
}
#endif

#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
static void lcd_display_off(void)
{
    display_on = false;

    /** Display OFF sequence **/

    /* PT1-0=10, SPT=0, GON=1, DTE=1, CL=0, REV=1, D1-0=10 */
    lcd_write_reg(R_DISP_CONTROL, 0x1036);

    sleep(HZ/25); /* 2 or more frames */

    /* PT1-0=10, SPT=0, GON=1, DTE=0, CL=0, REV=1, D1-0=00 */
    lcd_write_reg(R_DISP_CONTROL, 0x1034);

    sleep(HZ/500); /* 1ms or more */

    /* PT1-0=10, SPT=0, GON=0, DTE=0, CL=0, REV=1, D1-0=00 */
    lcd_write_reg(R_DISP_CONTROL, 0x1004);

    sleep(HZ/25); /* 2 or more frames */
}
#endif

#if defined(HAVE_LCD_ENABLE)
static void lcd_display_on(void)
{
    /* Be sure power is on first */
    if (!power_on)
        lcd_power_on();

    /** Display ON Sequence **/

    /* PT1-0=00, SPT=0, GON=1, DTE=0, CL=0, REV=0, D1-0=01 */
    lcd_write_reg(R_DISP_CONTROL, 0x0021);

    sleep(HZ/500); /* 1ms or more */

    /* PT1-0=00, SPT=0, GON=1, DTE=0, CL=0, REV=0, D1-0=11 */
    lcd_write_reg(R_DISP_CONTROL, 0x0023);

    sleep(HZ/25); /* 2 or more frames */

    /* PT1-0=10, SPT=0, GON=1, DTE=1, CL=0, REV=x, D1-0=11 */
    lcd_write_reg(R_DISP_CONTROL, invert ? 0x1033 : 0x1037);

    display_on = true;
}

void lcd_enable(bool on)
{
    if (on == display_on)
        return;

    if (on)
    {
        lcd_display_on();
        /* Probably out of sync and we don't wanna pepper the code with
           lcd_update() calls for this. */
        lcd_update();
        send_event(LCD_EVENT_ACTIVATION, NULL);
    }
    else
    {
        lcd_display_off();
    }
}
#endif

#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
bool lcd_active(void)
{
    return display_on;
}
#endif

/*** hardware configuration ***/
int lcd_default_contrast(void)
{
    return DEFAULT_CONTRAST_SETTING;
}

void lcd_set_contrast(int val)
{
    contrast = val & 0x7f;

    if (!display_on)
        return;

    /* VDV6-0=0000100, VCOMG=0, VCM6-0=xxxxxxx */
    lcd_write_reg(R_POWER_CONTROL4, 0x0400 | contrast);
}

void lcd_set_invert_display(bool yesno)
{
    invert = yesno;

    if (!display_on)
        return;

    /* PT1-0=10, SPT=0, GON=1, DTE=1, CL=0, REV=x, D1-0=11 */
    lcd_write_reg(R_DISP_CONTROL, invert ? 0x1033 : 0x1037);
}

/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
    flip = yesno;

    if (!display_on)
        return;

    /* DPL=0, EPL=1, SM=0, GS=x, SS=x, NL4-0=01100 */
    lcd_write_reg(R_DRV_OUTPUT_CONTROL, flip ? 0x090c : 0x0a0c);
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

    if (!display_on)
        return;

    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x;

    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y;

    if ((width <= 0) || (height <= 0))
        return; /* Nothing left to do. */

    addr = &lcd_framebuffer[y][x];

    lcd_write_reg(R_HORIZ_RAM_ADDR_POS, ((x + width - 1) << 8) | x);
    lcd_write_reg(R_VERT_RAM_ADDR_POS, ((y + height -1) << 8) | y);
    lcd_write_reg(R_RAM_ADDR_SET, ((y & 0xff) << 8) | (x & 0xff));
    lcd_send_command(R_WRITE_DATA_2_GRAM);

    do {
        int w = width;
        do {
            lcd_send_data(*addr++);
        } while (--w > 0);
        addr += LCD_WIDTH - width;
    } while (--height > 0);
}
