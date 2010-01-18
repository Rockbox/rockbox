/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
 *
 * Copyright (C) 2009 by Szymon Dziok
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
#include "synaptics-mep.h"

void power_init(void)
{
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
    /* Sound unmute (on) */
    GPIO_CLEAR_BITWISE(GPIOL_OUTPUT_VAL, 0x10);
}

unsigned int power_input_status(void)
{
    unsigned int status = POWER_INPUT_NONE;
    /* GPIOL - external charger connected  */
    if (GPIOL_INPUT_VAL & 0x20)
    status = POWER_INPUT_MAIN_CHARGER;
    /* GPIOL - usb connected */
    if (GPIOL_INPUT_VAL & 0x04)
    status |= POWER_INPUT_USB_CHARGER;

    return status;
}

void ide_power_enable(bool on)
{
    if(on){
        GPIO_SET_BITWISE(GPIOC_OUTPUT_VAL, 0x08);
        DEV_EN |= DEV_IDE0;
    } else
    {
        DEV_EN &= ~DEV_IDE0;
        GPIO_CLEAR_BITWISE(GPIOC_OUTPUT_VAL, 0x08);
    }
}

bool ide_powered(void)
{
    return ((GPIOC_INPUT_VAL & 0x08) == 1);
}

void power_off(void)
{
    /* from the OF */
/*
    DEV_INIT2 |= DEV_I2S;
    GPIO_SET_BITWISE(GPIOL_OUTPUT_VAL, 0x10);
    sleep(HZ/100);
    GPIO_SET_BITWISE(GPIOL_OUTPUT_VAL, 0x10);
    sleep(HZ);
    GPIO_CLEAR_BITWISE(GPIOB_OUTPUT_VAL, 0x80);
    sleep(HZ);
    GPIO_CLEAR_BITWISE(GPIOC_OUTPUT_VAL, 0x08);
    GPO32_VAL |= 0x40000000;
    GPO32_ENABLE |= 0x40000000;
*/
    /* Sound mute (off) */
    DEV_INIT2 |= DEV_I2S;
    GPIO_SET_BITWISE(GPIOL_OUTPUT_VAL, 0x10);
    /* shutdown bit */
    GPIO_CLEAR_BITWISE(GPIOB_OUTPUT_VAL, 0x80);
}
