/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Linus Nielsen Feltzing
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
#ifndef BACKLIGHT_TARGET_H
#define BACKLIGHT_TARGET_H

bool backlight_hw_init(void); /* Returns backlight current state (true=ON). */
void backlight_hw_on(void);
void backlight_hw_off(void);

#ifndef BOOTLOADER
#define _backlight_on_isr() backlight_hw_on()
#define _backlight_off_isr() backlight_hw_off()
#define _backlight_on_normal() backlight_hw_on()
#define _backlight_off_normal() backlight_hw_off()
#define _BACKLIGHT_FADE_BOOST
#endif

void remote_backlight_hw_on(void);
void remote_backlight_hw_off(void);

#endif
