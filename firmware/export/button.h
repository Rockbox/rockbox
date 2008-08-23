/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg
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
#ifndef _BUTTON_H_
#define _BUTTON_H_

#include <stdbool.h>
#include <inttypes.h>
#include "config.h"
#ifndef __PCTOOL__
#include "button-target.h"
#endif

extern struct event_queue button_queue;

void button_init (void);
void button_close(void);
int button_queue_count(void);
long button_get (bool block);
long button_get_w_tmo(int ticks);
intptr_t button_get_data(void);
int button_status(void);
void button_clear_queue(void);
#ifdef HAVE_LCD_BITMAP
void button_set_flip(bool flip); /* turn 180 degrees */
#endif
#ifdef HAVE_BACKLIGHT
void set_backlight_filter_keypress(bool value);
#ifdef HAVE_REMOTE_LCD
void set_remote_backlight_filter_keypress(bool value);
#endif
#endif

#ifdef HAVE_HEADPHONE_DETECTION
bool headphones_inserted(void);
#endif
#ifdef HAVE_WHEEL_POSITION
int wheel_status(void);
void wheel_send_events(bool send);
#endif

#ifdef HAVE_SCROLLWHEEL
int button_apply_acceleration(const unsigned int data);
#endif

#define BUTTON_NONE        0x00000000

/* Button modifiers */
#define BUTTON_REL         0x02000000
#define BUTTON_REPEAT      0x04000000
#define BUTTON_TOUCHSCREEN 0x08000000

#ifdef HAVE_TOUCHSCREEN
#if !defined(BUTTON_TOPLEFT) || !defined(BUTTON_TOPMIDDLE) \
 || !defined(BUTTON_TOPRIGHT) || !defined(BUTTON_MIDLEFT) \
 || !defined(BUTTON_CENTER) || !defined(BUTTON_MIDRIGHT) \
 || !defined(BUTTON_BOTTOMLEFT) || !defined(BUTTON_BOTTOMMIDDLE) \
 || !defined(BUTTON_BOTTOMRIGHT)
#error Touchscreen button mode BUTTON_* defines not set up correctly
#endif
enum touchscreen_mode {
    TOUCHSCREEN_POINT = 0, /* touchscreen returns pixel co-ords */
    TOUCHSCREEN_BUTTON,    /* touchscreen returns BUTTON_* area codes
                                  actual pixel value will still be accessable
                                  from button_get_data */
};
/* maybe define the number of buttons in button-target.h ? */
void touchscreen_set_mode(enum touchscreen_mode mode);
enum touchscreen_mode touchscreen_get_mode(void);
#endif

#endif /* _BUTTON_H_ */
