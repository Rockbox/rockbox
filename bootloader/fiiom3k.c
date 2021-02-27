/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include "system.h"
#include "core_alloc.h"
#include "kernel.h"
#include "kernel/kernel-internal.h"
#include "file_internal.h"
#include "i2c.h"
#include "power.h"
#include "lcd.h"
#include "adc.h"
#include "backlight-target.h"
#include "button.h"

extern void show_logo(void);

void main(void)
{
    system_init();
    core_allocator_init();
    kernel_init();
    filesystem_init();
    i2c_init();
    power_init();
    enable_irq();

    lcd_init();

    adc_init();

    backlight_hw_init();
    backlight_hw_on();
    buttonlight_hw_on();
    backlight_hw_brightness(MAX_BRIGHTNESS_SETTING);
    buttonlight_hw_brightness(MAX_BRIGHTNESS_SETTING);

    button_init();

    show_logo();
    sleep(HZ);

    dbg_hw_info();
    sleep(HZ/2);

    if(button_read_device() == (BUTTON_POWER|BUTTON_VOL_DOWN))
        system_reboot();
    else
        power_off();
}
