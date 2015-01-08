/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Bob Cousins
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
#include "backlight-target.h"
#include "backlight.h"
#include "lcd.h"
#include "power.h"


/* Dummy value at index 0, 1-12 used. */
static const unsigned char log_brightness[13] =
    {0,0,1,2,3,5,7,10,15,22,31,44,63};


static enum backlight_states
{
    BACKLIGHT_CONTROL_IDLE,
    BACKLIGHT_CONTROL_OFF,
    BACKLIGHT_CONTROL_ON,
    BACKLIGHT_CONTROL_SET,
    BACKLIGHT_CONTROL_FADE
} backlight_control;

static unsigned char _backlight_brightness;
static unsigned char backlight_target;


/* Assumes that the backlight has been initialized */
void backlight_hw_brightness(int brightness)
{
    if (brightness < 0)
        brightness = 0;
    else if(brightness > MAX_BRIGHTNESS_SETTING)
        brightness = MAX_BRIGHTNESS_SETTING;

    /* stop the interrupt from messing us up */
    backlight_control = BACKLIGHT_CONTROL_IDLE;
    _backlight_brightness = log_brightness[brightness];
    backlight_control = BACKLIGHT_CONTROL_SET;
}

static void _backlight_set_state (unsigned int level)
{
    if (level == 0)
       GPGDAT &= ~GPIO_LCD_PWR;
    else
       GPGDAT |= GPIO_LCD_PWR;
}

/* led_control_service runs in interrupt context - be brief!
 * This service is called once per interrupt timer tick - 100 times a second.
 *
 * There should be at most only one i2c operation per call - if more are need
 *  the calls should be spread across calls.
 *
 * Putting all led servicing in one thread means that we wont step on any
 * i2c operations - they are all serialized here in the ISR tick. It also
 * insures that we get called at equal timing for good visual effect.
 */
#ifndef BOOTLOADER
static void led_control_service(void)
{
    switch (backlight_control)
    {
        case BACKLIGHT_CONTROL_IDLE:
            backlight_control = BACKLIGHT_CONTROL_IDLE;
            break;
        case BACKLIGHT_CONTROL_OFF:
            backlight_hw_brightness(0);
            backlight_control = BACKLIGHT_CONTROL_IDLE;
            break;
        case BACKLIGHT_CONTROL_ON:
            backlight_hw_brightness(DEFAULT_BRIGHTNESS_SETTING);
            backlight_control = BACKLIGHT_CONTROL_IDLE;
            break;
        case BACKLIGHT_CONTROL_SET:
            /* TODO: This is probably wrong since it sets a fixed value.
            It was a fixed value of 255 before, but that was even more wrong
            since it accessed the log_brightness buffer out of bounds */
            backlight_hw_brightness(DEFAULT_BRIGHTNESS_SETTING);
            backlight_control = BACKLIGHT_CONTROL_IDLE;
            break;
        case BACKLIGHT_CONTROL_FADE:
            backlight_hw_brightness(0);
            backlight_control = BACKLIGHT_CONTROL_IDLE;
            break;
        default:
            backlight_control = BACKLIGHT_CONTROL_IDLE;
            break;
    }
}
#endif /* BOOTLOADER */

static void __backlight_dim(bool dim_now)
{
    /* dont let the interrupt tick happen */
    backlight_control = BACKLIGHT_CONTROL_IDLE;
    backlight_target = dim_now ? 0 : _backlight_brightness;
    if(backlight_target==0 && _backlight_brightness==0)
    {
        if(dim_now == false)
            backlight_control = BACKLIGHT_CONTROL_ON;
        else
            backlight_control = BACKLIGHT_CONTROL_OFF;
    }
    else
        backlight_control = BACKLIGHT_CONTROL_FADE;
}

void backlight_hw_on(void)
{
#ifdef HAVE_LCD_ENABLE
    lcd_enable(true); /* power on lcd + visible display */
#endif
    __backlight_dim(false);
}

void backlight_hw_off(void)
{
    __backlight_dim(true);
}


bool backlight_hw_init(void)
{
    unsigned char brightness = log_brightness[DEFAULT_BRIGHTNESS_SETTING];
    _backlight_brightness = brightness;

    backlight_control = BACKLIGHT_CONTROL_ON;

    _backlight_set_state (1);
    S3C2440_GPIO_CONFIG (GPGCON, 4, GPIO_OUTPUT);    
    
#ifndef BOOTLOADER
    /* put the led control on the tick list */
    tick_add_task(led_control_service);
#endif
    return true;
}
