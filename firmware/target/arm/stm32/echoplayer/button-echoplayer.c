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
#include "button.h"
#include "gpio-stm32h7.h"

void button_init_device(void)
{
}

int button_read_device(void)
{
    int buttons = 0;

    /* Most buttons are active low, so invert the read */
    uint32_t ga = ~REG_GPIO_IDR(GPIO_A);
    uint32_t gb = ~REG_GPIO_IDR(GPIO_B);
    uint32_t gc = ~REG_GPIO_IDR(GPIO_C);
    uint32_t gd = ~REG_GPIO_IDR(GPIO_D);
    uint32_t gf = ~REG_GPIO_IDR(GPIO_F);
    uint32_t gh = ~REG_GPIO_IDR(GPIO_H);

    if (ga & BIT_N(GPION_PIN(GPIO_BUTTON_A)))
        buttons |= BUTTON_A;
    if (ga & BIT_N(GPION_PIN(GPIO_BUTTON_B)))
        buttons |= BUTTON_B;
    if (ga & BIT_N(GPION_PIN(GPIO_BUTTON_X)))
        buttons |= BUTTON_X;
    if (ga & BIT_N(GPION_PIN(GPIO_BUTTON_Y)))
        buttons |= BUTTON_A;

    if (!(gb & BIT_N(GPION_PIN(GPIO_BUTTON_SELECT))))
        buttons |= BUTTON_SELECT;
    if (gb & BIT_N(GPION_PIN(GPIO_BUTTON_UP)))
        buttons |= BUTTON_UP;
    if (gb & BIT_N(GPION_PIN(GPIO_BUTTON_VOL_DOWN)))
        buttons |= BUTTON_UP;

    if (gc & BIT_N(GPION_PIN(GPIO_BUTTON_RIGHT)))
        buttons |= BUTTON_RIGHT;

    if (gd & BIT_N(GPION_PIN(GPIO_BUTTON_START)))
        buttons |= BUTTON_START;
    if (gd & BIT_N(GPION_PIN(GPIO_BUTTON_DOWN)))
        buttons |= BUTTON_DOWN;
    if (gd & BIT_N(GPION_PIN(GPIO_BUTTON_LEFT)))
        buttons |= BUTTON_LEFT;

    if (!(gf & BIT_N(GPION_PIN(GPIO_BUTTON_POWER))))
        buttons |= BUTTON_POWER;

    if (gh & BIT_N(GPION_PIN(GPIO_BUTTON_VOL_UP)))
        buttons |= BUTTON_VOL_UP;
    if (gh & BIT_N(GPION_PIN(GPIO_BUTTON_HOLD)))
        buttons |= BUTTON_HOLD;

    return buttons;
}

bool headphones_inserted(void)
{
    return true;
}

bool lineout_inserted(void)
{
    return false;
}
