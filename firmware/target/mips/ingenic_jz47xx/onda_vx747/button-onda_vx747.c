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
#include "button-target.h"

#define BTN_VOL_DOWN (1 << 27)
#define BTN_VOL_UP   (1 << 0)
#define BTN_MENU     (1 << 1)
#define BTN_OFF      (1 << 29)
#define BTN_HOLD     (1 << 16)
#define BTN_MASK     (BTN_VOL_DOWN | BTN_VOL_UP \
                      | BTN_MENU | BTN_OFF )

#define SADC_CFG_INIT (   \
                        (2 << SADC_CFG_CLKOUT_NUM_BIT) |  \
                        SADC_CFG_XYZ1Z2                |  \
                        SADC_CFG_SNUM_5                |  \
                        (1 << SADC_CFG_CLKDIV_BIT)     |  \
                        SADC_CFG_PBAT_HIGH             |  \
                        SADC_CFG_CMD_INT_PEN )

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
    
    REG_SADC_SAMETIME = 1;
    REG_SADC_WAITTIME = 1000; /* per 100 HZ */
    REG_SADC_STATE &= (~REG_SADC_STATE);
    REG_SADC_CTRL &= (~(SADC_CTRL_PENDM | SADC_CTRL_TSRDYM));
    REG_SADC_ENA = SADC_ENA_TSEN; // | REG_SADC_ENA;//SADC_ENA_TSEN | SADC_ENA_PBATEN | SADC_ENA_SADCINEN;
}

static int touch_to_pixels(short x, short y)
{ 
    /* X:300 -> 3800 Y:300->3900 */
    x -= 300;
    y -= 300;
    
    /* X & Y are switched */
    x /= 3200 / LCD_HEIGHT;
    y /= 3600 / LCD_WIDTH;
    
    x = LCD_HEIGHT - x;
    y = LCD_WIDTH - y;
    
    return (y << 16) | x;
}

int button_read_device(int *data)
{
    if(button_hold())
        return 0;
    
    unsigned int key = ~REG_GPIO_PXPIN(3);
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
    
    if(REG_SADC_STATE & (SADC_CTRL_TSRDYM|SADC_STATE_PEND))
    {
        if(REG_SADC_STATE & SADC_CTRL_PENDM)
        {
            REG_SADC_CTRL &= (~(SADC_CTRL_PENUM |  SADC_CTRL_TSRDYM));
            REG_SADC_CTRL |= (SADC_CTRL_PENDM);
            unsigned int dat;
            unsigned short xData,yData;
            short tsz1Data,tsz2Data;
            
            dat = REG_SADC_TSDAT;
            
            xData = (dat >>  0) & 0xfff;
            yData = (dat >> 16) & 0xfff;
            
            dat = REG_SADC_TSDAT;
            tsz1Data = (dat >>  0) & 0xfff;
            tsz2Data = (dat >> 16) & 0xfff;
            
            *data = touch_to_pixels(xData, yData);
            
            tsz1Data = tsz2Data - tsz1Data;
        }
        REG_SADC_STATE = 0;
        //__intc_unmask_irq(IRQ_SADC);
    }
    else
        *data = 0;

    return ret;
}
void button_set_touch_available(void)
{
    return;
}
