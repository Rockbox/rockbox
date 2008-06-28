/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Mark Arigo
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
#include "backlight-target.h"
#include "system.h"
#include "lcd.h"
#include "backlight.h"

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
static unsigned short backlight_brightness = DEFAULT_BRIGHTNESS_SETTING;

void _backlight_set_brightness(int brightness)
{
}
#endif

void _backlight_on(void)
{
}

void _backlight_off(void)
{
}

#ifdef HAVE_BUTTON_LIGHT
void _buttonlight_on(void)
{
}

void _buttonlight_off(void)
{
}
#endif
