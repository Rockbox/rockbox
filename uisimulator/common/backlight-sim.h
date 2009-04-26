/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 * Copyright (C) 2009 by Thomas Martitz
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
#ifdef SIMULATOR

static inline void _backlight_on(void)
{
    sim_backlight(100);
}

static inline void _backlight_off(void)
{
    sim_backlight(0);
}
#ifdef HAVE_BACKLIGHT_BRIGHTNESS
static inline void _backlight_set_brightness(int val)
{
    (void)val;
}
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */
#ifdef HAVE_BUTTON_LIGHT
static inline void _buttonlight_on(void)
{
}

static inline void _buttonlight_off(void)
{
}

#ifdef HAVE_BUTTONLIGHT_BRIGHTNESS
static inline void _buttonlight_set_brightness(int val)
{
    (void)val;
}
#endif /* HAVE_BUTTONLIGHT_BRIGHTNESS */
#endif /* HAVE_BUTTON_LIGHT */

#ifdef HAVE_REMOTE_LCD
static inline void _remote_backlight_on(void)
{
    sim_remote_backlight(100);
}

static inline void _remote_backlight_off(void)
{
    sim_remote_backlight(0);
}
#endif /* HAVE_REMOTE_LCD */

#endif /* SIMULATOR */
