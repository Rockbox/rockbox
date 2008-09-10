/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
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
#include "jz4740.h"
#include "lcd-target.h"

#define PIN_CS_N 	(32*1+17) /* Chip select */
#define PIN_RESET_N (32*1+18) /* Reset */
#define PIN_UNK_N   (32*2+19)

#define my__gpio_as_lcd_16bit()			\
do {						\
	REG_GPIO_PXFUNS(2) = 0x0014ffff;	\
	REG_GPIO_PXSELC(2) = 0x0014ffff;	\
	REG_GPIO_PXPES(2) = 0x0014ffff;		\
} while (0)


#define SLEEP(x) for(i=0; i<x; i++) asm("nop"); asm("nop");
#define DELAY    SLEEP(700000);
static void _display_pin_init(void)
{
    int i;
    my__gpio_as_lcd_16bit();
    __gpio_as_output(PIN_UNK_N);
    __gpio_set_pin(PIN_UNK_N);
    __gpio_as_output(PIN_CS_N);
    __gpio_as_output(PIN_RESET_N);
    DELAY; /* delay_ms(10); */
    
    __gpio_clear_pin(PIN_CS_N);
    DELAY; /* delay_ms(10); */
    
	__gpio_set_pin(PIN_RESET_N);
	DELAY; /* delay_ms(10); */	
	__gpio_clear_pin(PIN_RESET_N);
	DELAY; /* delay_ms(10); */
	__gpio_set_pin(PIN_RESET_N);
	DELAY; /* delay_ms(10); */
}

#define WAIT_ON_SLCD while(REG_SLCD_STATE & SLCD_STATE_BUSY);
#define SLCD_SET_DATA(x) REG_SLCD_DATA = (x) | SLCD_DATA_RS_DATA;
#define SLCD_SET_COMMAND(x) REG_SLCD_DATA = (x) | SLCD_DATA_RS_COMMAND;

#define SLCD_SEND_COMMAND(cmd,val) \
         __gpio_clear_pin(PIN_UNK_N); \
         SLCD_SET_COMMAND(cmd); \
         WAIT_ON_SLCD; \
         __gpio_set_pin(PIN_UNK_N); \
         SLCD_SET_DATA(val); \
         WAIT_ON_SLCD;
         
static void _display_init(void)
{
    int i;
    
    SLCD_SEND_COMMAND(0xE3, 0x8);
    SLCD_SEND_COMMAND(0xE4, 0x1411);
    SLCD_SEND_COMMAND(0xE5, 0x8000);
    SLCD_SEND_COMMAND(0x0, 0x1);
    DELAY; /* delay_ms(10); */
    
    SLCD_SEND_COMMAND(0x1, 0x100);
    SLCD_SEND_COMMAND(0x2, 0x400);
    SLCD_SEND_COMMAND(0x3, 0x1028);
    SLCD_SEND_COMMAND(0x4, 0);
    SLCD_SEND_COMMAND(0x8, 0x202);
    SLCD_SEND_COMMAND(0x9, 0);
    SLCD_SEND_COMMAND(0xA, 0);
    SLCD_SEND_COMMAND(0xC, 0);
    SLCD_SEND_COMMAND(0xD, 0);
    SLCD_SEND_COMMAND(0xF, 0);
    SLCD_SEND_COMMAND(0x10, 0);
    SLCD_SEND_COMMAND(0x11, 0x7);
    SLCD_SEND_COMMAND(0x12, 0);
    SLCD_SEND_COMMAND(0x13, 0);
    SLCD_SEND_COMMAND(0x10, 0x17B0);
    SLCD_SEND_COMMAND(0x11, 0x4);
    SLCD_SEND_COMMAND(0x12, 0x13C);
    SLCD_SEND_COMMAND(0x13, 0x1B00);
    SLCD_SEND_COMMAND(0x29, 0x16);
    SLCD_SEND_COMMAND(0x20, 0);
    SLCD_SEND_COMMAND(0x21, 0);
    SLCD_SEND_COMMAND(0x2B, 0x20);
    SLCD_SEND_COMMAND(0x30, 0);
    SLCD_SEND_COMMAND(0x31, 0x403);
    SLCD_SEND_COMMAND(0x32, 0x400);
    SLCD_SEND_COMMAND(0x35, 0x5);
    SLCD_SEND_COMMAND(0x36, 0x6);
    SLCD_SEND_COMMAND(0x37, 0x606);
    SLCD_SEND_COMMAND(0x38, 0x106);
    SLCD_SEND_COMMAND(0x39, 0x7);
    SLCD_SEND_COMMAND(0x3C, 0x700);
    SLCD_SEND_COMMAND(0x3D, 0x707);
    SLCD_SEND_COMMAND(0x50, 0);
    SLCD_SEND_COMMAND(0x51, 239);
    SLCD_SEND_COMMAND(0x52, 0);
    SLCD_SEND_COMMAND(0x53, 319);
    SLCD_SEND_COMMAND(0x60, 0x2700);
    SLCD_SEND_COMMAND(0x61, 0x1);
    SLCD_SEND_COMMAND(0x6A, 0);
    SLCD_SEND_COMMAND(0x80, 0);
    SLCD_SEND_COMMAND(0x81, 0);
    SLCD_SEND_COMMAND(0x82, 0);
    SLCD_SEND_COMMAND(0x83, 0);
    SLCD_SEND_COMMAND(0x84, 0);
    SLCD_SEND_COMMAND(0x85, 0);
    SLCD_SEND_COMMAND(0x90, 0x10);
    SLCD_SEND_COMMAND(0x92, 0);
    SLCD_SEND_COMMAND(0x93, 0x3);
    SLCD_SEND_COMMAND(0x95, 0x110);
    SLCD_SEND_COMMAND(0x97, 0);
    SLCD_SEND_COMMAND(0x98, 0);
    SLCD_SEND_COMMAND(0x7, 0x173);
    
    __gpio_clear_pin(PIN_UNK_N);
    SLCD_SET_COMMAND(0x22);
    WAIT_ON_SLCD;
    __gpio_set_pin(PIN_UNK_N);
}

static void _display_on(void)
{
}

static void _display_off(void)
{
}

static void _set_lcd_bus(void)
{
    REG_LCD_CFG &= ~LCD_CFG_LCDPIN_MASK;
	REG_LCD_CFG |= LCD_CFG_LCDPIN_SLCD;
    
    REG_SLCD_CFG = (SLCD_CFG_BURST_4_WORD | SLCD_CFG_DWIDTH_18 | SLCD_CFG_CWIDTH_18BIT
                   | SLCD_CFG_CS_ACTIVE_LOW | SLCD_CFG_RS_CMD_LOW | SLCD_CFG_CLK_ACTIVE_FALLING
                   | SLCD_CFG_TYPE_PARALLEL);
    
    REG_SLCD_CTRL = SLCD_CTRL_DMA_EN;
}

static void _set_lcd_clock(void)
{
	unsigned int val;
	int pll_div;
    
    __cpm_stop_lcd();
	pll_div = ( REG_CPM_CPCCR & CPM_CPCCR_PCS ); /* clock source, 0:pllout/2 1: pllout */
	pll_div = pll_div ? 1 : 2 ;
	val = ( __cpm_get_pllout()/pll_div ) / 336000000;
	val--;
	if ( val > 0x1ff )
		val = 0x1ff; /* CPM_LPCDR is too large, set it to 0x1ff */
	__cpm_set_pixdiv(val);
    __cpm_start_lcd();
}

void lcd_init_controller(void)
{
    int i;
    _display_pin_init();
    _set_lcd_bus();
    _set_lcd_clock();
    SLEEP(1000);
    _display_init();
}

void lcd_set_target(short x, short y, short width, short height)
{
    SLCD_SEND_COMMAND(0x50, y);
    SLCD_SEND_COMMAND(0x51, y+height-1);
    SLCD_SEND_COMMAND(0x52, x);
    SLCD_SEND_COMMAND(0x53, x+width-1);
    /* TODO */
}

void lcd_on(void)
{
    _display_on();
}

void lcd_off(void)
{
    _display_off();
}
