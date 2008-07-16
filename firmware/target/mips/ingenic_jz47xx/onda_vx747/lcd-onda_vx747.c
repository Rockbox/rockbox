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
#include "r61509.h"
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
static void _display_init(void)
{
    int i;
   
    SLCD_SEND_COMMAND(REG_SOFT_RESET, SOFT_RESET(1));
    SLEEP(700000);
    SLCD_SEND_COMMAND(REG_SOFT_RESET, SOFT_RESET(0));
    SLEEP(700000);
    SLCD_SEND_COMMAND(REG_ENDIAN_CTRL, 0);
    
    SLCD_SEND_COMMAND(REG_DRIVER_OUTPUT, 0x100);
    SLCD_SEND_COMMAND(REG_LCD_DR_WAVE_CTRL, 0x100);
    SLCD_SEND_COMMAND(REG_ENTRY_MODE, (ENTRY_MODE_BGR | ENTRY_MODE_VID | ENTRY_MODE_HID));
    SLCD_SEND_COMMAND(REG_DISP_CTRL2, 0x503);
    SLCD_SEND_COMMAND(REG_DISP_CTRL3, 1);
    SLCD_SEND_COMMAND(REG_LPCTRL, 0x10);
    SLCD_SEND_COMMAND(REG_EXT_DISP_CTRL1, 0);
    SLCD_SEND_COMMAND(REG_EXT_DISP_CTRL2, 0);
    SLCD_SEND_COMMAND(REG_DISP_CTRL1, DISP_CTRL1_D(1));
    SLCD_SEND_COMMAND(REG_PAN_INTF_CTRL1, 0x12);
    SLCD_SEND_COMMAND(REG_PAN_INTF_CTRL2, 0x202);
    SLCD_SEND_COMMAND(REG_PAN_INTF_CTRL3, 0x300);
    SLCD_SEND_COMMAND(REG_PAN_INTF_CTRL4, 0x21e);
    SLCD_SEND_COMMAND(REG_PAN_INTF_CTRL5, 0x202);
    SLCD_SEND_COMMAND(REG_PAN_INTF_CTRL6, 0x100);
    SLCD_SEND_COMMAND(REG_FRM_MRKR_CTRL, 0x8000);
    SLCD_SEND_COMMAND(REG_PWR_CTRL1, (PWR_CTRL1_SAPE | PWR_CTRL1_BT(6) | PWR_CTRL1_APE | PWR_CTRL1_AP(3)));
    SLCD_SEND_COMMAND(REG_PWR_CTRL2, 0x147);
    SLCD_SEND_COMMAND(REG_PWR_CTRL3, 0x1bd);
    SLCD_SEND_COMMAND(REG_PWR_CTRL4, 0x2f00);
    SLCD_SEND_COMMAND(REG_PWR_CTRL5, 0);
    SLCD_SEND_COMMAND(REG_PWR_CTRL6, 1);
    SLCD_SEND_COMMAND(REG_RAM_HADDR_SET, 0); /* set cursor at x_start */
    SLCD_SEND_COMMAND(REG_RAM_VADDR_SET, 0); /* set cursor at y_start */
    SLCD_SEND_COMMAND(REG_RAM_HADDR_START, 0); /* y_start*/
    SLCD_SEND_COMMAND(REG_RAM_HADDR_END, 239); /* y_end */
    SLCD_SEND_COMMAND(REG_RAM_VADDR_START, 0); /* x_start */
    SLCD_SEND_COMMAND(REG_RAM_VADDR_END, 399); /* x_end */
    SLCD_SEND_COMMAND(REG_RW_NVM, 0);
    SLCD_SEND_COMMAND(REG_VCOM_HVOLTAGE1, 6);
    SLCD_SEND_COMMAND(REG_VCOM_HVOLTAGE2, 0);
    SLCD_SEND_COMMAND(REG_GAMMA_CTRL1, 0x101);
    SLCD_SEND_COMMAND(REG_GAMMA_CTRL2, 0xb27);
    SLCD_SEND_COMMAND(REG_GAMMA_CTRL3, 0x132a);
    SLCD_SEND_COMMAND(REG_GAMMA_CTRL4, 0x2a13);
    SLCD_SEND_COMMAND(REG_GAMMA_CTRL5, 0x270b);
    SLCD_SEND_COMMAND(REG_GAMMA_CTRL6, 0x101);
    SLCD_SEND_COMMAND(REG_GAMMA_CTRL7, 0x1205);
    SLCD_SEND_COMMAND(REG_GAMMA_CTRL8, 0x512);
    SLCD_SEND_COMMAND(REG_GAMMA_CTRL9, 5);
    SLCD_SEND_COMMAND(REG_GAMMA_CTRL10, 3);
    SLCD_SEND_COMMAND(REG_GAMMA_CTRL11, 0xf04);
    SLCD_SEND_COMMAND(REG_GAMMA_CTRL12, 0xf00);
    SLCD_SEND_COMMAND(REG_GAMMA_CTRL13, 0xf);
    SLCD_SEND_COMMAND(REG_GAMMA_CTRL14, 0x40f);
    SLCD_SEND_COMMAND(0x30e, 0x300);
    SLCD_SEND_COMMAND(0x30f, 0x500);
    SLCD_SEND_COMMAND(REG_BIMG_NR_LINE, 0x3100);
    SLCD_SEND_COMMAND(REG_BIMG_DISP_CTRL, 1);
    SLCD_SEND_COMMAND(REG_BIMG_VSCROLL_CTRL, 0);
    SLCD_SEND_COMMAND(REG_PARTIMG1_POS, 0);
    SLCD_SEND_COMMAND(REG_PARTIMG1_RAM_START, 0);
    SLCD_SEND_COMMAND(REG_PARTIMG1_RAM_END, 0);
    SLCD_SEND_COMMAND(REG_PARTIMG2_POS, 0);
    SLCD_SEND_COMMAND(REG_PARTIMG2_RAM_START, 0);
    SLCD_SEND_COMMAND(REG_PARTIMG2_RAM_END, 0);
    SLCD_SEND_COMMAND(REG_ENDIAN_CTRL, 0);
    SLCD_SEND_COMMAND(REG_NVM_ACCESS_CTRL, 0);
    SLCD_SEND_COMMAND(0x7f0, 0x5420);
    SLCD_SEND_COMMAND(0x7f3, 0x288a);
    SLCD_SEND_COMMAND(0x7f4, 0x22);
    SLCD_SEND_COMMAND(0x7f5, 1);
    SLCD_SEND_COMMAND(0x7f0, 0);
    
    /* LCD ON: */
    SLCD_SEND_COMMAND(REG_DISP_CTRL1, (DISP_CTRL1_BASEE | DISP_CTRL1_VON
                                      | DISP_CTRL1_GON | DISP_CTRL1_DTE | DISP_CTRL1_D(3))
                                      );
    SLEEP(3500000);
    SLCD_SEND_COMMAND(REG_DISP_CTRL1, (DISP_CTRL1_BASEE | DISP_CTRL1_VON
                                      | DISP_CTRL1_GON | DISP_CTRL1_DTE | DISP_CTRL1_D(2))
                                      );
    SLEEP(3500000);
    SLCD_SEND_COMMAND(REG_DISP_CTRL1, (DISP_CTRL1_BASEE | DISP_CTRL1_VON
                                      | DISP_CTRL1_GON | DISP_CTRL1_DTE | DISP_CTRL1_D(3))
                                      );
    SLEEP(3500000);
}

static void _display_on(void)
{
    int i;
    SLCD_SEND_COMMAND(REG_PWR_CTRL1, (PWR_CTRL1_SAPE | PWR_CTRL1_BT(6) | PWR_CTRL1_APE | PWR_CTRL1_AP(3)));
    SLEEP(3500000);
    SLCD_SEND_COMMAND(REG_DISP_CTRL1, (DISP_CTRL1_VON | DISP_CTRL1_GON
                                      | DISP_CTRL1_D(1))
                                      );
    SLEEP(3500000);
    SLCD_SEND_COMMAND(REG_DISP_CTRL1, (DISP_CTRL1_VON | DISP_CTRL1_GON
                                      | DISP_CTRL1_DTE | DISP_CTRL1_D(3)
                                      | DISP_CTRL1_BASEE)
                                      );
}

static void _display_off(void)
{
    int i;
    SLCD_SEND_COMMAND(REG_DISP_CTRL1, (DISP_CTRL1_VON | DISP_CTRL1_GON
                                      | DISP_CTRL1_DTE | DISP_CTRL1_D(2))
                                      );
    SLEEP(3500000);
    SLCD_SEND_COMMAND(REG_DISP_CTRL1, DISP_CTRL1_D(1));
    SLEEP(3500000);
    SLCD_SEND_COMMAND(REG_DISP_CTRL1, DISP_CTRL1_D(0));
    SLEEP(3500000);
    SLCD_SEND_COMMAND(REG_PWR_CTRL1, PWR_CTRL1_SLP);
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
    _display_init();
}

void lcd_set_target(short x, short y, short width, short height)
{
    SLCD_SEND_COMMAND(REG_RAM_HADDR_START, y); /* y_start */
    SLCD_SEND_COMMAND(REG_RAM_HADDR_END, y+height); /* y_end */
    SLCD_SEND_COMMAND(REG_RAM_VADDR_START, x); /* x_start */
    SLCD_SEND_COMMAND(REG_RAM_VADDR_END, x+width); /* x_end */
    SLCD_SEND_COMMAND(REG_RAM_HADDR_SET, x); /* set cursor at x_start */
    SLCD_SEND_COMMAND(REG_RAM_VADDR_SET, y); /* set cursor at y_start */
    SLCD_SET_COMMAND(REG_RW_GRAM); /* write data to GRAM */
}

void lcd_on(void)
{
    _display_on();
}

void lcd_off(void)
{
    _display_off();
}
