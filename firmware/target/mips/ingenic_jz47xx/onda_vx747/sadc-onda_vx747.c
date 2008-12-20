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
#include "powermgmt.h"
#include "kernel.h"

#ifdef ONDA_VX747
#define BTN_OFF      (1 << 29)
#define BTN_VOL_DOWN (1 << 27)
#define BTN_HOLD     (1 << 16)
#define BTN_MENU     (1 << 1)
#define BTN_VOL_UP   (1 << 0)
#elif defined(ONDA_VX747P)
#define BTN_OFF      (1 << 29)
#define BTN_VOL_DOWN (1 << 27)
#define BTN_HOLD     (1 << 22)  /* on REG_GPIO_PXPIN(2) */
#define BTN_MENU     (1 << 20)
#define BTN_VOL_UP   (1 << 19)
#else
#error No buttons defined!
#endif

#define BTN_MASK     (BTN_OFF | BTN_VOL_DOWN | \
                      BTN_MENU | BTN_VOL_UP)


#define TS_AD_COUNT     3
#define SADC_CFG_SNUM   ((TS_AD_COUNT - 1) << SADC_CFG_SNUM_BIT)

#define SADC_CFG_INIT   (                                 \
                        (2 << SADC_CFG_CLKOUT_NUM_BIT) |  \
                        SADC_CFG_XYZ1Z2                |  \
                        SADC_CFG_SNUM                  |  \
                        (2 << SADC_CFG_CLKDIV_BIT)     |  \
                        SADC_CFG_PBAT_HIGH             |  \
                        SADC_CFG_CMD_INT_PEN              \
                        )

static signed int x_pos, y_pos;
static int datacount = 0, cur_touch = 0;
static bool pen_down = false;
static volatile unsigned short bat_val = 0;
static struct mutex battery_mtx;

static enum touchscreen_mode current_mode = TOUCHSCREEN_POINT;
static const int touchscreen_buttons[3][3] =
{
    {BUTTON_TOPLEFT,    BUTTON_TOPMIDDLE,    BUTTON_TOPRIGHT},
    {BUTTON_MIDLEFT,    BUTTON_CENTER,       BUTTON_MIDRIGHT},
    {BUTTON_BOTTOMLEFT, BUTTON_BOTTOMMIDDLE, BUTTON_BOTTOMRIGHT}
};

const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    /* TODO */
    3400
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    /* TODO */
    3300
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    /* TODO */
    { 3300, 3680, 3740, 3760, 3780, 3810, 3870, 3930, 3970, 4070, 4160 },
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
    /* TODO */
    3300, 3680, 3740, 3760, 3780, 3810, 3870, 3930, 3970, 4070, 4160
};

/* VBAT = (BDATA/4096) * 7.5V */
#define BATTERY_SCALE_FACTOR 7500

/* Returns battery voltage from ADC [millivolts] */
unsigned int battery_adc_voltage(void)
{
    register unsigned short dummy;
    
    mutex_lock(&battery_mtx);
    
    dummy = REG_SADC_BATDAT;
    dummy = REG_SADC_BATDAT;
    
    bat_val = 0;
    REG_SADC_ENA |= SADC_ENA_PBATEN;
    
    /* primitive wakeup event */
    while(bat_val == 0)
        yield();
    
    mutex_unlock(&battery_mtx);
    
    return (bat_val*BATTERY_SCALE_FACTOR)>>12;
}

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
    REG_SADC_CTRL = (~(SADC_CTRL_PENDM | SADC_CTRL_PENUM | SADC_CTRL_TSRDYM | SADC_CTRL_PBATRDYM));
    REG_SADC_ENA = (SADC_ENA_TSEN | SADC_ENA_PBATEN);
    
#ifdef ONDA_VX747
    __gpio_as_input(32*3 + 29);
    __gpio_as_input(32*3 + 27);
    __gpio_as_input(32*3 + 16);
    __gpio_as_input(32*3 + 1);
    __gpio_as_input(32*3 + 0);
#elif defined(ONDA_VX747P)
    __gpio_as_input(32*3 + 29);
    __gpio_as_input(32*3 + 27);
    __gpio_as_input(32*3 + 20);
    __gpio_as_input(32*3 + 19);
    __gpio_as_input(32*2 + 22);
#endif
    
    mutex_init(&battery_mtx);
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

    /* Filter button events out if HOLD button is pressed at firmware/ level */
    if(
#ifdef ONDA_VX747
    (~REG_GPIO_PXPIN(3)) & BTN_HOLD
#elif defined(ONDA_VX747P)
    (~REG_GPIO_PXPIN(2)) & BTN_HOLD
#endif
    )
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
        if(data != NULL && cur_touch != 0)
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
        datacount = 0;
        cur_touch = 0;
    }
    
    if(state & SADC_CTRL_TSRDYM)
    {
        unsigned int   dat;
        unsigned short xData, yData;
        signed short   tsz1Data, tsz2Data;
        
        dat = REG_SADC_TSDAT;
        xData = (dat >>  0) & 0xFFF;
        yData = (dat >> 16) & 0xFFF;
        
        dat = REG_SADC_TSDAT;
        tsz1Data = (dat >>  0) & 0xFFF;
        tsz2Data = (dat >> 16) & 0xFFF;
        
        if(!pen_down)
            return;
        
        tsz1Data = tsz2Data - tsz1Data;
        
        if((tsz1Data > 100) || (tsz1Data < -100))
        {
            if(datacount == 0)
            {
                x_pos = xData;
                y_pos = yData;
            }
            else
            {
                x_pos += xData;
                y_pos += yData;
            }
        }
        
        datacount++;
        
        if(datacount >= TS_AD_COUNT)
        {
            cur_touch = touch_to_pixels(x_pos/datacount, y_pos/datacount);
            datacount = 0;
        }
    }
    
    if(state & SADC_CTRL_PBATRDYM)
    {
        bat_val = REG_SADC_BATDAT;
        /* Battery AD IRQ */
    }
}
