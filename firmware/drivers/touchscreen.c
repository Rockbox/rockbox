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
#include "button.h"
#include "button-target.h"
#include "touchscreen.h"
#include "string.h"
#include "logf.h"
#include "lcd.h"

/* Size of the 'dead zone' around each 3x3 button */
#define BUTTON_MARGIN_X (int)(LCD_WIDTH * 0.03)
#define BUTTON_MARGIN_Y (int)(LCD_HEIGHT * 0.03)

static bool touch_enabled = true;
static enum touchscreen_mode current_mode = TOUCHSCREEN_POINT;
static const int touchscreen_buttons[3][3] =
{
    {BUTTON_TOPLEFT,    BUTTON_TOPMIDDLE,    BUTTON_TOPRIGHT},
    {BUTTON_MIDLEFT,    BUTTON_CENTER,       BUTTON_MIDRIGHT},
    {BUTTON_BOTTOMLEFT, BUTTON_BOTTOMMIDDLE, BUTTON_BOTTOMRIGHT}
};

#ifndef DEFAULT_TOUCHSCREEN_CALIBRATION
#define DEFAULT_TOUCHSCREEN_CALIBRATION { .A=1, .B=0, .C=0, \
                                          .D=0, .E=1, .F=0, \
                                          .divider=1 }
#endif

struct touchscreen_parameter calibration_parameters
                                              = DEFAULT_TOUCHSCREEN_CALIBRATION;
const struct touchscreen_parameter default_calibration_parameters
                                              = DEFAULT_TOUCHSCREEN_CALIBRATION;

void touchscreen_disable_mapping(void)
{
#define C(x) calibration_parameters.x
    C(A) = C(E) = 1;
    C(B) = C(C) = C(D) = C(F) = 0;
    C(divider) = 1;
#undef C
}

void touchscreen_reset_mapping(void)
{
    memcpy(&calibration_parameters, &default_calibration_parameters,
           sizeof(struct touchscreen_parameter));
}

int touchscreen_calibrate(struct touchscreen_calibration *cal)
{
#define C(x)   calibration_parameters.x /* Calibration */
#define S(i,j) cal->i[j][0]             /* Screen */
#define D(i,j) cal->i[j][1]             /* Display */
    long divider = (S(x,0) - S(x,2)) * (S(y,1) - S(y,2)) -
                   (S(x,1) - S(x,2)) * (S(y,0) - S(y,2));

    if(divider == 0)
        return -1;
    else
        C(divider) = divider;

    C(A) = (D(x,0) - D(x,2)) * (S(y,1) - S(y,2)) -
           (D(x,1) - D(x,2)) * (S(y,0) - S(y,2));

    C(B) = (S(x,0) - S(x,2)) * (D(x,1) - D(x,2)) -
           (D(x,0) - D(x,2)) * (S(x,1) - S(x,2));

    C(C) = S(y,0) * (S(x,2) * D(x,1) - S(x,1) * D(x,2)) +
           S(y,1) * (S(x,0) * D(x,2) - S(x,2) * D(x,0)) +
           S(y,2) * (S(x,1) * D(x,0) - S(x,0) * D(x,1));

    C(D) = (D(y,0) - D(y,2)) * (S(y,1) - S(y,2)) -
           (D(y,1) - D(y,2)) * (S(y,0) - S(y,2));

    C(E) = (S(x,0) - S(x,2)) * (D(y,1) - D(y,2)) -
           (D(y,0) - D(y,2)) * (S(x,1) - S(x,2));

    C(F) = S(y,0) * (S(x,2) * D(y,1) - S(x,1) * D(y,2)) +
           S(y,1) * (S(x,0) * D(y,2) - S(x,2) * D(y,0)) +
           S(y,2) * (S(x,1) * D(y,0) - S(x,0) * D(y,1));

    logf("A: %lX B: %lX C: %lX", C(A), C(B), C(C));
    logf("D: %lX E: %lX F: %lX", C(D), C(E), C(F));
    logf("divider: %lX", C(divider));

    return 0;
#undef C
#undef S
#undef D
}

static void map_pixels(int *x, int *y)
{
#define C(x) calibration_parameters.x
    int _x = *x, _y = *y;

    *x = (C(A) * _x + C(B) * _y + C(C)) / C(divider);
    *y = (C(D) * _x + C(E) * _y + C(F)) / C(divider);
#undef C
}

/* TODO: add jitter (and others) filter */
int touchscreen_to_pixels(int x, int y, int *data)
{
    if(!touch_enabled)
        return 0;
    x &= 0xFFFF;
    y &= 0xFFFF;

    map_pixels(&x, &y);

    if (current_mode == TOUCHSCREEN_BUTTON)
    {
        int column = 0, row = 0;

        if (x < LCD_WIDTH/3 - BUTTON_MARGIN_X)
           column = 0;
        else if (x > LCD_WIDTH/3 + BUTTON_MARGIN_X &&
                 x < 2*LCD_WIDTH/3 - BUTTON_MARGIN_X)
           column = 1;
        else if (x > 2*LCD_WIDTH/3 + BUTTON_MARGIN_X)
           column = 2;
        else
           return BUTTON_NONE;

        if (y < LCD_HEIGHT/3 - BUTTON_MARGIN_Y)
           row = 0;
        else if (y > LCD_HEIGHT/3 + BUTTON_MARGIN_Y &&
                 y < 2*LCD_HEIGHT/3 - BUTTON_MARGIN_Y)
           row = 1;
        else if (y > 2*LCD_HEIGHT/3 + BUTTON_MARGIN_Y)
           row = 2;
        else
            return BUTTON_NONE;
        
        return touchscreen_buttons[row][column];
    }
    else
    {
        *data = (x << 16 | y);
        return BUTTON_TOUCHSCREEN;
    }
}

void touchscreen_set_mode(enum touchscreen_mode mode)
{
    current_mode = mode;
}

enum touchscreen_mode touchscreen_get_mode(void)
{
    return current_mode;
}

#ifndef HAS_BUTTON_HOLD
void touchscreen_enable(bool en)
{
    if(en != touch_enabled)
    {
        touch_enabled = en;
        touchscreen_enable_device(en);
    }
}

bool touchscreen_is_enabled(void)
{
    return touch_enabled;
}
#endif

#if ((CONFIG_PLATFORM & PLATFORM_ANDROID) == 0) || defined(DX50) || defined(DX90)
/* android has an API for this */

#define TOUCH_SLOP      16u
#define REFERENCE_DPI   160

int touchscreen_get_scroll_threshold(void)
{
#ifdef LCD_DPI
    const int dpi = LCD_DPI;
#else
    const int dpi = lcd_get_dpi();
#endif

    /* Inspired by Android calculation
     *
     * float density = real dpi / reference dpi (=160)
     * int threshold = (int) (density * TOUCH_SLOP + 0.5f);(original calculation)
     *
     * + 0.5f is for rounding, we use fixed point math to achieve that
     */

    int result = dpi * (TOUCH_SLOP<<1) / REFERENCE_DPI;
    result += result & 1; /* round up if needed */
    return result>>1;
}

#endif
