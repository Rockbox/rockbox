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

/**
 * Backlight on/off operations
 */
void backlight_force_on(void);
void backlight_use_settings(void);
#ifdef HAVE_REMOTE_LCD
void remote_backlight_force_on(void);
void remote_backlight_use_settings(void);
#endif
#ifdef HAVE_BUTTON_LIGHT
void buttonlight_force_on(void);
void buttonlight_use_settings(void);
#endif

/**
 * Backlight brightness adjustment settings
 */
#ifdef HAVE_BACKLIGHT_BRIGHTNESS
void backlight_brightness_set(int brightness);
void backlight_brightness_use_setting(void);
#endif


#endif /* _LIB_HELPER_H_ */
