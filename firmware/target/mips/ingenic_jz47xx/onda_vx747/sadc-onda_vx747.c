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
#include "system.h"
#include "jz4740.h"
#include "button-target.h"

#define BTN_VOL_DOWN (1 << 27)
#define BTN_VOL_UP   (1 << 0)
#define BTN_MENU     (1 << 1)
#define BTN_OFF      (1 << 29)
#define BTN_HOLD     (1 << 16)
#define BTN_MASK     (BTN_VOL_DOWN | BTN_VOL_UP \
                      | BTN_MENU | BTN_OFF )


#define TS_AD_COUNT      5
#define M_SADC_CFG_SNUM  ((TS_AD_COUNT - 1) << SADC_CFG_SNUM_BIT)

#define SADC_CFG_INIT (   \
                        (2 << SADC_CFG_CLKOUT_NUM_BIT) |  \
                        SADC_CFG_XYZ1Z2                |  \
                        M_SADC_CFG_SNUM                |  \
                        (2 << SADC_CFG_CLKDIV_BIT)     |  \
                        SADC_CFG_PBAT_HIGH             |  \
                        SADC_CFG_CMD_INT_PEN              \
                        )

static bool pendown_flag = false;
static short x_pos = -1, y_pos = -1, datacount = 0;
static short stable_x_pos = -1, stable_y_pos = -1;

bool button_hold(void)
{
    return (~REG_GPIO_PXPIN(3) & BTN_HOLD ? 1 : 0);
}

void button_init_device(void)
{
    REG_SADC_ENA = 0;
    REG_SADC_STATE &= (~REG_SADC_STATE);
    REG_SADC_CTRL = 0x1f;
    
    __cpm_start_sadc();
    REG_SADC_CFG = SADC_CFG_INIT;
    
    system_enable_irq(IRQ_SADC);
    
    REG_SADC_SAMETIME = 350;
    REG_SADC_WAITTIME = 100; /* per 10 HZ */
    REG_SADC_STATE &= (~REG_SADC_STATE);
    REG_SADC_CTRL &= (~(SADC_CTRL_PENDM | SADC_CTRL_PENUM | SADC_CTRL_TSRDYM));
    REG_SADC_ENA = (SADC_ENA_TSEN | REG_SADC_ENA); //| SADC_ENA_PBATEN | SADC_ENA_SADCINEN);
    
    __gpio_port_as_input(3, 29);
    __gpio_port_as_input(3, 27);
    __gpio_port_as_input(3, 16);
    __gpio_port_as_input(3, 1);
    __gpio_port_as_input(3, 0);
}

//static unsigned short touchdivider[2] = {14.5833*1000, 9*1000};
static int touch_to_pixels(short x, short y)
{ 
    /* X:300 -> 3800 Y:300->3900 */
    x -= 300;
    y -= 300;
    
    x /= 3200 / LCD_WIDTH;
    y /= 3600 / LCD_HEIGHT;
    //x /= touchdivider[0];
    //y /= touchdivider[1];
    
    y = LCD_HEIGHT - y;
    
    return (x << 16) | y;
}

int button_read_device(int *data)
{
    if(button_hold())
        return 0;
    
    unsigned int key = ~(__gpio_get_port(3));
    int ret = 0;
    if(key & BTN_MASK)
    {
        if(key & BTN_VOL_DOWN)
            ret |= BUTTON_VOL_DOWN;
        if(key & BTN_VOL_UP)
            ret |= BUTTON_VOL_UP;
        if(key & BTN_MENU)
            ret |= BUTTON_MENU;
        if(key & BTN_OFF)
            ret |= BUTTON_POWER;
    }
    
    if(pendown_flag)
    {
        *data = touch_to_pixels(stable_x_pos, stable_y_pos);
        ret |= BUTTON_TOUCH;
    }
    else
        *data = 0;

    return ret;
}

void button_set_touch_available(void)
{
    return;
}

/* Interrupt handler */
void SADC(void)
{
    unsigned char state;
    unsigned char sadcstate;

    sadcstate = REG_SADC_STATE;
    state = REG_SADC_STATE & (~REG_SADC_CTRL);
    REG_SADC_STATE &= sadcstate;
            
    if(state & SADC_CTRL_PENDM)
    {
        /* Pen down IRQ */
        REG_SADC_CTRL &= (~(SADC_CTRL_PENUM |  SADC_CTRL_TSRDYM));
        REG_SADC_CTRL |= (SADC_CTRL_PENDM);// | SADC_CTRL_TSRDYM);
        pendown_flag = true;
    }
    if(state & SADC_CTRL_PENUM)
    {
        /* Pen up IRQ */
        REG_SADC_CTRL &= (~SADC_CTRL_PENDM );
        REG_SADC_CTRL |= SADC_CTRL_PENUM;
        pendown_flag = false;
        x_pos = -1;
        y_pos = -1;
        stable_x_pos = -1;
        stable_y_pos = -1;
    }
    if(state & SADC_CTRL_TSRDYM)
    {
    	unsigned int   dat;
    	unsigned short xData, yData;
    	short          tsz1Data, tsz2Data;
    	
    	dat = REG_SADC_TSDAT;
    	
    	xData = (dat >>  0) & 0xfff;
    	yData = (dat >> 16) & 0xfff;
    		
    	dat = REG_SADC_TSDAT;
        tsz1Data = (dat >>  0) & 0xfff;
    	tsz2Data = (dat >> 16) & 0xfff;
        
    	if(!pendown_flag)
    		return ;
    	
         tsz1Data = tsz2Data - tsz1Data;
    	
    	if((tsz1Data > 15) || (tsz1Data < -15))
        {
    		if(x_pos == -1)
    			x_pos = xData;
    		else
    			x_pos = (x_pos + xData) / 2;
    		
    		if(y_pos == -1)
    			y_pos = yData;
    		else
    			y_pos = (y_pos + yData) / 2;
        }
        
        datacount++;
    	
    	if(datacount > TS_AD_COUNT - 1)
    	{
    		if(x_pos != -1)
    		{
                stable_x_pos = x_pos;
                stable_y_pos = y_pos;
    			x_pos = -1;
    			y_pos = -1;
    		}
    		
    		datacount = 0;
    	}
    }
    if(state & SADC_CTRL_PBATRDYM)
    {
        /* Battery AD IRQ */
    }
    if(state & SADC_CTRL_SRDYM)
    {
        /* SADC AD IRQ */
    }
}
