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
#include "logf.h"
#include "pcf50605.h"
#include "pcf50606.h"
#include "usb.h"

#if CONFIG_CHARGING == CHARGING_CONTROL
bool charger_enabled;
#endif


#if CONFIG_TUNER

static bool powered = false;

bool tuner_powered(void)
{
    return powered;
}

bool tuner_power(bool status)
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

#ifndef SIMULATOR

void power_init(void)
{
#ifdef HAVE_POWEROFF_ON_PB5
    PBCR2 &= ~0x0c00;    /* GPIO for PB5 */
    or_b(0x20, &PBIORL); 
    or_b(0x20, &PBDRL);  /* hold power */
#endif
#if CONFIG_CHARGING == CHARGING_CONTROL
    PBCR2 &= ~0x0c00;    /* GPIO for PB5 */
    or_b(0x20, &PBIORL); /* Set charging control bit to output */
    charger_enable(false); /* Default to charger OFF */
#endif
#ifdef HAVE_TUNER_PWR_CTRL
    PACR2 &= ~0x0030;  /* GPIO for PA2 */
    or_b(0x04, &PADRL); /* drive PA2 high for tuner disable */
    or_b(0x04, &PAIORL); /* output for PA2 */
#endif
}


#if CONFIG_CHARGING
bool charger_inserted(void)
{     
#if CONFIG_CHARGING == CHARGING_CONTROL
    /* Recorder */
    return adc_read(ADC_EXT_POWER) > 0x100;
#elif defined (HAVE_FMADC)
    /* FM or V2, can also charge from the USB port */
    return (adc_read(ADC_CHARGE_REGULATOR) < 0x1FF);
#else
    /* Player */
    return (PADR & 1) == 0;
#endif
}
#endif /* CONFIG_CHARGING */

#if CONFIG_CHARGING == CHARGING_CONTROL
void charger_enable(bool on)
{
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
}
#endif

#if CONFIG_CHARGING == CHARGING_MONITOR
/* Returns true if the unit is charging the batteries. */
bool charging_state(void) {
#if CONFIG_BATTERY == BATT_LIION2200
  /* We use the information from the ADC_EXT_POWER ADC channel, which
    tells us the charging current from the LTC1734. When DC is
    connected (either via the external adapter, or via USB), we try
    to determine if it is actively charging or only maintaining the
    charge. My tests show that ADC readings below about 0x80 means
    that the LTC1734 is only maintaining the charge. */
    return adc_read(ADC_EXT_POWER) >= 0x80;
#endif
}
#endif

#ifndef HAVE_MMC
void ide_power_enable(bool on)
{
    (void)on;

    bool touched = false;
#ifdef NEEDS_ATA_POWER_ON
    if(on)
    {
#ifdef ATA_POWER_PLAYERSTYLE
        or_b(0x10, &PBDRL);
#else
        or_b(0x20, &PADRL);
#endif
        touched = true;
    }
#endif /* NEEDS_ATA_POWER_ON */
#ifdef HAVE_ATA_POWER_OFF
    if(!on)
    {
#ifdef ATA_POWER_PLAYERSTYLE
        and_b(~0x10, &PBDRL);
#else
        and_b(~0x20, &PADRL);
#endif
        touched = true;
    }
#endif /* HAVE_ATA_POWER_OFF */

/* late port preparation, else problems with read/modify/write 
   of other bits on same port, while input and floating high */
    if (touched)
    {
#ifdef ATA_POWER_PLAYERSTYLE
        or_b(0x10, &PBIORL); /* PB4 is an output */
        PBCR2 &= ~0x0300; /* GPIO for PB4 */
#else
        or_b(0x20, &PAIORL); /* PA5 is an output */
        PACR2 &= 0xFBFF; /* GPIO for PA5 */
#endif
    }
}


bool ide_powered(void)
{
#if defined(NEEDS_ATA_POWER_ON) || defined(HAVE_ATA_POWER_OFF)
#ifdef ATA_POWER_PLAYERSTYLE
    /* This is not correct for very old players, since these are unable to
     * control hd power. However, driving the pin doesn't hurt, because it
     * is not connected anywhere */
    if ((PBCR2 & 0x0300) || !(PBIORL & 0x10)) /* not configured for output */
        return false; /* would be floating low, disk off */
    else
        return (PBDRL & 0x10) != 0;
#else /* !ATA_POWER_PLAYERSTYLE */
    if ((PACR2 & 0x0400) || !(PAIORL & 0x20)) /* not configured for output */
        return true; /* would be floating high, disk on */
    else
        return (PADRL & 0x20) != 0;
#endif /* !ATA_POWER_PLAYERSTYLE */
#else /* !defined(NEEDS_ATA_POWER_ON) && !defined(HAVE_ATA_POWER_OFF) */
    return true; /* pretend always powered if not controlable */
#endif
}
#endif /* !HAVE_MMC */


void power_off(void)
{
    set_irq_level(HIGHEST_IRQ_LEVEL);
#ifdef HAVE_POWEROFF_ON_PBDR
    and_b(~0x10, &PBDRL);
    or_b(0x10, &PBIORL);
#elif defined(HAVE_POWEROFF_ON_PB5)
    and_b(~0x20, &PBDRL);
    or_b(0x20, &PBIORL);
#else
    /* Disable the backlight */
    and_b(~0x40, &PAIORH);

    and_b(~0x08, &PADRH);
    or_b(0x08, &PAIORH);
#endif
    while(1)
        yield();
}

#else

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

#ifdef HAVE_SPDIF_POWER
void spdif_power_enable(bool on)
{
   (void)on;
}
#endif

#endif /* SIMULATOR */
