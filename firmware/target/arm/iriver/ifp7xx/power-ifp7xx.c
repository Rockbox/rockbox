/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
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
#include "adc.h"
#include "kernel.h"
#include "system.h"
#include "power.h"
#include "hwcompat.h"
#include "logf.h"
#include "usb.h"

#if CONFIG_TUNER

static bool powered = false;

bool radio_powered(void)
{
    return powered;
}

bool radio_power(bool status)
{
    bool old_status = powered;
    powered = status;

    return old_status;
}

#endif /* #if CONFIG_TUNER */

#ifndef SIMULATOR

void power_init(void)
{
}

void ide_power_enable(bool on)
{
    (void)on;
    /* no ide controller */
}

bool ide_powered(void)
{
    return true; /* pretend always powered if not controlable */
}

void power_off(void)
{
    set_irq_level(HIGHEST_IRQ_LEVEL);
    GPIO1_CLR = 1 << 16;
    GPIO2_SET = 1;
    while(1)
        yield();
}

#else

void power_off(void)
{
}

void ide_power_enable(bool on)
{
   (void)on;
}

#endif /* SIMULATOR */
