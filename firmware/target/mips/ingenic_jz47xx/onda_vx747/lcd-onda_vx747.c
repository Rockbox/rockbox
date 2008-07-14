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

#define my__gpio_as_lcd_16bit()			\
do {						\
	REG_GPIO_PXFUNS(2) = 0x001cffff;	\
	REG_GPIO_PXSELC(2) = 0x001cffff;	\
	REG_GPIO_PXPES(2) = 0x001cffff;		\
} while (0)


#define SLEEP(x) for(i=0; i<x; i++) asm("nop"); asm("nop");
#define DELAY    SLEEP(700000);
static void _display_pin_init(void)
{
    int i;
    my__gpio_as_lcd_16bit();
    __gpio_as_output(PIN_CS_N);
    __gpio_as_output(PIN_RESET_N);
    __gpio_clear_pin(PIN_CS_N);
    
	__gpio_set_pin(PIN_RESET_N);
	DELAY;		
	__gpio_clear_pin(PIN_RESET_N);
	DELAY;
	__gpio_set_pin(PIN_RESET_N);
	DELAY;
}

#define WAIT_ON_SLCD while(REG_SLCD_STATE & SLCD_STATE_BUSY);
#define SLCD_SET_DATA(x) WAIT_ON_SLCD; REG_SLCD_DATA = (x) | SLCD_DATA_RS_DATA;
#define SLCD_SET_COMMAND(x) WAIT_ON_SLCD; REG_SLCD_DATA = (x) | SLCD_DATA_RS_COMMAND;
#define SLCD_SEND_COMMAND(cmd,val) SLCD_SET_COMMAND(cmd); SLCD_SET_DATA(val);
static void _display_on(void)
{
    int i;
   
    SLCD_SEND_COMMAND(0x600, 1);
    SLEEP(700000);
    SLCD_SEND_COMMAND(0x600, 0);
    SLEEP(700000);
    SLCD_SEND_COMMAND(0x606, 0);
    
    SLCD_SEND_COMMAND(1, 0x100);
    SLCD_SEND_COMMAND(2, 0x100);
    SLCD_SEND_COMMAND(3, 0x1028);
    SLCD_SEND_COMMAND(8, 0x503);
    SLCD_SEND_COMMAND(9, 1);
    SLCD_SEND_COMMAND(0xB, 0x10);
    SLCD_SEND_COMMAND(0xC, 0);
    SLCD_SEND_COMMAND(0xF, 0);
    SLCD_SEND_COMMAND(7, 1);
    SLCD_SEND_COMMAND(0x10, 0x12);
    SLCD_SEND_COMMAND(0x11, 0x202);
    SLCD_SEND_COMMAND(0x12, 0x300);
    SLCD_SEND_COMMAND(0x20, 0x21e);
    SLCD_SEND_COMMAND(0x21, 0x202);
    SLCD_SEND_COMMAND(0x22, 0x100);
    SLCD_SEND_COMMAND(0x90, 0x8000);
    SLCD_SEND_COMMAND(0x100, 0x16b0);
    SLCD_SEND_COMMAND(0x101, 0x147);
    SLCD_SEND_COMMAND(0x102, 0x1bd);
    SLCD_SEND_COMMAND(0x103, 0x2f00);
    SLCD_SEND_COMMAND(0x107, 0);
    SLCD_SEND_COMMAND(0x110, 1);
    SLCD_SEND_COMMAND(0x200, 0); /* set cursor at x_start */
    SLCD_SEND_COMMAND(0x201, 0); /* set cursor at y_start */
    SLCD_SEND_COMMAND(0x210, 0); /* y_start*/
    SLCD_SEND_COMMAND(0x211, 239); /* y_end */
    SLCD_SEND_COMMAND(0x212, 0); /* x_start */
    SLCD_SEND_COMMAND(0x213, 399); /* x_end */
    SLCD_SEND_COMMAND(0x280, 0);
    SLCD_SEND_COMMAND(0x281, 6);
    SLCD_SEND_COMMAND(0x282, 0);
    SLCD_SEND_COMMAND(0x300, 0x101);
    SLCD_SEND_COMMAND(0x301, 0xb27);
    SLCD_SEND_COMMAND(0x302, 0x132a);
    SLCD_SEND_COMMAND(0x303, 0x2a13);
    SLCD_SEND_COMMAND(0x304, 0x270b);
    SLCD_SEND_COMMAND(0x305, 0x101);
    SLCD_SEND_COMMAND(0x306, 0x1205);
    SLCD_SEND_COMMAND(0x307, 0x512);
    SLCD_SEND_COMMAND(0x308, 5);
    SLCD_SEND_COMMAND(0x309, 3);
    SLCD_SEND_COMMAND(0x30a, 0xf04);
    SLCD_SEND_COMMAND(0x30b, 0xf00);
    SLCD_SEND_COMMAND(0x30c, 0xf);
    SLCD_SEND_COMMAND(0x30d, 0x40f);
    SLCD_SEND_COMMAND(0x30e, 0x300);
    SLCD_SEND_COMMAND(0x30f, 0x500);
    SLCD_SEND_COMMAND(0x400, 0x3100);
    SLCD_SEND_COMMAND(0x401, 1);
    SLCD_SEND_COMMAND(0x404, 0);
    SLCD_SEND_COMMAND(0x500, 0);
    SLCD_SEND_COMMAND(0x501, 0);
    SLCD_SEND_COMMAND(0x502, 0);
    SLCD_SEND_COMMAND(0x503, 0);
    SLCD_SEND_COMMAND(0x504, 0);
    SLCD_SEND_COMMAND(0x505, 0);
    SLCD_SEND_COMMAND(0x606, 0);
    SLCD_SEND_COMMAND(0x6f0, 0);
    SLCD_SEND_COMMAND(0x7f0, 0x5420);
    SLCD_SEND_COMMAND(0x7f3, 0x288a);
    SLCD_SEND_COMMAND(0x7f4, 0x22);
    SLCD_SEND_COMMAND(0x7f5, 1);
    SLCD_SEND_COMMAND(0x7f0, 0);
    
    SLCD_SEND_COMMAND(7, 0x173);
    SLEEP(3500000);
    SLCD_SEND_COMMAND(7, 0x171);
    SLEEP(3500000);
    SLCD_SEND_COMMAND(7, 0x173);
    SLEEP(3500000);
}

static void _set_lcd_bus(void)
{
    REG_LCD_CFG &= ~LCD_CFG_LCDPIN_MASK;
	REG_LCD_CFG |= LCD_CFG_LCDPIN_SLCD;
    
    REG_SLCD_CFG = (SLCD_CFG_BURST_8_WORD | SLCD_CFG_DWIDTH_16 | SLCD_CFG_CWIDTH_16BIT
                   | SLCD_CFG_CS_ACTIVE_LOW | SLCD_CFG_RS_CMD_LOW | SLCD_CFG_CLK_ACTIVE_FALLING
                   | SLCD_CFG_TYPE_PARALLEL);
    
    REG_SLCD_CTRL = SLCD_CTRL_DMA_EN;
}

static void _set_lcd_clock(void)
{
	unsigned int val;
	int pll_div;
    
    __cpm_stop_lcd();
	pll_div = ( REG_CPM_CPCCR & CPM_CPCCR_PCS ); /* clock source,0:pllout/2 1: pllout */
	pll_div = pll_div ? 1 : 2 ;
	val = ( __cpm_get_pllout()/pll_div ) / 336000000;
	val--;
	if ( val > 0x1ff )
    {
		//printf("CPM_LPCDR too large, set it to 0x1ff\n");
		val = 0x1ff;
	}
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
    _display_on();
}

void lcd_set_target(short x, short y, short width, short height)
{
    SLCD_SEND_COMMAND(0x210, y); /* y_start */
    SLCD_SEND_COMMAND(0x211, y+height); /* y_end */
    SLCD_SEND_COMMAND(0x212, x); /* x_start */
    SLCD_SEND_COMMAND(0x213, x+width); /* x_end */
    SLCD_SEND_COMMAND(0x200, x); /* set cursor at x_start */
    SLCD_SEND_COMMAND(0x201, y); /* set cursor at y_start */
    SLCD_SET_COMMAND(0x202); /* write data? */
}

void lcd_on(void)
{
    _display_on();
}

void lcd_off(void)
{
    return;
}
