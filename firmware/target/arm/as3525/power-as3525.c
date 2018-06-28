/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2008 Rafaël Carré
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdbool.h>
#include "config.h"
#include "ascodec.h"
#include "as3514.h"
#include "power.h"

void power_off(void)
{
#ifdef HAVE_RTC_ALARM
    /* as3543 RTC wake-up needs a specific power down */

    extern void rtc_alarm_poweroff(void); /* in drivers/rtc/rtc_as3514.c */

    rtc_alarm_poweroff(); /* will return if wake-up isn't enabled */
#endif /* HAVE_RTC_ALARM */

    /* clear bit 0 of system register */
    ascodec_write(AS3514_SYSTEM, ascodec_read(AS3514_SYSTEM) & ~1);

    /* TODO : turn off peripherals properly ? */

    while(1); /* wait for system to shut down */
}

void power_init(void)
{
}

#if CONFIG_CHARGING
unsigned int power_input_status(void)
{
    return ascodec_chg_status() ?
        POWER_INPUT_MAIN_CHARGER : POWER_INPUT_NONE;

    /* TODO: Handle USB and other sources properly */
}
#endif /* CONFIG_CHARGING */

void ide_power_enable(bool on)
{
    (void)on;
}

#if CONFIG_TUNER
bool tuner_power(bool status)
{
    (void) status;
    return true;
}

bool tuner_powered(void)
{
    return true;
}
#endif

#ifdef HAVE_LINEOUT_POWEROFF
void lineout_set(bool enable)
{
    /* Call audio hardware driver implementation */
    audiohw_enable_lineout(enable);
}
#endif
