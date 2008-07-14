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
#include "backlight-target.h"

#define GPIO_PWM    123
#define PWM_CHN     7
#define PWM_FULL    101

static void set_backlight(int unk, int val)
{
    if(val == 0)
        __gpio_as_pwm7();
    else
    {
        REG_TCU_TCSR(7) |= 2;
        REG_TCU_TCSR(7) &= ~0x100;
        int tmp;
        tmp = (unk/2 + __cpm_get_rtcclk()) / unk;
        if(tmp > 0xFFFF)
            tmp = 0xFFFF;
        
        __tcu_set_half_data(7, (tmp * unk * 1374389535) >> 5);
        __tcu_set_full_data(7, tmp);
        
        REG_TCU_TSCR = (1 << 7);
        REG_TCU_TESR = (1 << 7);

        __tcu_enable_pwm_output(7);
    }
    __tcu_set_count(7, 0);
}

bool _backlight_init(void)
{
    __gpio_as_pwm7();
    
    __tcu_stop_counter(7);
    __tcu_disable_pwm_output(7);
    
    set_backlight(300, 7);
    
    return true;
}
void _backlight_on(void)
{
    set_backlight(300, 7);
}
void _backlight_off(void)
{
    set_backlight(300, 0);
}

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
void _backlight_set_brightness(int brightness)
{
    (void)brightness;
    return;
}
#endif
