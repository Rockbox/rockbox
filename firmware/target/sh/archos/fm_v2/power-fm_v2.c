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
#include "usb.h"

#if CONFIG_TUNER

bool tuner_power(bool status)
{
    (void)status;
    return true;
}

#endif /* #if CONFIG_TUNER */

void power_init(void)
{
    PBCR2 &= ~0x0c00;    /* GPIO for PB5 */
    or_b(0x20, &PBIORL);
    or_b(0x20, &PBDRL);  /* hold power */
}

bool charger_inserted(void)
{
    /* FM or V2 can also charge from the USB port */
    return (adc_read(ADC_CHARGE_REGULATOR) < 0x1FF);
}

/* Returns true if the unit is charging the batteries. */
bool charging_state(void) 
{
  /* We use the information from the ADC_EXT_POWER ADC channel, which
    tells us the charging current from the LTC1734. When DC is
    connected (either via the external adapter, or via USB), we try
    to determine if it is actively charging or only maintaining the
    charge. My tests show that ADC readings below about 0x80 means
    that the LTC1734 is only maintaining the charge. */
    return adc_read(ADC_EXT_POWER) >= 0x80;
}

void ide_power_enable(bool on)
{
    bool touched = false;

    if(on)
    {
        or_b(0x20, &PADRL);
        touched = true;
    }
#ifdef HAVE_ATA_POWER_OFF
    if(!on)
    {
        and_b(~0x20, &PADRL);
        touched = true;
    }
#endif /* HAVE_ATA_POWER_OFF */

/* late port preparation, else problems with read/modify/write 
   of other bits on same port, while input and floating high */
    if (touched)
    {
        or_b(0x20, &PAIORL); /* PA5 is an output */
        PACR2 &= 0xFBFF; /* GPIO for PA5 */
    }
}


bool ide_powered(void)
{
#ifdef HAVE_ATA_POWER_OFF
    if ((PACR2 & 0x0400) || !(PAIORL & 0x20)) /* not configured for output */
        return true; /* would be floating high, disk on */
    else
        return (PADRL & 0x20) != 0;
#else /* !defined(NEEDS_ATA_POWER_ON) && !defined(HAVE_ATA_POWER_OFF) */
    return true; /* pretend always powered if not controlable */
#endif
}

void power_off(void)
{
    disable_irq();
    and_b(~0x20, &PBDRL);
    or_b(0x20, &PBIORL);
    while(1);
}
