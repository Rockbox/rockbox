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

#include "gpio-x1000.h"
#include "kernel.h"

#ifndef BOOTLOADER_SPL
struct mutex gpio_z_mutex;
#endif

void gpio_init(void)
{
#ifndef BOOTLOADER_SPL
    mutex_init(&gpio_z_mutex);
#endif

    /* Set all pins to input state */
    for(int i = 0; i < 4; ++i) {
        jz_clr(GPIO_INT(GPIO_Z), 0xffffffff);
        jz_set(GPIO_MSK(GPIO_Z), 0xffffffff);
        jz_set(GPIO_PAT1(GPIO_Z), 0xffffffff);
        jz_clr(GPIO_PAT0(GPIO_Z), 0xffffffff);
        REG_GPIO_Z_GID2LD = i;
    }

    /* Clear flag and disable pull resistor */
    for(int i = 0; i < 4; ++i) {
        jz_clr(GPIO_FLAG(i), 0xffffffff);
        jz_set(GPIO_PULL(i), 0xffffffff);
    }
}

void gpio_lock(void)
{
#ifndef BOOTLOADER_SPL
    mutex_lock(&gpio_z_mutex);
#endif
}

void gpio_unlock(void)
{
#ifndef BOOTLOADER_SPL
    mutex_unlock(&gpio_z_mutex);
#endif
}

void gpio_config(int port, unsigned pinmask, int func)
{
    unsigned intr = REG_GPIO_INT(port);
    unsigned mask = REG_GPIO_MSK(port);
    unsigned pat1 = REG_GPIO_PAT1(port);
    unsigned pat0 = REG_GPIO_PAT0(port);

    gpio_lock();
    if(func & 8) jz_set(GPIO_INT(GPIO_Z), (intr & pinmask) ^ pinmask);
    else         jz_clr(GPIO_INT(GPIO_Z), (~intr & pinmask) ^ pinmask);
    if(func & 4) jz_set(GPIO_MSK(GPIO_Z), (mask & pinmask) ^ pinmask);
    else         jz_clr(GPIO_MSK(GPIO_Z), (~mask & pinmask) ^ pinmask);
    if(func & 2) jz_set(GPIO_PAT1(GPIO_Z), (pat1 & pinmask) ^ pinmask);
    else         jz_clr(GPIO_PAT1(GPIO_Z), (~pat1 & pinmask) ^ pinmask);
    if(func & 1) jz_set(GPIO_PAT0(GPIO_Z), (pat0 & pinmask) ^ pinmask);
    else         jz_clr(GPIO_PAT0(GPIO_Z), (~pat0 & pinmask) ^ pinmask);
    REG_GPIO_Z_GID2LD = port;
    gpio_unlock();
    gpio_set_pull(port, pinmask, func & 16);
}
