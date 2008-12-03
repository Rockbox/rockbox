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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "system.h"
#include "power.h"
#include "power-imx31.h"
#include "backlight.h"
#include "backlight-target.h"
#include "avic-imx31.h"
#include "mc13783.h"

static bool charger_detect = false;

/* This is called from the mc13783 interrupt thread */
void charger_detect_event(void)
{
    charger_detect =
        mc13783_read(MC13783_INTERRUPT_SENSE0) & MC13783_CHGDETS;
}

unsigned int power_input_status(void)
{
    unsigned int status = POWER_INPUT_NONE;

    if ((GPIO3_DR & (1 << 20)) != 0)
        status |= POWER_INPUT_BATTERY;

    if (charger_detect)
        status |= POWER_INPUT_MAIN_CHARGER;

    return status;
}

/* Returns true if the unit is charging the batteries. */
bool charging_state(void)
{
    return false;
}

void ide_power_enable(bool on)
{
    if (!on)
    {
        /* Bus must be isolated before power off */
        imx31_regmod32(&GPIO2_DR, (1 << 16), (1 << 16));
    }

    /* HD power switch */
    imx31_regmod32(&GPIO3_DR, on ? (1 << 5) : 0, (1 << 5));

    if (on)
    {
        /* Bus switch may be turned on after powerup */
        sleep(HZ/10);
        imx31_regmod32(&GPIO2_DR, 0, (1 << 16));
    }
}

bool ide_powered(void)
{
    return (GPIO3_DR & (1 << 5)) != 0;
}

void power_off(void)
{
    mc13783_set(MC13783_POWER_CONTROL0, MC13783_USEROFFSPI);

    disable_interrupt(IRQ_FIQ_STATUS);
    while (1);
}

void power_init(void)
{
    /* Poll initial state */
    charger_detect_event();

    /* Enable detect event */
    mc13783_enable_event(MC13783_CHGDET_EVENT);
}

