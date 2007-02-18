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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* Created from power.c using some iPod code, and some custom stuff based on 
   GPIO analysis 
*/

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

#if CONFIG_CHARGING == CHARGING_CONTROL
bool charger_enabled;
#endif

#if 0
#if CONFIG_TUNER

static bool powered = false;

bool radio_powered()
{
    return powered;
}

bool radio_power(bool status)
{
    bool old_status = powered;
    powered = status;
#ifdef HAVE_TUNER_PWR_CTRL
    if (status)
    {
        and_b(~0x04, &PADRL); /* drive PA2 low for tuner enable */
        sleep(1); /* let the voltage settle */
    }
    else
        or_b(0x04, &PADRL); /* drive PA2 high for tuner disable */
#endif
    return old_status;
}

#endif /* #if CONFIG_TUNER */
#endif

void power_init(void)
{
}

bool charger_inserted(void)
{     
    return (GPIOF_INPUT_VAL & 0x08)?true:false;
}

void ide_power_enable(bool on)
{
    if(on){
        GPIOF_OUTPUT_VAL &=~ 0x1;
        sleep(1);
    } else {
        GPIOF_OUTPUT_VAL |= 0x1;
    }
}


bool ide_powered(void)
{
    return ((GPIOF_INPUT_VAL & 0x1) == 0);
}

void power_off(void)
{
    GPIOF_OUTPUT_VAL &=~ 0x20;
}
