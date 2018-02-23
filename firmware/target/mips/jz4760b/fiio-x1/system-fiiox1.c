/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2017 by Amaury Pouly
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
#include "gpio-jz4760b.h"

static int hw_ver = 0;

int fiiox1_get_hw_version(void)
{
    if(hw_ver != 0)
        return hw_ver;
    /* use PA28 to detect hardware version: GPIO out, no pullup, set to 1 to see if there is a pulldown */
    jz_gpio_setup_std_out(0, 28, true);
    if(jz_gpio_get_input(0, 28))
        hw_ver = 1;
    else
        hw_ver = 2;
    jz_gpio_set_output(0, 28, false);
    return hw_ver;
}

void fiiox1_enable_blue_led(bool en)
{
    jz_gpio_setup_std_out(1, 16, en); /* PB16 */
}
