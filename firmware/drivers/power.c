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
#include "kernel.h"
#include "system.h"
#include "power.h"

#ifdef HAVE_CHARGE_CTRL
bool charger_enabled;
#endif

#ifndef SIMULATOR

void power_init(void)
{
#ifdef HAVE_CHARGE_CTRL
    or_b(0x20, &PBIORL); /* Set charging control bit to output */
    charger_enable(false); /* Default to charger OFF */
#endif
#ifdef HAVE_ATA_POWER_OFF
    or_b(0x20, &PADRL); /* leave the disk on */
    or_b(0x20, &PAIORL);
    PACR2 &= 0xFBFF;
#endif
}

bool charger_inserted(void)
{
#ifdef HAVE_CHARGE_CTRL
    /* Recorder */
    return adc_read(ADC_EXT_POWER) > 0x100;
#else
#ifdef HAVE_FMADC
    /* FM */
    return adc_read(ADC_CHARGE_REGULATOR) < 0x1FF;
#else
    /* Player */
    return (PADR & 1) == 0;
#endif /* HAVE_FMADC */
#endif /* HAVE_CHARGE_CTRL */
}

void charger_enable(bool on)
{
#ifdef HAVE_CHARGE_CTRL
    if(on) 
    {
        and_b(~0x20, &PBDRL);
        charger_enabled = 1;
    } 
    else 
    {
        or_b(0x20, &PBDRL);
        charger_enabled = 0;
    }
#else
    on = on;
#endif
}

void ide_power_enable(bool on)
{
#ifdef HAVE_ATA_POWER_OFF
    if(on)
        or_b(0x20, &PADRL);
    else
        and_b(~0x20, &PADRL);
#else
    on = on;
#endif
}

void power_off(void)
{
    set_irq_level(15);
#ifdef HAVE_POWEROFF_ON_PBDR
    and_b(~0x10, &PBDRL);
    or_b(0x10, &PBIORL);
#elif defined(HAVE_POWEROFF_ON_PB5)
    and_b(~0x20, &PBDRL);
    or_b(0x20, &PBIORL);
#else
    and_b(~0x08, &PADRH);
    or_b(0x08, &PAIORH);
#endif
    while(1);
}

#else

bool charger_inserted(void)
{
    return false;
}

void charger_enable(bool on)
{
    on = on;
}

void power_off(void)
{
}

void ide_power_enable(bool on)
{
   on = on;
}

#endif /* SIMULATOR */
