/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
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
#include "cpu.h"
#include "button.h"
#include "adc.h"
#include "pcf50606.h"
#include "backlight.h"
#include "touchscreen.h"
#include "stdlib.h"

#define NO_OF_TOUCH_DATA 5

static bool touch_available = false;
static short x[NO_OF_TOUCH_DATA], y[NO_OF_TOUCH_DATA];

/* comparator for qsort */
static int short_cmp(const void *a, const void *b)
{
    return *(short*)a - *(short*)b;
}

void button_read_touch()
{
    static long last_touch_interrupt = 0;
    static int touch_data_index = 0;

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

struct touch_calibration_point {
    short px_x; /* known pixel value */
    short px_y;
    short val_x; /* touchscreen value at the known pixel */
    short val_y;
};

static struct touch_calibration_point topleft, bottomright;

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

void button_init_device(void)
{
    /* Configure GPIOA 4 (POWER) and 8 (HOLD) for input */
    GPIOA_DIR &= ~0x110;

    /* Configure GPIOB 4 (button pressed) for input */
    GPIOB_DIR &= ~0x10;

    touch_available = false;

    /* Arbitrary touchscreen calibration */
    topleft.px_x = 0;
    topleft.px_y = 0;
    topleft.val_x = 50;
    topleft.val_y = 50;

    bottomright.px_x = LCD_WIDTH;
    bottomright.px_y = LCD_HEIGHT;
    bottomright.val_x = 980;
    bottomright.val_y = 980;
}

bool button_hold(void)
{
    return (GPIOA & 0x8) ? false : true;
}

int button_read_device(int *data)
{
    int btn = BUTTON_NONE;
    int adc;
    static int old_data = 0;

    static bool hold_button = false;
    bool hold_button_old;
    static bool touch_hold = false;
    static long last_touch = 0;

    *data = old_data;

    hold_button_old = hold_button;
    hold_button = button_hold();

#ifndef BOOTLOADER
    if (hold_button != hold_button_old)
        backlight_hold_changed(hold_button);
#endif

    if (hold_button)
        return BUTTON_NONE;

    if (GPIOB & 0x4)
    {
        adc = adc_read(ADC_BUTTONS);

        /* The following contains some arbitrary, but working, guesswork */
        if (adc < 0x038) {
            btn |= (BUTTON_MINUS | BUTTON_PLUS | BUTTON_MENU);
        } else if (adc < 0x048) {
            btn |= (BUTTON_MINUS | BUTTON_PLUS);
        } else if (adc < 0x058) {
            btn |= (BUTTON_PLUS | BUTTON_MENU);
        } else if (adc < 0x070) {
            btn |= BUTTON_PLUS;
        } else if (adc < 0x090) {
            btn |= (BUTTON_MINUS  | BUTTON_MENU);
        } else if (adc < 0x150) {
            btn |= BUTTON_MINUS;
        } else if (adc < 0x200) {
            btn |= BUTTON_MENU;
        }
    }

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
        old_data = *data = touch_to_pixels(x_touch, y_touch);
        btn |= touchscreen_to_pixels((*data&0xffff0000)>>16,
                                     (*data&0x0000ffff),
                                     data);
    }

    if (TIME_AFTER(current_tick, last_touch + 10))
    {
        /* put the touchscreen back into interrupt mode */
        touch_hold = false;
        pcf50606_write(PCF5060X_ADCC1, 1);
    }

    if (!(GPIOA & 0x4))
        btn |= BUTTON_POWER;

    if (btn & BUTTON_TOUCHSCREEN && !is_backlight_on(true))
        old_data = *data = 0;

    return btn;
}
