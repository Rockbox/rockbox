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
#include "ascodec.h"
#include "tuner.h"
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

bool charger_inserted(void)
{
#ifdef SANSA_E200
    if(GPIOB_INPUT_VAL & 0x10)
#else /* SANSA_C200 */
    if(GPIOH_INPUT_VAL & 0x2)
#endif
        return true;
    return false;
}

void ide_power_enable(bool on)
{
    (void)on;
}

#if CONFIG_TUNER

/** Tuner **/
static bool powered = false;

bool tuner_power(bool status)
{
    bool old_status;
    lv24020lp_lock();

    old_status = powered;

    if (status != old_status)
    {
        if (status)
        {
            /* init mystery amplification device */
#if defined(SANSA_E200)
            GPO32_ENABLE |= 0x1;
#else /* SANSA_C200 */
            DEV_INIT2 &= ~0x800;
#endif
            udelay(5);

            /* When power up, host should initialize the 3-wire bus
               in host read mode: */

            /* 1. Set direction of the DATA-line to input-mode. */
            GPIOH_OUTPUT_EN &= ~(1 << 5); 
            GPIOH_ENABLE |= (1 << 5); 

            /* 2. Drive NR_W low */
            GPIOH_OUTPUT_VAL &= ~(1 << 3); 
            GPIOH_OUTPUT_EN |= (1 << 3); 
            GPIOH_ENABLE |= (1 << 3); 

            /* 3. Drive CLOCK high */
            GPIOH_OUTPUT_VAL |= (1 << 4); 
            GPIOH_OUTPUT_EN |= (1 << 4); 
            GPIOH_ENABLE |= (1 << 4);

            lv24020lp_power(true);
        }
        else
        {
            lv24020lp_power(false);

            /* set all as inputs */
            GPIOH_OUTPUT_EN &= ~((1 << 5) | (1 << 3) | (1 << 4));
            GPIOH_ENABLE &= ~((1 << 3) | (1 << 4)); 

            /* turn off mystery amplification device */
#if defined (SANSA_E200)
            GPO32_ENABLE &= ~0x1;
#else
            DEV_INIT2 |= 0x800;
#endif
        }

        powered = status;
    }

    lv24020lp_unlock();
    return old_status;
}

#endif /* CONFIG_TUNER */
