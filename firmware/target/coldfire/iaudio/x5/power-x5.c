/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
#include "pcf50606.h"

#ifndef SIMULATOR

void power_init(void)
{
    /* Charger detect */
    and_l(~0x01000000, &GPIO1_ENABLE);
    or_l(0x01000000, &GPIO1_FUNCTION);
    
    pcf50606_init();
}

bool charger_inserted(void)
{     
    return (GPIO1_READ & 0x01000000)?true:false;
}

void ide_power_enable(bool on)
{
    (void)on;
}

bool ide_powered(void)
{
    return false;
}

void power_off(void)
{
    set_irq_level(HIGHEST_IRQ_LEVEL);
    and_l(~0x00000008, &GPIO_OUT);
    while(1)
        yield();
}

#else

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
}

void ide_power_enable(bool on)
{
   (void)on;
}

#endif /* SIMULATOR */
