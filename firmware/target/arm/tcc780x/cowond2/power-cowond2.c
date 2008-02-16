/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Dave Chapman
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "cpu.h"
#include "kernel.h"
#include "system.h"
#include "power.h"

#ifndef SIMULATOR

void power_init(void)
{
    #warning function not implemented
}

void ide_power_enable(bool on)
{
    #warning function not implemented
    (void)on;
}

bool ide_powered(void)
{
    #warning function not implemented
    return true;
}

void power_off(void)
{
    #warning function not implemented
}

#else /* SIMULATOR */

bool charger_inserted(void)
{
    return false;
}

void charger_enable(bool on)
{
    (void)on;
}

void power_off(void)
{
    /* Disable interrupts on this core */
    set_interrupt_status(IRQ_FIQ_DISABLED, IRQ_FIQ_STATUS);

    /* Mask them on both cores */
    CPU_INT_CLR = -1;
    COP_INT_CLR = -1;

    /* Shutdown: stop XIN oscillator */
    CLKCTRL &= ~(1 << 31);

    /* Halt everything and wait for device to power off */
    while (1)
    {
        CPU_CTL = PROC_SLEEP;
        COP_CTL = PROC_SLEEP;
    }
}

void ide_power_enable(bool on)
{
   (void)on;
}

#endif /* SIMULATOR */
