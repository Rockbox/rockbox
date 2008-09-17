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
#include "button.h"
#include "button-target.h"

#define BTN_OFF      (1 << 29)
#define BTN_VOL_DOWN (1 << 27)
#define BTN_HOLD     (1 << 16)
#define BTN_MENU     (1 << 1)
#define BTN_VOL_UP   (1 << 0)
#define BTN_MASK     (BTN_OFF | BTN_VOL_DOWN | \
                      BTN_MENU | BTN_VOL_UP)


#define TS_AD_COUNT     5
#define M_SADC_CFG_SNUM ((TS_AD_COUNT - 1) << SADC_CFG_SNUM_BIT)

#define SADC_CFG_INIT   (                                 \
                        (2 << SADC_CFG_CLKOUT_NUM_BIT) |  \
                        SADC_CFG_XYZ1Z2                |  \
                        M_SADC_CFG_SNUM                |  \
                        (2 << SADC_CFG_CLKDIV_BIT)     |  \
                        SADC_CFG_PBAT_HIGH             |  \
                        SADC_CFG_CMD_INT_PEN              \
                        )

static short x_pos = -1, y_pos = -1, datacount = 0;
static bool pen_down = false;
static int cur_touch = 0;

static enum touchscreen_mode current_mode = TOUCHSCREEN_POINT;
static int touchscreen_buttons[3][3] =
{
    {BUTTON_TOPLEFT,    BUTTON_TOPMIDDLE,    BUTTON_TOPRIGHT},
    {BUTTON_MIDLEFT,    BUTTON_CENTER,       BUTTON_MIDRIGHT},
    {BUTTON_BOTTOMLEFT, BUTTON_BOTTOMMIDDLE, BUTTON_BOTTOMRIGHT}
};

void button_init_device(void)
{
    REG_SADC_ENA = 0;
    REG_SADC_STATE &= (~REG_SADC_STATE);
    REG_SADC_CTRL = 0x1F;
    
    __cpm_start_sadc();
    REG_SADC_CFG = SADC_CFG_INIT;
    
    system_enable_irq(IRQ_SADC);
    
    REG_SADC_SAMETIME = 350;
    REG_SADC_WAITTIME = 100;
    REG_SADC_STATE &= (~REG_SADC_STATE);
    REG_SADC_CTRL &= (~(SADC_CTRL_PENDM | SADC_CTRL_PENUM | SADC_CTRL_TSRDYM));
    REG_SADC_ENA = SADC_ENA_TSEN; //| SADC_ENA_PBATEN | SADC_ENA_SADCINEN);
    
    __gpio_as_input(32*3 + 29);
    __gpio_as_input(32*3 + 27);
    __gpio_as_input(32*3 + 16);
    __gpio_as_input(32*3 + 1);
    __gpio_as_input(32*3 + 0);
}

static int touch_to_pixels(short x, short y)
{
    /* X:300 -> 3800 Y:300->3900 */
    x -= 300;
    y -= 300;
    
#if CONFIG_ORIENTATION == SCREEN_PORTRAIT
    x /= 3200 / LCD_WIDTH;
    y /= 3600 / LCD_HEIGHT;

    y = LCD_HEIGHT - y;
    
    return (x << 16) | y;
#else
    x /= 3200 / LCD_HEIGHT;
    y /= 3600 / LCD_WIDTH;

    y = LCD_WIDTH - y;
    x = LCD_HEIGHT - x;
    
    return (y << 16) | x;
#endif
}

bool button_hold(void)
{
    return ((~REG_GPIO_PXPIN(3)) & BTN_HOLD ? true : false);
}

int button_read_device(int *data)
{
    int ret = 0, tmp;

    if((~REG_GPIO_PXPIN(3)) & BTN_HOLD)
        return 0;

    tmp = (~REG_GPIO_PXPIN(3)) & BTN_MASK;

    if(tmp & BTN_VOL_DOWN)
        ret |= BUTTON_VOL_DOWN;
    if(tmp & BTN_VOL_UP)
        ret |= BUTTON_VOL_UP;
    if(tmp & BTN_MENU)
        ret |= BUTTON_MENU;
    if(tmp & BTN_OFF)
        ret |= BUTTON_POWER;

    if(current_mode == TOUCHSCREEN_BUTTON && cur_touch != 0)
    {
        int px_x = cur_touch >> 16;
        int px_y = cur_touch & 0xFFFF;
        ret |= touchscreen_buttons[px_y/(LCD_HEIGHT/3)]
                                  [px_x/(LCD_WIDTH/3)];
    }
    else if(pen_down)
    {
        ret |= BUTTON_TOUCH;
        if(data != NULL)
            *data = cur_touch;
    }

    return ret;
}

void touchscreen_set_mode(enum touchscreen_mode mode)
{
    current_mode = mode;
}

enum touchscreen_mode touchscreen_get_mode(void)
{
    return current_mode;
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
        REG_SADC_CTRL |= (SADC_CTRL_PENDM);
        pen_down = true;
    }
    if(state & SADC_CTRL_PENUM)
    {
        /* Pen up IRQ */
        REG_SADC_CTRL &= (~SADC_CTRL_PENDM );
        REG_SADC_CTRL |= SADC_CTRL_PENUM;
        pen_down = false;
        x_pos = -1;
        y_pos = -1;
        cur_touch = 0;
    }
    if(state & SADC_CTRL_TSRDYM)
    {
        unsigned int   dat;
        unsigned short xData, yData;
        short          tsz1Data, tsz2Data;
        
        dat = REG_SADC_TSDAT;
        
        xData = (dat >>  0) & 0xFFF;
        yData = (dat >> 16) & 0xFFF;
        
        dat = REG_SADC_TSDAT;
        tsz1Data = (dat >>  0) & 0xFFF;
        tsz2Data = (dat >> 16) & 0xFFF;
        
        if( !pen_down )
            return;
        
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
                cur_touch = touch_to_pixels(x_pos, y_pos);
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
