/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Daniel Ankers
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

#include <stdbool.h>
#include "system.h"
#include "cpu.h"
#include "i2c-pp.h"
#include "tuner.h"
#include "ascodec.h"
#include "as3514.h"
#include "power.h"

void power_init(void)
{
}

void power_off(void)
{
    char byte;

    /* Send shutdown command to PMU */
    byte = ascodec_read(AS3514_SYSTEM);
    byte &= ~0x1;   
    ascodec_write(AS3514_SYSTEM, byte);

    /* Stop interrupts on both cores */
    disable_interrupt(IRQ_FIQ_STATUS);
    COP_INT_DIS = -1;
    CPU_INT_DIS = -1;

    /* Halt everything and wait for device to power off */
    while (1)
    {
        COP_CTL = 0x40000000;
        CPU_CTL = 0x40000000;
    }
}

unsigned int power_input_status(void)
{
    return POWER_INPUT_NONE;
}

void ide_power_enable(bool on)
{
    (void)on;
}
