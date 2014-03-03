/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2013 Lorenzo Miori
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

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include "system.h"
#include "panic.h"
#include "debug.h"
#include "hostfs.h"

#include "gpio-ypr.h"
#include "pmu-ypr1.h"
#include "ioctl-ypr1.h"
#include "audiohw.h"
#include "button-target.h"

void power_off(void)
{
    /* Something that we need to do before exit on our platform */
    pmu_close();
    max17040_close();
    button_close_device();
    gpio_close();
    exit(EXIT_SUCCESS);
}

uintptr_t *stackbegin;
uintptr_t *stackend;
void system_init(void)
{
    int *s;
    /* fake stack, OS manages size (and growth) */
    stackbegin = stackend = (uintptr_t*)&s;

    /* Here begins our platform specific initilization for various things */
    audiohw_init();
    gpio_init();
    max17040_init();
    pmu_init();
}

void system_reboot(void)
{
    power_off();
}

void system_exception_wait(void)
{
    system_reboot();
}

int hostfs_init()
{
    /* stub */
    return 0;
}

int hostfs_flush(void)
{
    sync();

    return 0;
}
