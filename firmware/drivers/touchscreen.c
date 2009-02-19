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

static enum touchscreen_mode current_mode = TOUCHSCREEN_POINT;
static const int touchscreen_buttons[3][3] =
{
    {BUTTON_TOPLEFT,    BUTTON_TOPMIDDLE,    BUTTON_TOPRIGHT},
    {BUTTON_MIDLEFT,    BUTTON_CENTER,       BUTTON_MIDRIGHT},
    {BUTTON_BOTTOMLEFT, BUTTON_BOTTOMMIDDLE, BUTTON_BOTTOMRIGHT}
};

/* Based on ftp://ftp.embedded.com/pub/2002/06vidales/calibrate.c
 *
 *   Copyright (c) 2001, Carlos E. Vidales. All rights reserved.
 *
 *   This sample program was written and put in the public domain 
 *    by Carlos E. Vidales.  The program is provided "as is" 
 *    without warranty of any kind, either expressed or implied.
 *   If you choose to use the program within your own products
 *    you do so at your own risk, and assume the responsibility
 *    for servicing, repairing or correcting the program should
 *    it prove defective in any manner.
 *   You may copy and distribute the program's source code in any 
 *    medium, provided that you also include in each copy an
 *    appropriate copyright notice and disclaimer of warranty.
 *   You may also modify this program and distribute copies of
 *    it provided that you include prominent notices stating 
 *    that you changed the file(s) and the date of any change,
 *    and that you do not charge any royalties or licenses for 
 *    its use.
 */
struct touchscreen_parameter
{
    long A;
    long B;
    long C;
    long D;
    long E;
    long F;
    long divider;
};

#ifndef DEFAULT_TOUCHSCREEN_CALIBRATION
#define DEFAULT_TOUCHSCREEN_CALIBRATION {.A=1, .B=0, .C=0, \
                                         .D=0, .E=1, .F=0, \
                                         .divider=1}
#endif

static struct touchscreen_parameter calibration_parameters
                                              = DEFAULT_TOUCHSCREEN_CALIBRATION;
static const struct touchscreen_parameter default_parameters
                                              = DEFAULT_TOUCHSCREEN_CALIBRATION;

void touchscreen_disable_mapping(void)
{
    calibration_parameters.A = 1;
    calibration_parameters.B = 0;
    calibration_parameters.C = 0;
    calibration_parameters.D = 0;
    calibration_parameters.E = 1;
    calibration_parameters.F = 0;
    calibration_parameters.divider = 1;
}

void touchscreen_reset_mapping(void)
{
    memcpy(&calibration_parameters, &default_parameters,
           sizeof(struct touchscreen_parameter));
}

int touchscreen_calibrate(struct touchscreen_calibration *cal)
{
    calibration_parameters.divider = ((cal->x[0] - cal->x[2]) * (cal->y[1] - cal->y[2])) - 
                                     ((cal->x[1] - cal->x[2]) * (cal->y[0] - cal->y[2])) ;

    if(calibration_parameters.divider == 0)
        return -1;
    
    calibration_parameters.A = ((cal->xfb[0] - cal->xfb[2]) * (cal->y[1] - cal->y[2])) - 
                               ((cal->xfb[1] - cal->xfb[2]) * (cal->y[0] - cal->y[2])) ;

    calibration_parameters.B = ((cal->x[0]   - cal->x[2])   * (cal->xfb[1] - cal->xfb[2])) - 
                               ((cal->xfb[0] - cal->xfb[2]) * (cal->x[1]   - cal->x[2])) ;

    calibration_parameters.C = (cal->x[2] * cal->xfb[1] - cal->x[1] * cal->xfb[2]) * cal->y[0] +
                               (cal->x[0] * cal->xfb[2] - cal->x[2] * cal->xfb[0]) * cal->y[1] +
                               (cal->x[1] * cal->xfb[0] - cal->x[0] * cal->xfb[1]) * cal->y[2] ;

    calibration_parameters.D = ((cal->yfb[0] - cal->yfb[2]) * (cal->y[1] - cal->y[2])) - 
                               ((cal->yfb[1] - cal->yfb[2]) * (cal->y[0] - cal->y[2])) ;

    calibration_parameters.E = ((cal->x[0]   - cal->x[2])   * (cal->yfb[1] - cal->yfb[2])) - 
                               ((cal->yfb[0] - cal->yfb[2]) * (cal->x[1]   - cal->x[2])) ;

    calibration_parameters.F = (cal->x[2] * cal->yfb[1] - cal->x[1] * cal->yfb[2]) * cal->y[0] +
                               (cal->x[0] * cal->yfb[2] - cal->x[2] * cal->yfb[0]) * cal->y[1] +
                               (cal->x[1] * cal->yfb[0] - cal->x[0] * cal->yfb[1]) * cal->y[2] ;

    logf("A: %lX B: %lX C: %lX", calibration_parameters.A,
         calibration_parameters.B, calibration_parameters.C);
    logf("D: %lX E: %lX F: %lX", calibration_parameters.D,
         calibration_parameters.E, calibration_parameters.F);
    logf("divider: %lX", calibration_parameters.divider);
    
    return 0;
}

static void map_pixels(int *x, int *y)
{
    int _x = *x, _y = *y;
    
    *x = (calibration_parameters.A*_x + calibration_parameters.B*_y +
          calibration_parameters.C) / calibration_parameters.divider;
    *y = (calibration_parameters.D*_x + calibration_parameters.E*_y +
          calibration_parameters.F) / calibration_parameters.divider;
}

int touchscreen_to_pixels(int x, int y, int *data)
{
    x &= 0xFFFF;
    y &= 0xFFFF;
    
    map_pixels(&x, &y);
    
    if(current_mode == TOUCHSCREEN_BUTTON)
        return touchscreen_buttons[y / (LCD_HEIGHT/3)]
                                  [x / (LCD_WIDTH/3) ];
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
