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

#ifdef HAVE_CHARGE_CTRL
bool charger_enabled;
#endif


#ifdef CONFIG_TUNER

static int fmstatus = 0;

void radio_set_status(int status)
{
    fmstatus = status;
#ifdef HAVE_TUNER_PWR_CTRL
    if (status)
    {
        and_b(~0x04, &PADRL); /* drive PA2 low for tuner enable */
        sleep(1); /* let the voltage settle */
    }
    else
        or_b(0x04, &PADRL); /* drive PA2 high for tuner disable */
#endif
}

int radio_get_status(void)
{
    return fmstatus;
}

#endif /* #ifdef CONFIG_TUNER */

#ifndef SIMULATOR

void power_init(void)
{
#if CONFIG_CPU == MCF5249
    or_l(0x00080000, &GPIO1_OUT);
    or_l(0x00080000, &GPIO1_ENABLE);
    or_l(0x00080000, &GPIO1_FUNCTION);

#ifndef BOOTLOADER
    /* The boot loader controls the power */
    ide_power_enable(true);
#endif
    or_l(0x80000000, &GPIO_ENABLE);
    or_l(0x80000000, &GPIO_FUNCTION);
#ifdef HAVE_SPDIF_POWER
    spdif_power_enable(false);
#endif
#elif CONFIG_CPU == PP5020
#warning Implement power_init()
#else
#ifdef HAVE_POWEROFF_ON_PB5
    PBCR2 &= ~0x0c00;    /* GPIO for PB5 */
    or_b(0x20, &PBIORL); 
    or_b(0x20, &PBDRL);  /* hold power */
#endif
#ifdef HAVE_CHARGE_CTRL
    PBCR2 &= ~0x0c00;    /* GPIO for PB5 */
    or_b(0x20, &PBIORL); /* Set charging control bit to output */
    charger_enable(false); /* Default to charger OFF */
#endif
#ifdef HAVE_TUNER_PWR_CTRL
    PACR2 &= ~0x0030;  /* GPIO for PA2 */
    or_b(0x04, &PADRL); /* drive PA2 high for tuner disable */
    or_b(0x04, &PAIORL); /* output for PA2 */
#endif
#endif
}

bool charger_inserted(void)
{     
#if CONFIG_CPU == MCF5249
    return (GPIO1_READ & 0x00400000)?true:false;
#elif defined(GMINI_ARCH)
    return (P7 & 0x80) == 0;
#else
#ifdef HAVE_CHARGING
#ifdef HAVE_CHARGE_CTRL
    /* Recorder */
    return adc_read(ADC_EXT_POWER) > 0x100;
#else
#ifdef HAVE_FMADC
    /* FM or V2, can also charge from the USB port */
    return (adc_read(ADC_CHARGE_REGULATOR) < 0x1FF) ||
        (adc_read(ADC_USB_POWER) < 0x1FF);
#else
    /* Player */
    return (PADR & 1) == 0;
#endif /* HAVE_FMADC */
#endif /* HAVE_CHARGE_CTRL */
#else
    /* Ondio */
    return false;
#endif /* HAVE_CHARGING */
#endif
}

#ifdef HAVE_CHARGE_CTRL
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

#ifdef HAVE_SPDIF_POWER
void spdif_power_enable(bool on)
{
    or_l(0x01000000, &GPIO1_FUNCTION);
    or_l(0x01000000, &GPIO1_ENABLE);

#ifdef SPDIF_POWER_INVERTED
    if(!on)
#else
    if(on)
#endif
        and_l(~0x01000000, &GPIO1_OUT);
    else
        or_l(0x01000000, &GPIO1_OUT);
}
#endif

#ifndef HAVE_MMC
void ide_power_enable(bool on)
{
    (void)on;

#if CONFIG_CPU == MCF5249
    if(on)
        and_l(~0x80000000, &GPIO_OUT);
    else
        or_l(0x80000000, &GPIO_OUT);
#elif CONFIG_CPU == PP5020
#warning Implement ide_power_enable()
#elif defined(GMINI_ARCH)
    if(on)
        P1 |= 0x08;
    else
        P1 &= ~0x08;
#else /* SH1 based archos */
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
#endif /* SH1 based archos */
}


bool ide_powered(void)
{
#if CONFIG_CPU == MCF5249
    return (GPIO_OUT & 0x80000000)?false:true;
#elif CONFIG_CPU == PP5020
#warning Implement ide_powered()
    return true;
#elif defined(GMINI_ARCH)
    return (P1 & 0x08?true:false);
#else /* SH1 based archos */
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
#endif
}
#endif /* !HAVE_MMC */


void power_off(void)
{
    set_irq_level(HIGHEST_IRQ_LEVEL);
#if CONFIG_CPU == MCF5249
    and_l(~0x00080000, &GPIO1_OUT);
#elif CONFIG_CPU == PP5020
#warning Implement power_off()
#elif defined(GMINI_ARCH)
    P1 &= ~1;
    P1CON &= ~1;
#else
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
