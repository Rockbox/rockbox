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
#include "i2c-imx31.h"

extern struct i2c_node si4700_i2c_node;
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
bool tuner_power(bool status)
{
    if (status)
    {
        /* the si4700 is the only thing connected to i2c2 so
           we can diable the i2c module when not in use */
        i2c_enable_node(&si4700_i2c_node, true);
        /* enable the fm chip */
        imx31_regset32(&GPIO1_DR, (1 << 26));
        /* enable CLK32KMCU clock */
        mc13783_set(MC13783_POWER_CONTROL0, MC13783_CLK32KMCUEN);
    }
    else
    {
        /* the si4700 is the only thing connected to i2c2 so
           we can diable the i2c module when not in use */
        i2c_enable_node(&si4700_i2c_node, false);
        /* disable the fm chip */
        imx31_regclr32(&GPIO1_DR, (1 << 26));
        /* disable CLK32KMCU clock */
        mc13783_clear(MC13783_POWER_CONTROL0, MC13783_CLK32KMCUEN);
    }
    return true;
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
    /* Poll initial state */
    charger_main_detect_event();

    /* Enable detect event */
    mc13783_enable_event(MC13783_SE1_EVENT);
}
