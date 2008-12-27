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
#include "usb.h"
#include "usb_core.h"
#include "power.h"
#include "power-imx31.h"
#include "backlight.h"
#include "backlight-target.h"
#include "avic-imx31.h"
#include "mc13783.h"
#if CONFIG_TUNER
#include "fmradio_i2c.h"
#endif

static unsigned int power_status = POWER_INPUT_NONE;

/* Detect which power sources are present. */
unsigned int power_input_status(void)
{
    unsigned int status = power_status;

    if (GPIO3_DR & (1 << 20))
        status |= POWER_INPUT_BATTERY;

    if (usb_allowed_current() < 500)
    {
        /* ACK that USB is connected but NOT chargeable */
        status &= ~(POWER_INPUT_USB_CHARGER & POWER_INPUT_CHARGER);
    }

    return status;
}

/* Detect changes in presence of the AC adaptor. */
void charger_main_detect_event(void)
{
    if (mc13783_read(MC13783_INTERRUPT_SENSE0) & MC13783_SE1S)
        power_status |= POWER_INPUT_MAIN_CHARGER;
    else
        power_status &= ~POWER_INPUT_MAIN_CHARGER;
}

/* Detect changes in USB bus power. Called from usb connect event handler. */
void charger_usb_detect_event(int status)
{
    /* USB plugged does not imply charging is possible or even
     * powering the device to maintain the battery. */
    if (status == USB_INSERTED)
        power_status |= POWER_INPUT_USB_CHARGER;
    else
        power_status &= ~POWER_INPUT_USB_CHARGER;
}

/* charging_state is implemented in powermgmt-imx31.c */

void ide_power_enable(bool on)
{
    if (!on)
    {
        /* Bus must be isolated before power off */
        imx31_regset32(&GPIO2_DR, (1 << 16));
    }

    /* HD power switch */
    imx31_regmod32(&GPIO3_DR, on ? (1 << 5) : 0, (1 << 5));

    if (on)
    {
        /* Bus switch may be turned on after powerup */
        sleep(HZ/10);
        imx31_regclr32(&GPIO2_DR, (1 << 16));
    }
}

bool ide_powered(void)
{
    return (GPIO3_DR & (1 << 5)) != 0;
}

#if CONFIG_TUNER
static bool tuner_on = false;

bool tuner_power(bool status)
{
    if (status != tuner_on)
    {
        tuner_on = status;
        /* Handle power and pin setup */
        fmradio_i2c_enable(status);
        status = !status;
    }

    return status;
}

bool tuner_powered(void)
{
    return tuner_on;
}
#endif /* #if CONFIG_TUNER */

void power_off(void)
{
    /* Cut backlight */
    _backlight_off();

    /* Let it fade */
    sleep(5*HZ/4);

    /* Set user off mode */
    mc13783_set(MC13783_POWER_CONTROL0, MC13783_USEROFFSPI);

    /* Wait for power cut */
    disable_interrupt(IRQ_FIQ_STATUS);

    while (1);
}

void power_init(void)
{
#if CONFIG_TUNER
    fmradio_i2c_init();
#endif

    /* Poll initial state */
    charger_main_detect_event();

    /* Enable detect event */
    mc13783_enable_event(MC13783_SE1_EVENT);
}
