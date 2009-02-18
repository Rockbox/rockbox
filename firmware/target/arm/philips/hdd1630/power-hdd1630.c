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
#include "adc.h"
#include "kernel.h"
#include "system.h"
#include "power.h"
#include "logf.h"
#include "usb.h"
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

    syn_init();
#endif
}

unsigned int power_input_status(void)
{
    unsigned int status = POWER_INPUT_NONE;

    /* AC charger */
    if (GPIOE_INPUT_VAL & 0x20)
        status |= POWER_INPUT_MAIN_CHARGER;

    /* Do nothing with USB for now
    if (GPIOE_INPUT_VAL & 0x4)
        status |= POWER_INPUT_USB_CHARGER;
    */

    return status;
}

void ide_power_enable(bool on)
{
    (void)on;
    /* We do nothing */
}

bool ide_powered(void)
{
    /* pretend we are always powered - we don't turn it off */
    return true;
}

void power_off(void)
{
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
