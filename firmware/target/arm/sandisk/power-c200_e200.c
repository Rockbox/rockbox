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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
#include "as3514.h"

void power_init(void)
{
}

void power_off(void)
{
    char byte;

    /* Disable interrupts on this core */
    set_interrupt_status(IRQ_FIQ_DISABLED, IRQ_FIQ_STATUS);

    /* Mask them on both cores */
    CPU_INT_CLR = -1;
    COP_INT_CLR = -1;

    /* Send shutdown command to PMU */
    byte = i2c_readbyte(AS3514_I2C_ADDR, SYSTEM);
    byte &= ~0x1;   
    pp_i2c_send(AS3514_I2C_ADDR, SYSTEM, byte);

    /* Halt everything and wait for device to power off */
    while (1)
    {
        CPU_CTL = PROC_SLEEP;
        COP_CTL = PROC_SLEEP;
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
