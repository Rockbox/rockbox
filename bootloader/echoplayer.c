/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 by Aidan MacDonald
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
#include "kernel/kernel-internal.h"
#include "system.h"
#include "power.h"
#include "rtc.h"
#include "lcd.h"
#include "backlight.h"
#include "button.h"

extern void show_logo(void);

void main(void)
{
    system_init();
    kernel_init();
    rtc_init();

    lcd_init();
    backlight_init();
    backlight_on();

    show_logo();

    while (1) {
        lcd_putsxyf(60, 140, "btn: %08x", button_read_device());
        lcd_update();
    }
}
