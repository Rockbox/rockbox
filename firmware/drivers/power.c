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
#include "sh7034.h"
#include <stdbool.h>
#include "config.h"
#include "adc.h"
#include "power.h"

#ifndef SIMULATOR

bool charger_inserted(void)
{
#ifdef ARCHOS_RECORDER
    return adc_read(ADC_EXT_POWER) > 0x200;
#else
    return (PADR & 1) == 0;
#endif
}

/* Returns battery level in percent */
int battery_level(void)
{
    int level;
    
    level = adc_read(ADC_UNREG_POWER);
    if(level < 0)
        level = 0;
    
    if(level > BATTERY_LEVEL_FULL)
        level = BATTERY_LEVEL_FULL;

    if(level < BATTERY_LEVEL_EMPTY)
        level = BATTERY_LEVEL_EMPTY;

    return ((level-BATTERY_LEVEL_EMPTY) * 100) / BATTERY_RANGE;
}

bool battery_level_safe(void)
{
    return adc_read(ADC_UNREG_POWER) > BATTERY_LEVEL_DANGEROUS;
}

void charger_enable(bool on)
{
#ifdef ARCHOS_RECORDER
    if(on)
        PBDR &= ~0x20;
    else
        PBDR |= 0x20;
#else
    on = on;
#endif
}

void ide_power_enable(bool on)
{
#ifdef ARCHOS_RECORDER
    if(on)
        PADR |= 0x20;
    else
        PADR &= ~0x20;
#else
    on = on;
#endif
}

void power_off(void)
{
#ifdef ARCHOS_RECORDER
    PBDR &= ~PBDR_BTN_OFF;
    PBIOR |= PBDR_BTN_OFF;
#else
    PADR &= ~0x800;
    PAIOR |= 0x800;
#endif
}

#else

bool charger_inserted(void)
{
    return false;
}

/* Returns battery level in percent */
int battery_level(void)
{
    return 100;
}

void charger_enable(bool on)
{
    on = on;
}

void power_off(void)
{
}

#endif /* SIMULATOR */
