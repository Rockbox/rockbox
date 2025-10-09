/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2025 Mauricio G.
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
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "config.h"
#include "backlight-target.h"
#include "sysfs.h"
#include "panic.h"
#include "lcd.h"
#include "debug.h"

#include <3ds/services/gsplcd.h>
#include <3ds/result.h>
#include "luminance-ctru.h"

/* TODO: To use calibrated values in rockbox,
   MIN_BRIGHTNESS_SETTING (etc) would need to be
   declared as constants in settings-list.c */

u32 ctru_min_lum = 16;
u32 ctru_max_lum = 142;
u32 ctru_luminance = 28;

/* FIXME: After calling gspLcdInit() the home menu will no longer be
   accesible, to fix this we have to call gspLcdInit/gspLcdExit as
   a mutex lock/unlock when using gsplcd.h functions. */
void lcd_mutex_lock(void)
{
    Result res = gspLcdInit();
    if (R_FAILED(res)) {
        DEBUGF("backlight_hw_init: gspLcdInit failed.\n");
    }
}

void lcd_mutex_unlock(void)
{
    gspLcdExit();
}

bool backlight_hw_init(void)
{   
    /* read calibrated values */
    ctru_luminance = getCurrentLuminance(false);
    ctru_min_lum = getMinLuminancePreset();
    ctru_max_lum = getMaxLuminancePreset();
    
    backlight_hw_on();
    backlight_hw_brightness(DEFAULT_BRIGHTNESS_SETTING);
    return true;
}

static int last_bl = -1;

void backlight_hw_on(void)
{
    lcd_mutex_lock();
    if (last_bl != 1) {
#ifdef HAVE_LCD_ENABLE
        lcd_enable(true);
#endif
        GSPLCD_PowerOnAllBacklights();
	last_bl = 1;
    }
    lcd_mutex_unlock();
}

void backlight_hw_off(void)
{
    lcd_mutex_lock();
    if (last_bl != 0) {
        /* only power off rockbox ui screen */
        GSPLCD_PowerOffBacklight(GSPLCD_SCREEN_BOTTOM);
#ifdef HAVE_LCD_ENABLE
        lcd_enable(false);
#endif
	last_bl = 0;
    }
    lcd_mutex_unlock();
}

void backlight_hw_brightness(int brightness)
{
    /* cap range, just in case */
    if (brightness > MAX_BRIGHTNESS_SETTING)
        brightness = MAX_BRIGHTNESS_SETTING;
    if (brightness < MIN_BRIGHTNESS_SETTING)
        brightness = MIN_BRIGHTNESS_SETTING;

    /* normalize level on both screens */
    lcd_mutex_lock();
    GSPLCD_SetBrightnessRaw(GSPLCD_SCREEN_TOP | GSPLCD_SCREEN_BOTTOM, (u32) brightness);
    lcd_mutex_unlock();
}
