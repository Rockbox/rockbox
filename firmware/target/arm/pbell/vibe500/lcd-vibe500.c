/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
 *
 * Copyright (C) 2009 by Szymon Dziok
 * Based on the Iriver H10 code by Barry Wardell
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

/** Initialized in lcd_init_device() **/
/* Is the power turned on? */
static bool power_on;
/* Is the display turned on? */
static bool display_on;
/* Reverse flag. Must be remembered when display is turned off. */
static unsigned short disp_control_rev;
/* Contrast setting << 8 */
static int lcd_contrast;

static unsigned lcd_yuv_options SHAREDBSS_ATTR = 0;

/* Forward declarations */
#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
static void lcd_display_off(void);
#endif

/* register defines for the Renesas HD66773R */
/*
   In Packard Bell from the OF - it seems to be
   HD66774(gate) with HD66772(source) - registers
   are very similar but not all the same
*/
#define R_START_OSC             0x00
#define R_DEVICE_CODE_READ      0x00
#define R_DRV_OUTPUT_CONTROL    0x01
#define R_DRV_AC_CONTROL        0x02
#define R_POWER_CONTROL1        0x03
#define R_POWER_CONTROL2        0x04
#define R_ENTRY_MODE            0x05
#define R_COMPARE_REG           0x06
#define R_DISP_CONTROL          0x07
#define R_FRAME_CYCLE_CONTROL   0x0b
#define R_POWER_CONTROL3        0x0c
#define R_POWER_CONTROL4        0x0d
#define R_POWER_CONTROL5        0x0e
#define R_GATE_SCAN_START_POS   0x0f
#define R_VERT_SCROLL_CONTROL   0x11
#define R_1ST_SCR_DRV_POS       0x14
#define R_2ND_SCR_DRV_POS       0x15
#define R_HORIZ_RAM_ADDR_POS    0x16
#define R_VERT_RAM_ADDR_POS     0x17
#define R_RAM_WRITE_DATA_MASK   0x20
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
#define R_GAMMA_AMP_ADJ_POS     0x3a
#define R_GAMMA_AMP_ADJ_NEG     0x3b

static inline void lcd_wait_write(void)
{
    while (LCD1_CONTROL & LCD1_BUSY_MASK);
}

/* Send command */
static inline void lcd_send_cmd(unsigned v)
{
    lcd_wait_write();
    LCD1_CMD = (v >> 8);
    lcd_wait_write();
    LCD1_CMD = (v & 0xff);
}

/* Send 16-bit data */
static inline void lcd_send_data(unsigned v)
{
    lcd_wait_write();
    LCD1_DATA = (v >> 8);
    lcd_wait_write();
    LCD1_DATA = (v & 0xff);
}

/* Send 16-bit data byte-swapped.  */
static inline void lcd_send_data_swapped(unsigned v)
{
    lcd_wait_write();
    LCD1_DATA = (v & 0xff);
    lcd_wait_write();
    LCD1_DATA = (v >> 8);
}

/* Write value to register */
static void lcd_write_reg(int reg, int val)
{
    lcd_send_cmd(reg);
    lcd_send_data(val);
}

/*** hardware configuration ***/

int lcd_default_contrast(void)
{
    return DEFAULT_CONTRAST_SETTING;
}

void lcd_set_contrast(int val)
{
    lcd_contrast = (val&0x1f) << 8;
    if (!power_on) return;

    /* VCOMG=1, VDV4-0=xxxxx, VCM4-0=11000 */
    lcd_write_reg(R_POWER_CONTROL5, 0x2018 | lcd_contrast);
}

void lcd_set_invert_display(bool yesno)
{
    if (yesno == (disp_control_rev == 0x0000))
        return;

    disp_control_rev = yesno ? 0x0000 : 0x0004;

    if (!display_on)
        return;

    /* PT1-0=00, VLE2-1=00, SPT=0, GON=1, DTE=1, REV=x, D1-0=11 */
    lcd_write_reg(R_DISP_CONTROL, 0x0033 | disp_control_rev);
}

/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
    if (!power_on)
        return;

    /* SM=0, GS=x, SS=x, NL4-0=10011 (G1-G160) */
    lcd_write_reg(R_DRV_OUTPUT_CONTROL, yesno ? 0x0313 : 0x0013);
}

/* LCD init */
void lcd_init_device(void)
{
    power_on = true;
    display_on = true;
    disp_control_rev = 0x0004;
    lcd_contrast     = DEFAULT_CONTRAST_SETTING << 8;
}

#ifdef HAVE_LCD_SLEEP
static void lcd_power_on(void)
{
    /* from the OF */
    lcd_write_reg(R_START_OSC,0x01); /* START_OSC */
    sleep(HZ/40); /* 25ms */
    /* set 396x160 dots, SM=0, GS=x, SS=0, NL4-0=10011 G1-G160)*/
    lcd_write_reg(R_DRV_OUTPUT_CONTROL,0x13);
    /* FLD1-0=01 (1 field), B/C=1, EOR=1 (C-pat), NW5-0=000000 (1 row) */
    lcd_write_reg(R_DRV_AC_CONTROL,7 << 8);
    /* DIT=0, BGR=1, HWM=0, I/D1-0=0 - decrement AC, AM=1, LG2-0=000 */
    lcd_write_reg(R_ENTRY_MODE,0x1008);
    lcd_write_reg(0x25,0x0000); /* - ?? */
    lcd_write_reg(0x26,0x0202); /* - ?? */
    lcd_write_reg(0x0A,0x0000); /* - ?? */
    lcd_write_reg(R_FRAME_CYCLE_CONTROL,0x0000);
    lcd_write_reg(R_POWER_CONTROL4,0x0000);
    sleep(HZ/67); /* 15ms */
    lcd_write_reg(R_POWER_CONTROL5,0x0000);
    sleep(HZ/67); /* 15ms */
    lcd_write_reg(R_POWER_CONTROL3,0x0000);
    lcd_write_reg(0x09,0x0008); /* - ?? */
    sleep(HZ/67); /* 15ms */
    lcd_write_reg(R_POWER_CONTROL4,0x0003);
    sleep(HZ/67); /* 15ms */
    lcd_write_reg(R_POWER_CONTROL5,0x1019);
    sleep(HZ/20); /* 50ms */
    lcd_write_reg(R_POWER_CONTROL4,0x0013);
    sleep(HZ/67); /* 15ms */
    lcd_write_reg(R_POWER_CONTROL1,0x0010);
    sleep(HZ/67); /* 15ms */
    lcd_write_reg(0x09,0x0000); /* - ?? */
    sleep(HZ/67); /* 15ms */
    lcd_write_reg(R_POWER_CONTROL1,0x0010);
    sleep(HZ/67); /* 15ms */
    lcd_write_reg(R_POWER_CONTROL5,0x3019);
    sleep(HZ*10/66);/* /6.6 = 150ms */
    lcd_write_reg(0x09,0x0002); /* - ?? */
    sleep(HZ/67); /* 15ms */
    lcd_write_reg(R_POWER_CONTROL4,0x0018);
    sleep(HZ/20); /* 50ms */
    /* RAM Address set (0x0000) */
    lcd_write_reg(R_RAM_ADDR_SET,0x0000);
    /* Gamma settings */
    lcd_write_reg(R_GAMMA_FINE_ADJ_POS1,0x0004);
    lcd_write_reg(R_GAMMA_FINE_ADJ_POS2,0x0606);
    lcd_write_reg(R_GAMMA_FINE_ADJ_POS3,0x0505);
    lcd_write_reg(R_GAMMA_GRAD_ADJ_POS,0x0206);
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG1,0x0505);
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG2,0x0707);
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG3,0x0105);
    lcd_write_reg(R_GAMMA_GRAD_ADJ_NEG,0x0301);
    lcd_write_reg(R_GAMMA_AMP_ADJ_POS,0x1A00);
    lcd_write_reg(R_GAMMA_AMP_ADJ_NEG,0x010E);

    lcd_write_reg(R_GATE_SCAN_START_POS,0x0000);
    /* Horizontal ram address start/end position (0,127); */
    lcd_write_reg(R_HORIZ_RAM_ADDR_POS,0x7F00);
    /* Vertical ram address start/end position (0,159); */
    lcd_write_reg(R_VERT_RAM_ADDR_POS,0x9F00);

    lcd_write_reg(R_DISP_CONTROL,0x0005);
    sleep(HZ/25); /* 40ms */
    lcd_write_reg(R_DISP_CONTROL,0x0025);
    lcd_write_reg(R_DISP_CONTROL,0x0027);
    sleep(HZ/25); /* 40ms */
    lcd_write_reg(R_DISP_CONTROL,0x0033 | disp_control_rev);
    sleep(HZ/100); /* 10ms */
    lcd_write_reg(R_POWER_CONTROL1,0x0110);
    lcd_write_reg(R_FRAME_CYCLE_CONTROL,0x0000);
    lcd_write_reg(R_POWER_CONTROL4,0x0013);
    lcd_write_reg(R_POWER_CONTROL5,0x2018 | lcd_contrast);
    sleep(HZ/20); /* 50ms */

    power_on = true;
}

static void lcd_power_off(void)
{
    /* Display must be off first */
    if (display_on)
        lcd_display_off();

    /* power_on = false; */

    /** Power OFF sequence **/
    /* The method is unknown */
}

void lcd_sleep(void)
{
    if (power_on)
        lcd_power_off();

    /* Set standby mode */
    /* Because we dont know how to power off display
       we cannot set standby */
    /* BT2-0=000, DC2-0=000, AP2-0=000, SLP=0, STB=1 */
    /* lcd_write_reg(R_POWER_CONTROL1, 0x0001); */
}
#endif

#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
static void lcd_display_off(void)
{
    display_on = false;

    /** Display OFF sequence **/
    /* In the OF it is called "EnterStandby" */

    /* PT1-0=00, VLE2-1=00, SPT=0, GON=1, DTE=1, REV=x, D1-0=10 */
    lcd_write_reg(R_DISP_CONTROL, 0x0032 | disp_control_rev);
    sleep(HZ/22); /* 45ms */
    /* PT1-0=00, VLE2-1=00, SPT=0, GON=1, DTE=0, REV=x, D1-0=10 */
    lcd_write_reg(R_DISP_CONTROL, 0x0022 | disp_control_rev);
    sleep(HZ/22); /* 45ms */
    /* PT1-0=00, VLE2-1=00, SPT=0, GON=0, DTE=0, REV=0, D1-0=00 */
    lcd_write_reg(R_DISP_CONTROL, 0x0000);

    lcd_write_reg(R_POWER_CONTROL1, 0x0000);
    lcd_write_reg(0x09, 0x0000); /* -?? */
    lcd_write_reg(R_POWER_CONTROL4, 0x0000);

    sleep(HZ/22); /* 45ms */
}
#endif

#if defined(HAVE_LCD_ENABLE)
static void lcd_display_on(void)
{
    /* Be sure power is on first */
    if (!power_on)
        lcd_power_on();

    /** Display ON Sequence **/
    /* In the OF it is called "ExitStandby" */

    lcd_write_reg(R_START_OSC,1);
    sleep(HZ/40); /* 25ms */
    lcd_write_reg(R_POWER_CONTROL4,0);
    sleep(HZ/67); /* 15ms */
    lcd_write_reg(R_POWER_CONTROL5,0);
    sleep(HZ/67); /* 15ms */
    lcd_write_reg(R_DISP_CONTROL,0);
    sleep(HZ/67); /* 15ms */
    lcd_write_reg(R_POWER_CONTROL3,0);
    lcd_write_reg(0x09,8); /* -?? */
    sleep(HZ/67); /* 15ms */
    lcd_write_reg(R_POWER_CONTROL4,3);
    sleep(HZ/67); /* 15ms */
    lcd_write_reg(R_POWER_CONTROL5,0x1019);
    sleep(HZ/20); /* 50ms */
    lcd_write_reg(R_POWER_CONTROL4,0x13);
    sleep(HZ/67); /* 15ms */
    lcd_write_reg(R_POWER_CONTROL1,0x10);
    sleep(HZ/20); /* 50ms */
    lcd_write_reg(0x09,0); /* -?? */
    sleep(HZ/67); /* 15ms */
    lcd_write_reg(R_POWER_CONTROL1,0x10);
    sleep(HZ/67); /* 15ms */
    lcd_write_reg(R_POWER_CONTROL5,0x3019);
    sleep(HZ*10/66);/* /6.6 = 150ms */
    lcd_write_reg(0x09,2); /* -?? */
    sleep(HZ/20); /* 50ms */
    lcd_write_reg(R_DISP_CONTROL,5);
    sleep(HZ/22); /* 45ms */
    lcd_write_reg(R_DISP_CONTROL,0x25);
    sleep(HZ/22); /* 45ms */
    lcd_write_reg(R_DISP_CONTROL,0x27);
    sleep(HZ/25); /* 40ms */
    lcd_write_reg(R_DISP_CONTROL,0x33 | disp_control_rev);
    sleep(HZ/22); /* 45ms */
    /* fix contrast */
    lcd_write_reg(R_POWER_CONTROL5, 0x2018 | lcd_contrast);

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

/*** update functions ***/

void lcd_yuv_set_options(unsigned options)
{
    lcd_yuv_options = options;
}

/* Line write helper function for lcd_yuv_blit. Write two lines of yuv420. */

extern void lcd_write_yuv420_lines(unsigned char const * const src[3],
                                   int width,
                                   int stride);
extern void lcd_write_yuv420_lines_odither(unsigned char const * const src[3],
                                           int width,
                                           int stride,
                                           int x_screen, /* To align dither pattern */
                                           int y_screen);

/* Performance function to blit a YUV bitmap directly to the LCD */
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    const unsigned char *yuv_src[3];
    const unsigned char *ysrc_max;
    int y0;
    int options;

    if (!display_on)
        return;

    width &= ~1;
    height &= ~1;

    lcd_write_reg(R_VERT_RAM_ADDR_POS, ((LCD_WIDTH - 1 - x) << 8) |
    ((LCD_WIDTH-1) - (x + width - 1)));

    y0 = LCD_HEIGHT - 1 - y;

    lcd_write_reg(R_ENTRY_MODE,0x1000);

    yuv_src[0] = src[0] + src_y * stride + src_x;
    yuv_src[1] = src[1] + (src_y * stride >> 2) + (src_x >> 1);
    yuv_src[2] = src[2] + (yuv_src[1] - src[1]);
    ysrc_max = yuv_src[0] + height * stride;

    options = lcd_yuv_options;

    do
    {
        lcd_write_reg(R_HORIZ_RAM_ADDR_POS, (y0 << 8) | (y0 - 1));
        lcd_write_reg(R_RAM_ADDR_SET, ((LCD_WIDTH - 1 - x) << 8) | y0);

        /* start drawing */
        lcd_send_cmd(R_WRITE_DATA_2_GRAM);

        if (options & LCD_YUV_DITHER)
        {
            lcd_write_yuv420_lines_odither(yuv_src, width, stride,x, y);
            y -= 2;
        }
        else
        {
            lcd_write_yuv420_lines(yuv_src, width, stride);
        }

        y0 -= 2;
        yuv_src[0] += stride << 1;
        yuv_src[1] += stride >> 1;
        yuv_src[2] += stride >> 1;
    }
    while (yuv_src[0] < ysrc_max);
    lcd_write_reg(R_ENTRY_MODE,0x1008);
}

/* Update a fraction of the display. */
void lcd_update_rect(int x0, int y0, int width, int height)
{
    const fb_data *addr;
    int x1, y1;

    if (!display_on)
       return;

    if ((width<=0)||(height<=0))
       return;

    if ((x0 + width)>=LCD_WIDTH)
       width = LCD_WIDTH - x0;
    if ((y0 + height)>=LCD_HEIGHT)
       height = LCD_HEIGHT - y0;

    y1 = (y0 + height) - 1;
    x1 = (x0 + width) - 1;

    /* In the PB Vibe LCD is flipped and the RAM addresses are decremented */
    lcd_send_cmd(R_HORIZ_RAM_ADDR_POS);
    lcd_send_data( (((LCD_HEIGHT-1)-y0) << 8) | ((LCD_HEIGHT-1)-y1));

    lcd_send_cmd(R_VERT_RAM_ADDR_POS);
    lcd_send_data( (((LCD_WIDTH -1)-x0) << 8) | ((LCD_WIDTH -1)-x1));

    lcd_send_cmd(R_RAM_ADDR_SET);
    lcd_send_data( (((LCD_WIDTH -1)-x0) << 8) | ((LCD_HEIGHT-1)-y0));

    /* start drawing */
    lcd_send_cmd(R_WRITE_DATA_2_GRAM);

    addr = &lcd_framebuffer[y0][x0];

    int c, r;
    for (r = 0; r < height; r++)
    {
        for (c = 0; c < width; c++)
        lcd_send_data_swapped(*addr++);
        addr += LCD_WIDTH - width;
    }
}

/* Update the display.
   This must be called after all other LCD 
   functions that change the display. */
void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}
