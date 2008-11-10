/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Barry Wardell
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

/* Taken from button-h10.c by Barry Wardell and reverse engineering by MrH. */

#include "system.h"
#include "button.h"
#include "backlight.h"
#include "powermgmt.h"

#define WHEEL_REPEAT_INTERVAL   300000
#define WHEEL_FAST_ON_INTERVAL   20000
#define WHEEL_FAST_OFF_INTERVAL  60000
#define WHEELCLICKS_PER_ROTATION    48 /* wheelclicks per full rotation */

/* Clickwheel */
#ifndef BOOTLOADER
static unsigned int  old_wheel_value   = 0;
static unsigned int  wheel_repeat      = BUTTON_NONE;
static unsigned int  wheel_click_count = 0;
static unsigned int  wheel_delta       = 0;
static          int  wheel_fast_mode   = 0;
static unsigned long last_wheel_usec   = 0;
static unsigned long wheel_velocity    = 0;
static          long last_wheel_post   = 0;
static          long next_backlight_on = 0;
/* Buttons */
static bool hold_button     = false;
static bool hold_button_old = false;
#define _button_hold() hold_button
#else
#define _button_hold() false  /* FIXME */
#endif /* BOOTLOADER */
static int  int_btn         = BUTTON_NONE;

void button_init_device(void)
{
}

bool button_hold(void)
{
    return _button_hold();
}

/* clickwheel */
#ifndef BOOTLOADER
void clickwheel_int(void)
{
}
#endif /* BOOTLOADER */

/* device buttons */
void button_int(void)
{
    int_btn = BUTTON_NONE;
}

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
#ifdef BOOTLOADER
    /* Read buttons directly in the bootloader */
    button_int();
#else
    /* light handling */
    if (hold_button != hold_button_old)
    {
        hold_button_old = hold_button;
        backlight_hold_changed(hold_button);
    }
#endif /* BOOTLOADER */

    /* The int_btn variable is set in the button interrupt handler */
    return int_btn;
}
