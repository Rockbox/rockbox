/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Vortex
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
#include "kernel.h"
#include "audiohw.h"

void wm8740_hw_init(void)
{
    GPIO_PADR &= ~(1<<0); /* MD */
    GPIO_PACON |= (1<<0);

    GPIO_PADR &= ~(1<<1); /* MC */
    GPIO_PACON |= (1<<1);

    SCU_IOMUXB_CON &= ~(1<<2);
    GPIO_PCDR |= (1<<4);  /* ML */
    GPIO_PCCON |= (1<<4);
}

void wm8740_set_md(const int val)
{
    if (val)
        GPIO_PADR |= (1<<0);
    else
        GPIO_PADR &= ~(1<<0);
}

void wm8740_set_mc(const int val)
{
    if (val)
        GPIO_PADR |= (1<<1);
    else
        GPIO_PADR &= ~(1<<1);
}

void wm8740_set_ml(const int val)
{
    if (val)
        GPIO_PCDR |= (1<<4);
    else
        GPIO_PCDR &= ~(1<<4);
}

static void pop_ctrl(const int val)
{
    if (val)
        GPIO_PADR |= (1<<7);
    else
        GPIO_PADR &= ~(1<<7);
}

void audiohw_postinit(void)
{
    pop_ctrl(0);
    sleep(HZ/4);
    wm8740_hw_init();
    audiohw_init();
    sleep(HZ/2);
    pop_ctrl(1);
    sleep(HZ/4);
    audiohw_unmute();
}

void audiohw_close(void)
{
    audiohw_mute();
    pop_ctrl(0);
    sleep(HZ/4);
}
