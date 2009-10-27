/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___void 
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Rob Purchase
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
#include "touchscreen-target.h"
#include "adc-target.h"
#include "system.h"
#include "stdlib.h"
#include "button.h"
#include "touchscreen.h"

#define NO_OF_TOUCH_DATA 5

struct touch_calibration_point {
    short px_x; /* known pixel value */
    short px_y;
    short val_x; /* touchscreen value at the known pixel */
    short val_y;
};

static struct touch_calibration_point topleft, bottomright;

static bool touch_available = false;

static short x[NO_OF_TOUCH_DATA], y[NO_OF_TOUCH_DATA];

/* comparator for qsort */
static int short_cmp(const void *a, const void *b)
{
    return *(short*)a - *(short*)b;
}

static int touch_to_pixels(short val_x, short val_y)
{
    short x,y;

    x=val_x;
    y=val_y;

    x = (x-topleft.val_x)*(bottomright.px_x - topleft.px_x)
            / (bottomright.val_x - topleft.val_x) + topleft.px_x;

    y = (y-topleft.val_y)*(bottomright.px_y - topleft.px_y)
            / (bottomright.val_y - topleft.val_y) + topleft.px_y;

    if (x < 0)
        x = 0;
    else if (x>=LCD_WIDTH)
        x=LCD_WIDTH-1;

    if (y < 0)
        y = 0;
    else if (y>=LCD_HEIGHT)
        y=LCD_HEIGHT-1;

    return (x<<16)|y;
}

void touchscreen_init_device()
{
    /* set touchscreen adc controller into wait for interrupt mode */
    ADCTSC = 0xd3; /* Wfait,XP-PU,XP_DIS,XM_DIS,YP_dis,YM_end */

    /* Arbitrary touchscreen calibration */
    topleft.px_x = 0;
    topleft.px_y = 0;
    topleft.val_x = 105;
    topleft.val_y = 925;

    bottomright.px_x = LCD_WIDTH;
    bottomright.px_y = LCD_HEIGHT;
    bottomright.val_x = 890;
    bottomright.val_y = 105;

    touch_available = false;
}

void touchscreen_scan_device()
{
    static long last_touch_read = 0;
    static int touch_data_index = 0;

   int saveADCDLY;
 
    /* check touch state */ 
    if(ADCDAT1 & (1<<15))    
    {
        return;
    }

    if (TIME_AFTER(current_tick, last_touch_read + 1))
    {
        /* resets the index if the last touch could not be read 5 times */
        touch_data_index = 0;
    }
    
    /* read touch data */    
    saveADCDLY = ADCDLY;
    ADCDLY = 40000; /*delay ~0.8ms (1/50M)*4000 */ 
    ADCTSC = (1<<3)|(1<<2); /* pullup disable, seq x,y pos measure */
    /* start adc */
    ADCCON|= 0x1; 
    /* wait for start and end */
    while(ADCCON & 0x1);
    while(!(ADCCON & 0x8000));

    x[touch_data_index] = ADCDAT0&0x3ff;
    y[touch_data_index] = ADCDAT1&0x3ff;
   
    ADCTSC = 0xd3; /* back to interrupt mode */
    ADCDLY = saveADCDLY;
    
    touch_data_index++;

    if (touch_data_index > NO_OF_TOUCH_DATA - 1)
    {
        /* coordinates 5 times read */
        touch_available = true;
        touch_data_index = 0;
    }
    last_touch_read = current_tick;
}

int touchscreen_read_device(int *data, int *old_data)
{
    int btn = BUTTON_NONE;
    static bool touch_hold = false;
    static long last_touch = 0;

    if (touch_available || touch_hold)
    {
        short x_touch, y_touch;
        static short last_x = 0, last_y = 0;

        if (touch_hold)
        {
            /* get rid of very fast unintended double touches */
            x_touch = last_x;
            y_touch = last_y;
        }
        else
        {
            /* sort the 5 data taken and use the median value */
            qsort(x, NO_OF_TOUCH_DATA, sizeof(short), short_cmp);
            qsort(y, NO_OF_TOUCH_DATA, sizeof(short), short_cmp);
            
            x_touch = last_x = x[(NO_OF_TOUCH_DATA - 1)/2];
            y_touch = last_y = y[(NO_OF_TOUCH_DATA - 1)/2];
            
            last_touch = current_tick;
            
            touch_hold = true;
            touch_available = false;
        }
        
        *old_data = *data = touch_to_pixels(x_touch, y_touch);
        
        btn |= touchscreen_to_pixels((*data&0xffff0000) >> 16,
                                     (*data&0x0000ffff),
                                     data);
    }

    if (TIME_AFTER(current_tick, last_touch + 10))
    {
        /* put the touchscreen back into interrupt mode */
        touch_hold = false;
    }
    
    return btn;
}


