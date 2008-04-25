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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef BACKLIGHT_TARGET_H
#define BACKLIGHT_TARGET_H

bool _backlight_init(void); /* Returns backlight current state (true=ON). */
void _backlight_hw_on(void);
void _backlight_hw_off(void);

#ifdef BOOTLOADER
#define _backlight_on() _backlight_hw_on()
#define _backlight_off() _backlight_hw_off()
#else
#define _backlight_on_isr() _backlight_hw_on()
#define _backlight_off_isr() _backlight_hw_off()
#define _backlight_on_normal() _backlight_hw_on()
#define _backlight_off_normal() _backlight_hw_off()
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

void _buttonlight_set_brightness(int brightness);
void _buttonlight_on(void);
void _buttonlight_off(void);

#endif
