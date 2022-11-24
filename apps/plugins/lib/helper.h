/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Peter D'Hoye
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
#ifndef _LIB_HELPER_H_
#define _LIB_HELPER_H_

#include "plugin.h"

#ifndef MAX_BRIGHTNESS_SETTING
#define MAX_BRIGHTNESS_SETTING 0
#endif
#ifndef MIN_BRIGHTNESS_SETTING
#define MIN_BRIGHTNESS_SETTING 0
#endif
#ifndef DEFAULT_BRIGHTNESS_SETTING
#define DEFAULT_BRIGHTNESS_SETTING 0
#endif

int talk_val(long n, int unit, bool enqueue);

/**
 * Backlight on/off operations
 */
void backlight_force_on(void);
void backlight_ignore_timeout(void);
void backlight_use_settings(void);

/**
 * Disable and restore software poweroff (i.e. holding PLAY on iPods).
 * Only call _restore() if _disable() was called earlier!
 */
void sw_poweroff_disable(void);
void sw_poweroff_restore(void);

void remote_backlight_force_on(void);
void remote_backlight_ignore_timeout(void);
void remote_backlight_use_settings(void);

void buttonlight_force_on(void);
void buttonlight_force_off(void);
void buttonlight_ignore_timeout(void);
void buttonlight_use_settings(void);

/**
 * Backlight brightness adjustment settings
 */
void backlight_brightness_set(int brightness);
void backlight_brightness_use_setting(void);

void buttonlight_brightness_set(int brightness);
void buttonlight_brightness_use_setting(void);

#endif /* _LIB_HELPER_H_ */
