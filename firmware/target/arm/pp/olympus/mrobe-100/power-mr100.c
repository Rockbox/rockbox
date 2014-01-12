/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Barry Wardell
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
#include "lcd-remote.h"

void power_init(void)
{
    /* Enable power-off bit */
    GPIOB_ENABLE     |=  0x80;
    GPIOB_OUTPUT_VAL &= ~0x80;
    GPIOB_OUTPUT_EN  |=  0x80;

    /* IDE power */
    GPIOC_ENABLE     |=  0x8;
    GPIOC_OUTPUT_VAL &= ~0x8;
    GPIOC_OUTPUT_EN  |=  0x8;
}

unsigned int power_input_status(void)
{
    return (GPIOL_INPUT_VAL & 0x24) ?
        POWER_INPUT_MAIN_CHARGER : POWER_INPUT_NONE;
}

void ide_power_enable(bool on)
{
    if(on)
    {
        GPIO_CLEAR_BITWISE(GPIOC_OUTPUT_VAL, 0x08);
        DEV_EN |= DEV_IDE0;
    }
    else
    {
        DEV_EN &= ~DEV_IDE0;
        GPIO_SET_BITWISE(GPIOC_OUTPUT_VAL, 0x08);
    }
}


bool ide_powered(void)
{
    return ((GPIOC_INPUT_VAL & 0x8) == 0);
}

void power_off(void)
{
#ifdef HAVE_REMOTE_LCD
    lcd_remote_off();
#endif

    /* Disable interrupts on this core */
    disable_interrupt(IRQ_FIQ_STATUS);

    /* Mask them on both cores */
    CPU_INT_DIS = -1;
    COP_INT_DIS = -1;

    while (1)
        GPIOB_OUTPUT_VAL |= 0x80;
}
