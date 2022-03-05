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
#include "clk-x1000.h"
#include "spl-x1000.h"
#include "gpio-x1000.h"

const struct spl_boot_option spl_boot_options[] = {
    [BOOT_OPTION_ROCKBOX] = {
        .storage_addr = 0x6800,
        .storage_size = 102 * 1024,
        .load_addr = X1000_DRAM_BASE,
        .exec_addr = X1000_DRAM_BASE,
        .flags = BOOTFLAG_UCLPACK,
    },
};

int spl_get_boot_option(void)
{
    return BOOT_OPTION_ROCKBOX;
}

void spl_error(void)
{
    /* Flash the backlight */
    int level = 0;
    while(1) {
        gpio_set_function(GPIO_PC(25), GPIOF_OUTPUT(level));
        mdelay(100);
        level = 1 - level;
    }
}
