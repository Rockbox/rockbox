/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2015 Lorenzo Miori
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

#include "bluetooh.h"
#include "gpio-ypr.h"

/** Internal functions **/

static void gpio_init(void)
{
    /* REG_EN */
    gpio_set_iomux(GPIO_REG_EN, IOMUX_CONFIG_ALT3);
    gpio_set_pad(GPIO_REG_EN, PAD_CTL_DRV_HIGH | PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE | PAD_CTL_SRE_FAST);
    gpio_direction_output(GPIO_REG_EN);
    gpio_set(GPIO_REG_EN, 0);

    /* BT_RST */
    gpio_set_iomux(GPIO_BT_RST, IOMUX_CONFIG_ALT4);
    gpio_set_pad(GPIO_BT_RST, PAD_CTL_DRV_HIGH | PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE | PAD_CTL_SRE_FAST);
    gpio_direction_output(GPIO_BT_RST);
    gpio_set(GPIO_BT_RST, 0);

    /* BT_WAKE */
    mxc_request_iomux(GPIO_BT_WAKE, IOMUX_CONFIG_ALT4); 
    mxc_iomux_set_pad(GPIO_BT_WAKE, PAD_CTL_DRV_HIGH | PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE | PAD_CTL_SRE_FAST);
    gpio_direction_output(GPIO_BT_WAKE);
    gpio_set(GPIO_BT_WAKE, 0);
}

/** Bluetooth HAL API **/

void bluetooth_init(void)
{
    gpio_init();
}

void bluetooth_power(bool asserted)
{
    gpio_set(GPIO_REG_EN, asserted);
}

void bluetooth_reset(bool asserted)
{
    gpio_set(GPIO_BT_RST, asserted);
}

void bluetooth_awake(bool asserted)
{
    gpio_set(GPIO_BT_WAKE, asserted);
}

bool bluetooth_is_powered(void)
{
    return gpio_get(GPIO_REG_EN);
}

bool bluetooth_is_reset(void)
{
    return gpio_get(GPIO_BT_RST);
}

bool bluetooth_is_awake(void)
{
    return gpio_get(GPIO_BT_WAKE);
}
