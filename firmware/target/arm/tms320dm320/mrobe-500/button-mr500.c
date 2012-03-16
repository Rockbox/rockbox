/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007, 2009 by Karl Kurbjun
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
#include "system.h"
#include "button.h"
#include "kernel.h"
#include "backlight.h"
#include "backlight-target.h"
#include "lcd-remote-target.h"
#include "uart-target.h"
#include "tsc2100.h"
#include "string.h"
#include "touchscreen.h"

static bool hold_button     = false;

static struct touch_calibration_point topleft, bottomright;

/* Jd's tests.. These will hopefully work for everyone so we dont have to
 *  create a calibration screen.
 *  Portait:
 *      (0,0) = 200, 3900
 *      (480,640) = 3880, 270
 *  Landscape:
 *      (0,0) = 200, 270
 *      (640,480) = 3880, 3900
*/

static int touch_to_pixels(short *val_x, short *val_y)
{
    short x,y;

#if CONFIG_ORIENTATION == SCREEN_PORTRAIT
    x=*val_x;
    y=*val_y;
#else
    x=*val_y;
    y=*val_x;
#endif

    x = (x-topleft.val_x)*(bottomright.px_x - topleft.px_x) / (bottomright.val_x - topleft.val_x) + topleft.px_x;
    y = (y-topleft.val_y)*(bottomright.px_y - topleft.px_y) / (bottomright.val_y - topleft.val_y) + topleft.px_y;

    if (x < 0)
        x = 0;
    else if (x>=LCD_WIDTH)
        x=LCD_WIDTH-1;
        
    if (y < 0)
        y = 0;
    else if (y>=LCD_HEIGHT)
        y=LCD_HEIGHT-1;

    *val_x=x;
    *val_y=y;

    return (x<<16)|y;
}

void button_init_device(void)
{
    /* GIO is the power button, set as input */
    IO_GIO_DIR0 |= 0x01;

#if CONFIG_ORIENTATION == SCREEN_PORTRAIT
    topleft.val_x = 200;        
    topleft.val_y = 3900;
    
    bottomright.val_x = 3880;
    bottomright.val_y = 270;
#else
    topleft.val_x = 270;
    topleft.val_y = 200;
    
    bottomright.val_x = 3900;
    bottomright.val_y = 3880;
#endif

    topleft.px_x = 0;
    topleft.px_y = 0;

    bottomright.px_x = LCD_WIDTH;
    bottomright.px_y = LCD_HEIGHT;
}

inline bool button_hold(void)
{
    return hold_button;
}

#ifdef HAVE_REMOTE_LCD
static bool remote_hold_button;
static int remote_read_buttons(void)
{
    static char read_buffer[5];
    int read_button = BUTTON_NONE;
    
    static int oldbutton=BUTTON_NONE;
    
    /* Handle remote buttons */
    if(uart1_gets_queue(read_buffer, 5)>=0)
    {
        int button_location;
        
        for(button_location=0;button_location<4;button_location++)
        {
            if((read_buffer[button_location]&0xF0)==0xF0 
                && (read_buffer[button_location+1]&0xF0)!=0xF0)
                break;
        }
        
        if(button_location==4)
            button_location=0;
        
        button_location++;
            
        read_button |= read_buffer[button_location];
        
        /* Find the hold status location */
        if(button_location==4)
            button_location=0;
        else
            button_location++;
            
        remote_hold_button=((read_buffer[button_location]&0x80)?true:false);
        
        uart1_clear_queue();
        oldbutton=read_button;
    }
    else
        read_button=oldbutton;
        
    return read_button;
}
#endif

/* Since this is a touchscreen, the expectation in higher levels is that the
 *  previous touch location is maintained when a release occurs.  This is
 *  intended to mimic a mouse or other similar pointing device.
 */
int button_read_device(int *data)
{
    static int old_data = 0;
    int button_read = BUTTON_NONE;
    short touch_x, touch_y, touch_z1, touch_z2;
#ifndef BOOTLOADER
    static bool hold_button_old = false;
#endif
    *data = old_data;

    /* Handle touchscreen */
    if (tsc2100_read_touch(&touch_x, &touch_y, &touch_z1, &touch_z2))
    {
        old_data = *data = touch_to_pixels(&touch_x, &touch_y);
        button_read |= touchscreen_to_pixels(touch_x, touch_y, data);
    }

    tsc2100_set_mode(true, 0x02);
        
    /* Handle power button */
    if ((IO_GIO_BITSET0&0x01) == 0) {
        button_read |=  BUTTON_POWER;
    }

#if defined(HAVE_REMOTE_LCD)
    /* Read data from the remote */
    button_read |= remote_read_buttons();
    hold_button=remote_hold_button;
#endif
    
    /* Take care of hold notifications */
#ifndef BOOTLOADER
    /* give BL notice if HB state chaged */
    if (hold_button != hold_button_old) {
        backlight_hold_changed(hold_button);
        hold_button_old=hold_button;
    }
#endif

    if (hold_button)
    {
        button_read = BUTTON_NONE;
    }
    
    return button_read;
}

