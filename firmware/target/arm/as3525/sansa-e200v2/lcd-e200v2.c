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
#include "config.h"

#include "cpu.h"
#include "lcd.h"
#include "kernel.h"
#include "thread.h"
#include <string.h>
#include <stdlib.h>
#include "file.h"
#include "debug.h"
#include "system.h"
#include "font.h"
#include "bidi.h"

static bool display_on = false; /* is the display turned on? */
static bool display_flipped = false;
static int xoffset = 0; /* needed for flip */

/* register defines */
#define R_START_OSC             0x00
#define R_DRV_OUTPUT_CONTROL    0x01
#define R_DRV_WAVEFORM_CONTROL  0x02
#define R_ENTRY_MODE            0x03
#define R_COMPARE_REG1          0x04
#define R_COMPARE_REG2          0x05

#define R_DISP_CONTROL1     0x07
#define R_DISP_CONTROL2     0x08
#define R_DISP_CONTROL3     0x09

#define R_FRAME_CYCLE_CONTROL 0x0b
#define R_EXT_DISP_IF_CONTROL 0x0c

#define R_POWER_CONTROL1    0x10
#define R_POWER_CONTROL2    0x11
#define R_POWER_CONTROL3    0x12
#define R_POWER_CONTROL4    0x13

#define R_RAM_ADDR_SET  0x21
#define R_WRITE_DATA_2_GRAM 0x22

#define R_GAMMA_FINE_ADJ_POS1   0x30
#define R_GAMMA_FINE_ADJ_POS2   0x31
#define R_GAMMA_FINE_ADJ_POS3   0x32
#define R_GAMMA_GRAD_ADJ_POS    0x33

#define R_GAMMA_FINE_ADJ_NEG1   0x34
#define R_GAMMA_FINE_ADJ_NEG2   0x35
#define R_GAMMA_FINE_ADJ_NEG3   0x36
#define R_GAMMA_GRAD_ADJ_NEG    0x37

#define R_GAMMA_AMP_ADJ_RES_POS     0x38
#define R_GAMMA_AMP_AVG_ADJ_RES_NEG 0x39 

#define R_GATE_SCAN_POS         0x40
#define R_VERT_SCROLL_CONTROL   0x41
#define R_1ST_SCR_DRV_POS       0x42
#define R_2ND_SCR_DRV_POS       0x43
#define R_HORIZ_RAM_ADDR_POS    0x44
#define R_VERT_RAM_ADDR_POS     0x45

#define LCD_CMD  (*(volatile unsigned short *)0xf0000000)
#define LCD_DATA (*(volatile unsigned short *)0xf0000002)

#define R_ENTRY_MODE_HORZ 0x7030
#define R_ENTRY_MODE_VERT 0x7038

/* TODO: Implement this function */
static void lcd_delay(int x)
{
    (void)x;
}

/* DBOP initialisation, do what OF does */
static void ams3525_dbop_init(void)
{

    /* TODO: More... */

    DBOP_TIMPOL_01 = 0xe167e167;
    DBOP_TIMPOL_23 = 0xe167006e;
    DBOP_CTRL = 0x41008;

    GPIOB_AFSEL = 0xfc;
    GPIOC_AFSEL = 0xff;

    DBOP_TIMPOL_23 = 0x6000e;
    DBOP_CTRL = 0x51008;
    DBOP_TIMPOL_01 = 0x6e167;
    DBOP_TIMPOL_23 = 0xa167e06f;

    /* TODO: More... */
}

static void lcd_write_reg(int reg, int value)
{
    DBOP_CTRL &= ~(1<<14);

    DBOP_TIMPOL_23 = 0xa167006e;

    DBOP_DOUT = reg;

    /* Wait for fifo to empty */
    while ((DBOP_STAT & (1<<10)) == 0);

    DBOP_TIMPOL_23 = 0xa167e06f;


    DBOP_DOUT = value;

    /* Wait for fifo to empty */
    while ((DBOP_STAT & (1<<10)) == 0);
}

void lcd_write_data(const fb_data* p_bytes, int count)
{
    (void)p_bytes;
    (void)count;
}


/*** hardware configuration ***/

void lcd_set_contrast(int val)
{
    (void)val;
}

void lcd_set_invert_display(bool yesno)
{
    (void)yesno;
}

static void flip_lcd(bool yesno)
{
    (void)yesno;
}


/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
    display_flipped = yesno;
    xoffset = yesno ? 4 : 0;

    if (display_on)
        flip_lcd(yesno);
}

static void _display_on(void)
{
    /* Initialisation the display the same way as the original firmware */

    lcd_write_reg(R_START_OSC, 0x0001); /* Start Oscilation */

    lcd_write_reg(R_DRV_OUTPUT_CONTROL, 0x011b); /* 220 lines, GS=0, SS=1 */

    /* B/C = 1: n-line inversion form
     * EOR = 1: polarity inversion occurs by applying an EOR to odd/even
     *          frame select signal and an n-line inversion signal.
     * FLD = 01b: 1 field interlaced scan, external display iface */
    lcd_write_reg(R_DRV_WAVEFORM_CONTROL, 0x0700);

    /* Address counter updated in horizontal direction; left to right;
     * vertical increment horizontal increment.
     * data format for 8bit transfer or spi = 65k (5,6,5) */
    lcd_write_reg(R_ENTRY_MODE, 0x0030);

    /* Replace data on writing to GRAM */
    lcd_write_reg(R_COMPARE_REG1, 0);
    lcd_write_reg(R_COMPARE_REG2, 0);

    lcd_write_reg(R_DISP_CONTROL1, 0x0000);  /* GON = 0, DTE = 0, D1-0 = 00b */

    /* Front porch lines: 2; Back porch lines: 2; */
    lcd_write_reg(R_DISP_CONTROL2, 0x0203);

    /* Scan cycle = 0 frames */
    lcd_write_reg(R_DISP_CONTROL3, 0x0000);

    /* 16 clocks */
    lcd_write_reg(R_FRAME_CYCLE_CONTROL, 0x0000);

    /* 18-bit RGB interface (one transfer/pixel)
     * internal clock operation;
     * System interface/VSYNC interface */
    lcd_write_reg(R_EXT_DISP_IF_CONTROL, 0x0000);


    /* zero everything*/
    lcd_write_reg(R_POWER_CONTROL1, 0x0000); /* STB = 0, SLP = 0 */

    lcd_delay(10);

    /* initialise power supply */

    /* DC12-10 = 000b: Step-up1 = clock/8,
     * DC02-00 = 000b: Step-up2 = clock/16,
     * VC2-0   = 010b: VciOUT = 0.87 * VciLVL */
    lcd_write_reg(R_POWER_CONTROL2, 0x0002); 

    /* VRH3-0 = 1000b: Vreg1OUT = REGP * 1.90 */
    lcd_write_reg(R_POWER_CONTROL3, 0x0008);

    lcd_delay(40);
    
    lcd_write_reg(R_POWER_CONTROL4, 0x0000); /* VCOMG = 0 */

    /* This register is unknown */
    lcd_write_reg(0x56, 0x80f);


    lcd_write_reg(R_POWER_CONTROL1, 0x4140);

    lcd_delay(10);

    lcd_write_reg(R_POWER_CONTROL2, 0x0000); 
    lcd_write_reg(R_POWER_CONTROL3, 0x0013); 

    lcd_delay(20);

    lcd_write_reg(R_POWER_CONTROL4, 0x6d0e);

    lcd_delay(20);

    lcd_write_reg(R_POWER_CONTROL4, 0x6d0e);

    lcd_write_reg(R_GAMMA_FINE_ADJ_POS1, 0x0002);
    lcd_write_reg(R_GAMMA_FINE_ADJ_POS2, 0x0707);
    lcd_write_reg(R_GAMMA_FINE_ADJ_POS3, 0x0182);
    lcd_write_reg(R_GAMMA_GRAD_ADJ_POS, 0x0203);
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG1, 0x0706);
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG2, 0x0006);
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG3, 0x0706);
    lcd_write_reg(R_GAMMA_GRAD_ADJ_NEG, 0x0000);
    lcd_write_reg(R_GAMMA_AMP_ADJ_RES_POS, 0x030f);
    lcd_write_reg(R_GAMMA_AMP_AVG_ADJ_RES_NEG, 0x0f08);


    lcd_write_reg(R_RAM_ADDR_SET, 0);
    lcd_write_reg(R_GATE_SCAN_POS, 0);
    lcd_write_reg(R_VERT_SCROLL_CONTROL, 0);
    
    lcd_write_reg(R_1ST_SCR_DRV_POS, 219 << 8);
    lcd_write_reg(R_2ND_SCR_DRV_POS, 219 << 8);

    lcd_write_reg(R_HORIZ_RAM_ADDR_POS, 175 << 8);
    lcd_write_reg(R_VERT_RAM_ADDR_POS, 219 << 8);


    lcd_write_reg(R_DISP_CONTROL1, 0x0037);

    display_on=true;  /* must be done before calling lcd_update() */
    lcd_update();
}

/* LCD init */
void lcd_init_device(void)
{
  ams3525_dbop_init();

  /* Init GPIOs the same as the OF */

  GPIOA_DIR |= (1<<5);
  GPIOA_PIN(5) = 0;

  GPIOA_PIN(3) = (1<<3);

  GPIOA_DIR |= (3<<3);

  GPIOA_PIN(3) = (1<<3);

  GPIOA_PIN(4) = 0;  //c80b0040 := 0;

  GPIOA_DIR |= (1<<7);
  GPIOA_PIN(7) = 0;

  CCU_IO &= ~(1<<2);
  CCU_IO &= ~(1<<3);

  GPIOD_DIR |= (1<<7);

#if 0
  /* TODO: This code is conditional on a variable in the OF init, we need to
           work out what it means */

  GPIOD_PIN(7) = (1<<7);
  GPIOD_DIR |= (1<<7);
#endif

  lcd_delay(1);

  GPIOA_PIN(5) = (1<<5);

  lcd_delay(1);

    _display_on();
}

void lcd_enable(bool on)
{
    if(display_on!=on)
    {
        if(on)
        {
            _display_on();
            lcd_call_enable_hook();
        }
        else
        {
            /* TODO: Implement off sequence */
            display_on=false;
        }
    }
}

bool lcd_enabled(void)
{
    return display_on;
}

/*** update functions ***/

/* Performance function to blit a YUV bitmap directly to the LCD
 * src_x, src_y, width and height should be even
 * x, y, width and height have to be within LCD bounds
 */
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
void lcd_update(void) ICODE_ATTR;
void lcd_update(void)
{
    if(display_on){
        /* TODO */
    }
}


/* Update a fraction of the display. */
void lcd_update_rect(int, int, int, int) ICODE_ATTR;
void lcd_update_rect(int x, int y, int width, int height)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;

    if(display_on) {
        /* TODO */
    }
}
