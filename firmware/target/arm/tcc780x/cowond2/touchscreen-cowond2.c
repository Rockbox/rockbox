/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Rob Purchase, Carsten Schreiter, Jonas Aaberg
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
#include "button.h"
#include "pcf50606.h"
#include "touchscreen.h"
#include "stdlib.h"
#include "power-target.h"
#include "tsc200x.h"

#define NO_OF_TOUCH_DATA 5

static bool touch_available = false;

static short x[NO_OF_TOUCH_DATA], y[NO_OF_TOUCH_DATA];

/* comparator for qsort */
static int short_cmp(const void *a, const void *b)
{
    return *(short*)a - *(short*)b;
}

static int touch_to_pixels(short val_x, short val_y)
{
    int x = (val_x * LCD_WIDTH) >> 10;
    int y = (val_y * LCD_HEIGHT) >> 10;

    if (x < 0)
        x = 0;
    else if (x >= LCD_WIDTH)
        x = LCD_WIDTH - 1;

    if (y < 0)
        y = 0;
    else if (y >= LCD_HEIGHT)
        y = LCD_HEIGHT - 1;

    return (x << 16) | y;
}

static int touchscreen_read_pcf50606(int *data, int *old_data)
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
        pcf50606_write(PCF5060X_ADCC1, 1);
    }
    
    return btn;
}

static int touchscreen_read_tsc200x(int *data, int *old_data)
{
    int btn = BUTTON_NONE;
    short x_touch, y_touch;
    
    static long last_read = 0;
    static int last_btn = BUTTON_NONE;

    /* Don't read hw every check button round. I2C is slow
     * and man is even slower. */
    if (TIME_BEFORE(current_tick, last_read + 10))
    {
        *data = *old_data;
        return last_btn;
    }

    if (tsc200x_is_pressed())
    {
        if (tsc200x_read_coords(&x_touch, &y_touch))
        {
            *old_data = *data = touch_to_pixels(x_touch, y_touch);  

            btn = touchscreen_to_pixels((*data & 0xffff0000) >> 16,
                                        (*data & 0x0000ffff),
                                        data);
        }
    }

    last_read = current_tick;

    last_btn = btn;

    return btn;
}

void touchscreen_init_device(void)
{
    touch_available = false;
    
    if (get_pmu_type() != PCF50606)
    {
        tsc200x_init();
    }
}

void touchscreen_handle_device_irq(void)
{
    static long last_touch_interrupt = 0;
    static int touch_data_index = 0;
    
    /* don't read the coordinates when hold is enabled */
    if (button_hold()) return;

    /* put the touchscreen into idle mode */
    pcf50606_write(PCF5060X_ADCC1, 0);

    if (TIME_AFTER(current_tick, last_touch_interrupt + 1))
    {
        /* resets the index if the last touch could not be read 5 times */
        touch_data_index = 0;
    }

    /* here the touch coordinates are read 5 times */
    /* they will be sorted and the middle one will be used */
    pcf50606_read_adc(PCF5060X_ADC_TSC_XY,
                      &x[touch_data_index], &y[touch_data_index]);

    touch_data_index++;

    if (touch_data_index > NO_OF_TOUCH_DATA - 1)
    {
        /* coordinates 5 times read */
        touch_available = true;
        touch_data_index = 0;
    }
    else
    {
        /* put the touchscreen back into the interrupt mode */
        pcf50606_write(PCF5060X_ADCC1, 1);
    }
    last_touch_interrupt = current_tick;
}


int touchscreen_read_device(int *data, int *old_data)
{
    int btn;
    
    if (get_pmu_type() == PCF50606)
        btn = touchscreen_read_pcf50606(data, old_data);
    else
        btn = touchscreen_read_tsc200x(data, old_data);
        
    return btn;
}
