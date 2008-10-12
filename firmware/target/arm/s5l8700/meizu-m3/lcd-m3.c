/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Denes Balatoni
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

#include "hwcompat.h"
#include "kernel.h"
#include "lcd.h"
#include "system.h"
#include "cpu.h"
#include "inttypes.h"
#include "s5l8700.h"

/*** definitions ***/


/** globals **/
static uint8_t lcd_type;
static int xoffset; /* needed for flip */

/*** hardware configuration ***/

int lcd_default_contrast(void)
{
    return 0x1f;
}

void lcd_set_contrast(int val)
{
}

void lcd_set_invert_display(bool yesno)
{
}

/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
    /* TODO: flip mode isn't working.  The commands in the else part of
       this function are how the original firmware inits the LCD */

    if (yesno)
    {
        xoffset = 132 - LCD_WIDTH; /* 132 colums minus the 128 we have */
    }
    else 
    {
        xoffset = 0;
    }
}

static void lcd_sleep(uint32_t t)
{
    volatile uint32_t i;
    for(i=0;i<t;++i) t=t;
}

static uint8_t lcd_readdata()
{
        LCD_RDATA = 0;
        lcd_sleep(64);
        return (LCD_DBUFF/* & 0xff*/);
}

static void lcd_writereg(uint32_t reg, uint32_t data)
{
        LCD_WCMD = reg >> 8;
	LCD_WCMD = reg & 0xff;
	LCD_WDATA = data >> 8;
	LCD_WDATA = data & 0xff;
}

void lcd_on() {
    if (lcd_type == 1) {
        LCD_WCMD = 0x29;    
    } else {
	lcd_writereg(0x7, 0x21);
	lcd_writereg(0x12, 0x137);
	lcd_sleep(70000);
	lcd_writereg(0x7, 0x21);
	lcd_writereg(0x12, 0x1137);
	lcd_sleep(700000);
	lcd_writereg(0x7, 0x233);
    }
}

void lcd_off() {
    /* FIXME wait for DMA to finnish */
    if (lcd_type == 1) {
        LCD_WCMD = 0x28;
	LCD_WDATA = 0;
    } else {
    
    }
}

/* LCD init */
void lcd_init_device(void)
{
    uint8_t data[5];

/* init basic things */
    PWRCON &= ~0x800;
    PCON_ASRAM = 0x2;
    PCON7 = 0x12222233;

    LCD_CON = 0xca0;
    LCD_PHTIME = 0;
    LCD_INTCON = 0;
    LCD_RST_TIME = 0x7ff;

/* detect lcd type */
    LCD_WCMD = 0x1;
    lcd_sleep(166670);
    LCD_WCMD = 0x11;
    lcd_sleep(2000040);
    lcd_readdata();
    LCD_WCMD = 0x4;
    lcd_sleep(100);
    data[0]=lcd_readdata();
    data[1]=lcd_readdata();
    data[2]=lcd_readdata();
    data[3]=lcd_readdata();
    data[4]=lcd_readdata();
    
    lcd_type=0;
    if (((data[1]==0x38) && ((data[2] & 0xf0) == 0x80)) ||
        ((data[2]==0x38) && ((data[3] & 0xf0) == 0x80)))
        lcd_type=1;
    
/* init lcd */
    if (lcd_type == 1) {
        LCD_WCMD = 0x3a;
        LCD_WDATA = 0x6;
        LCD_WCMD = 0xab;
        LCD_WCMD = 0x35;
        LCD_WDATA = 0;
        LCD_WCMD=0x13;
        LCD_WCMD = 0x2a;
        LCD_WDATA = 0;
        LCD_WDATA = 0;
        LCD_WCMD = 0x2b;
        LCD_WDATA = 0;
        LCD_WDATA = 0;
        LCD_WCMD = 0x29;
    } else {
        LCD_WCMD = 0x0;
        LCD_WCMD = 0x0;
        LCD_WCMD = 0x0;
        LCD_WCMD = 0x0;
        lcd_sleep(700000);
	lcd_writereg(0xa4, 0x1);
        lcd_sleep(1100000);
	lcd_writereg(0x1, 0x100);
	lcd_writereg(0x2, 0x300);
	lcd_writereg(0x3, 0x9230);
	lcd_writereg(0x8, 0x404);
	lcd_writereg(0xe, 0x10);
	lcd_writereg(0x70, 0x1000);
	lcd_writereg(0x71, 0x1);
	lcd_writereg(0x30, 0x2);
	lcd_writereg(0x31, 0x400);
	lcd_writereg(0x32, 0x7);
	lcd_writereg(0x33, 0x500);
	lcd_writereg(0x34, 0x7);
	lcd_writereg(0x35, 0x703);
	lcd_writereg(0x36, 0x507);
	lcd_writereg(0x37, 0x5);
	lcd_writereg(0x38, 0x1404);
	lcd_writereg(0x39, 0xe);
	lcd_writereg(0x40, 0x202);
	lcd_writereg(0x41, 0x3);
	lcd_writereg(0x42, 0x0);
	lcd_writereg(0x43, 0x200);
	lcd_writereg(0x44, 0x707);
	lcd_writereg(0x45, 0x407);
	lcd_writereg(0x46, 0x505);
	lcd_writereg(0x47, 0x2);
	lcd_writereg(0x48, 0x4);
	lcd_writereg(0x49, 0x4);
	lcd_writereg(0x60, 0x202);
	lcd_writereg(0x61, 0x3);
	lcd_writereg(0x62, 0x0);
	lcd_writereg(0x63, 0x200);
	lcd_writereg(0x64, 0x707);
	lcd_writereg(0x65, 0x407);
	lcd_writereg(0x66, 0x505);
	lcd_writereg(0x67, 0x2);
	lcd_writereg(0x68, 0x4);
	lcd_writereg(0x69, 0x4);
	lcd_writereg(0x7, 0x1);
	lcd_writereg(0x18, 0x1);
	lcd_writereg(0x10, 0x1690);
	lcd_writereg(0x11, 0x100);
	lcd_writereg(0x12, 0x117);
	lcd_writereg(0x13, 0xf80);
	lcd_writereg(0x12, 0x137);
	lcd_writereg(0x20, 0x0);
	lcd_writereg(0x21, 0x0);
	lcd_writereg(0x50, 0x0);
	lcd_writereg(0x51, 0xaf);
	lcd_writereg(0x52, 0x0);
	lcd_writereg(0x53, 0x83);
	lcd_writereg(0x90, 0x0);
	lcd_writereg(0x91, 0x0);
	lcd_writereg(0x92, 0x0);
	lcd_writereg(0x98, 0x0);
	lcd_writereg(0x99, 0x903);
	lcd_writereg(0x9a, 0x502);
	lcd_writereg(0x9b, 0x300);
	LCD_WCMD = 0x0;
	LCD_WCMD = 0x22;
	lcd_sleep(700000);
	lcd_on();
    }
}

/*** Update functions ***/

/* Performance function that works with an external buffer
   note that by and bheight are in 8-pixel units! */
void lcd_blit_mono(const unsigned char *data, int x, int by, int width,
                   int bheight, int stride)
{
    /* Copy display bitmap to hardware */
    while (bheight--)
    {
    }
}


/* Performance function that works with an external buffer
   note that by and bheight are in 8-pixel units! */
void lcd_blit_grey_phase_blit(unsigned char *values, unsigned char *phases,
                         int x, int by, int width, int bheight, int stride)
{
    (void)values;
    (void)phases;
    (void)x;
    (void)by;
    (void)width;
    (void)bheight;
    (void)stride;
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void) ICODE_ATTR;
void lcd_update(void)
{
    int i;
    fb_data *p;

    /* Copy display bitmap to hardware */
    if (lcd_type == 1) {
        LCD_WCMD = 0x2a;
	LCD_WDATA = 0;
	LCD_WDATA = 0;
	LCD_WDATA = 0;
	LCD_WDATA = 0xaf;
        LCD_WCMD = 0x2b;
	LCD_WDATA = 0;
	LCD_WDATA = 0;
	LCD_WDATA = 0;
	LCD_WDATA = 0x83;
	LCD_WCMD = 0x2c;
    } else {
	lcd_writereg(0x20, 0x0);
	lcd_writereg(0x21, 0x0);
	LCD_WCMD = 0;
	LCD_WCMD = 0x22;
    }
    for(p=&lcd_framebuffer[0][0], i=0;i<LCD_WIDTH*LCD_FBHEIGHT;++i, ++p) {
        LCD_WDATA = RGB_UNPACK_RED(*p)<<3;
        LCD_WDATA = RGB_UNPACK_GREEN(*p)<<2;
        LCD_WDATA = RGB_UNPACK_BLUE(*p)<<3;
        lcd_sleep(3); /* if data is sent too fast to lcdif, machine freezes */
    }
}

/* Update a fraction of the display. */
void lcd_update_rect(int, int, int, int) ICODE_ATTR;
void lcd_update_rect(int x, int y, int width, int height)
{
    lcd_update();
}
