/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Mark Arigo
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

#include "config.h"
#include "cpu.h"
#include <stdbool.h>
#include "kernel.h"
#include "system.h"
#include "power.h"
#include "logf.h"
#include "usb.h"
#include "backlight-target.h"
#include "synaptics-mep.h"

void power_init(void)
{
    /* power off bit */
    GPIOB_ENABLE |= 0x80;
    GPIOB_OUTPUT_VAL |= 0x80;
    GPIOB_OUTPUT_EN |= 0x80;

    /* charger inserted bit */
    GPIOE_ENABLE |= 0x20;
    GPIOE_INPUT_VAL |= 0x20;

#if CONFIG_TUNER
    /* fm antenna? */
    GPIOE_ENABLE |= 0x40;
    GPIOE_OUTPUT_EN |= 0x40;
    GPIOE_OUTPUT_VAL &= ~0x40; /* off */
#endif

#ifndef BOOTLOADER
    /* enable touchpad here because we need it for
       both buttons and button lights */
    GPO32_ENABLE     |=  0x80;
    GPO32_VAL        &= ~0x80;
    udelay(1000);

    GPIOD_ENABLE |= 0x80;          /* enable ACK */
    GPIOA_ENABLE |= (0x10 | 0x20); /* enable DATA, CLK */

    GPIOD_OUTPUT_EN  |=  0x80; /* set ACK */
    GPIOD_OUTPUT_VAL |=  0x80; /*    high */

    GPIOA_OUTPUT_EN  &= ~0x20; /* CLK */

    GPIOA_OUTPUT_EN  |=  0x10; /* set DATA */
    GPIOA_OUTPUT_VAL |=  0x10; /*     high */

    if (!touchpad_init())
    {
        logf("touchpad not ready");
    }
#if defined(PHILIPS_HDD6330)
    /* reduce transmission overhead */
    touchpad_set_parameter(0,0x21,0x0008);
    /* set GPO_LEVELS = 0 - for the buttonlights */
    touchpad_set_parameter(0,0x23,0x0000);
    touchpad_set_parameter(1,0x22,0x0000);
#endif

#endif
}

unsigned int power_input_status(void)
{
    unsigned int status = POWER_INPUT_NONE;

    /* AC charger */
    if (GPIOE_INPUT_VAL & 0x20)
        status |= POWER_INPUT_MAIN_CHARGER;

    if (GPIOE_INPUT_VAL & 0x4)
        status |= POWER_INPUT_USB_CHARGER;

    return status;
}

void ide_power_enable(bool on)
{
#if defined(PHILIPS_HDD6330)
    if (on)
    {
        GPIO_SET_BITWISE(GPIOC_OUTPUT_VAL, 0x08);
        DEV_EN |= DEV_IDE0;
    }
    else
    {
        DEV_EN &= ~DEV_IDE0;
        GPIO_CLEAR_BITWISE(GPIOC_OUTPUT_VAL, 0x08);
    }
#else
    (void)on;
    /* We do nothing */
#endif
}

bool ide_powered(void)
{
#if defined(PHILIPS_HDD6330)
    return ((GPIOC_INPUT_VAL & 0x08) != 0);
#else
    /* pretend we are always powered - we don't turn it off */
    return true;
#endif
}

void power_off(void)
{
    backlight_hw_off();
    sleep(HZ/10);

    /* power off bit */
    GPIOB_ENABLE |= 0x80;
    GPIOB_OUTPUT_VAL &= ~0x80;
    GPIOB_OUTPUT_EN |= 0x80;
}

#if CONFIG_TUNER
bool tuner_power(bool status)
{
    if (status)
        GPIOE_OUTPUT_VAL |= 0x40;
    else
        GPIOE_OUTPUT_VAL &= ~0x40;

    return status;
}
#endif
