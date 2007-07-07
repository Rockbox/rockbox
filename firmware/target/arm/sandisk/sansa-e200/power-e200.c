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
    byte = i2c_readbyte(0x46, 0x20);
    byte &= ~0x1;   
    pp_i2c_send(0x46, 0x20, byte);

    /* Halt everything and wait for device to power off */
    while (1)
    {
        CPU_CTL = PROC_SLEEP;
        COP_CTL = PROC_SLEEP;
    }
}

bool charger_inserted(void)
{
    if(GPIOB_INPUT_VAL & 0x10)
        return true;
    return false;
}

void ide_power_enable(bool on)
{
    (void)on;
}
