/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#include "system.h"
#include "system-target.h"
#include "cpu.h"
#include "string.h"
#include "pinctrl-imx233.h"

#ifdef IMX233_PINCTRL_DEBUG
// 4 banks of 32 pins
static const char *pin_use[4][32];

void imx233_pinctrl_acquire(unsigned bank, unsigned pin, const char *name)
{
    if(pin_use[bank][pin] != NULL && pin_use[bank][pin] != name)
        panicf("acquire B%dP%02d for %s, was %s!", bank, pin, name, pin_use[bank][pin]);
    pin_use[bank][pin] = name;
}

void imx233_pinctrl_release(unsigned bank, unsigned pin, const char *name)
{
    if(pin_use[bank][pin] != NULL && pin_use[bank][pin] != name)
        panicf("release B%dP%02d for %s: was %s!", bank, pin, name, pin_use[bank][pin]);
    pin_use[bank][pin] = NULL;
}

const char *imx233_pinctrl_blame(unsigned bank, unsigned pin)
{
    return pin_use[bank][pin];
}
#endif

static pin_irq_cb_t pin_cb[3][32]; /* 3 banks, 32 pins/bank */
static intptr_t pin_cb_user[3][32];

static void INT_GPIO(int bank)
{
    uint32_t fire = HW_PINCTRL_IRQSTATn(bank) & HW_PINCTRL_IRQENn(bank);
    for(int pin = 0; pin < 32; pin++)
        if(fire & (1 << pin))
        {
            pin_irq_cb_t cb = pin_cb[bank][pin];
            intptr_t arg = pin_cb_user[bank][pin];
            /* WARNING: this call will modify pin_cb and pin_cb_user, that's
             * why we copy the data before ! */
            imx233_pinctrl_setup_irq(bank, pin, false, false, false, NULL, 0);
            if(cb)
                cb(bank, pin, arg);
        }
}

void INT_GPIO0(void)
{
    INT_GPIO(0);
}

void INT_GPIO1(void)
{
    INT_GPIO(1);
}

void INT_GPIO2(void)
{
    INT_GPIO(2);
}

void imx233_pinctrl_setup_irq(unsigned bank, unsigned pin, bool enable_int,
    bool level, bool polarity, pin_irq_cb_t cb, intptr_t user)
{
    HW_PINCTRL_PIN2IRQn_CLR(bank) = 1 << pin;
    HW_PINCTRL_IRQENn_CLR(bank) = 1 << pin;
    HW_PINCTRL_IRQSTATn_CLR(bank) = 1 << pin;
    pin_cb[bank][pin] = cb;
    pin_cb_user[bank][pin] = user;
    if(enable_int)
    {
        if(level)
            HW_PINCTRL_IRQLEVELn_SET(bank) = 1 << pin;
        else
            HW_PINCTRL_IRQLEVELn_CLR(bank) = 1 << pin;
        if(polarity)
            HW_PINCTRL_IRQPOLn_SET(bank) = 1 << pin;
        else
            HW_PINCTRL_IRQPOLn_CLR(bank) = 1 << pin;
        HW_PINCTRL_PIN2IRQn_SET(bank) = 1 << pin;
        HW_PINCTRL_IRQENn_SET(bank) = 1 << pin;
        imx233_icoll_enable_interrupt(INT_SRC_GPIO(bank), true);
    }
}
