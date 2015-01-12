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

#ifndef BACKLIGHT_TARGET_H
#define BACKLIGHT_TARGET_H

bool backlight_hw_init(void); /* Returns backlight current state (true=ON). */
void backlight_hw_on(void);
void backlight_hw_off(void);

#ifdef HAVE_REMOTE_LCD
void remote_backlight_hw_on(void);
void remote_backlight_hw_off(void);
#endif

#ifndef BOOTLOADER
#define _backlight_on_isr() backlight_hw_on()
#define _backlight_off_isr() backlight_hw_off()
#define _backlight_on_normal() backlight_hw_on()
#define _backlight_off_normal() backlight_hw_off()
#define _BACKLIGHT_FADE_BOOST
#endif

/* Button lights are controlled by GPIOA_OUTPUT_VAL */
#define BUTTONLIGHT_PLAY     0x01
#define BUTTONLIGHT_MENU     0x02
#define BUTTONLIGHT_DISPLAY  0x04
#define BUTTONLIGHT_LEFT     0x08
#define BUTTONLIGHT_RIGHT    0x10
#define BUTTONLIGHT_SCROLL   0x20
#define BUTTONLIGHT_ALL      (BUTTONLIGHT_PLAY    | BUTTONLIGHT_MENU  | \
                              BUTTONLIGHT_DISPLAY | BUTTONLIGHT_LEFT  | \
                              BUTTONLIGHT_RIGHT   | BUTTONLIGHT_SCROLL)

void buttonlight_hw_brightness(int brightness);
void buttonlight_hw_on(void);
void buttonlight_hw_off(void);

#endif
