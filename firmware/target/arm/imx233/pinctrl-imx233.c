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

void imx233_pinctrl_acquire_pin(unsigned bank, unsigned pin, const char *name)
{
    if(pin_use[bank][pin] != NULL && pin_use[bank][pin] != name)
        panicf("acquire B%dP%02d for %s, was %s!", bank, pin, name, pin_use[bank][pin]);
    pin_use[bank][pin] = name;
}

void imx233_pinctrl_acquire_pin_mask(unsigned bank, uint32_t mask, const char *name)
{
    for(unsigned pin = 0; pin < 32; pin++)
        if(mask & (1 << pin))
            imx233_pinctrl_acquire_pin(bank, pin, name);
}

void imx233_pinctrl_release_pin(unsigned bank, unsigned pin, const char *name)
{
    if(pin_use[bank][pin] != NULL && pin_use[bank][pin] != name)
        panicf("release B%dP%02d for %s: was %s!", bank, pin, name, pin_use[bank][pin]);
    pin_use[bank][pin] = NULL;
}

void imx233_pinctrl_release_pin_mask(unsigned bank, uint32_t mask, const char *name)
{
    for(unsigned pin = 0; pin < 32; pin++)
        if(mask & (1 << pin))
            imx233_pinctrl_release_pin(bank, pin, name);
}

const char *imx233_pinctrl_get_pin_use(unsigned bank, unsigned pin)
{
    return pin_use[bank][pin];
}
#endif

static pin_irq_cb_t pin_cb[3][32]; /* 3 banks, 32 pins/bank */

static void INT_GPIO(int bank)
{
    uint32_t fire = HW_PINCTRL_IRQSTAT(bank) & HW_PINCTRL_IRQEN(bank);
    for(int pin = 0; pin < 32; pin++)
        if(fire & (1 << pin))
        {
            pin_irq_cb_t cb = pin_cb[bank][pin];
            imx233_setup_pin_irq(bank, pin, false, false, false, NULL);
            if(cb)
                cb(bank, pin);
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

void imx233_setup_pin_irq(int bank, int pin, bool enable_int,
    bool level, bool polarity, pin_irq_cb_t cb)
{
    __REG_CLR(HW_PINCTRL_PIN2IRQ(bank)) = 1 << pin;
    __REG_CLR(HW_PINCTRL_IRQEN(bank)) = 1 << pin;
    __REG_CLR(HW_PINCTRL_IRQSTAT(bank))= 1 << pin;
    pin_cb[bank][pin] = cb;
    if(enable_int)
    {
        if(level)
            __REG_SET(HW_PINCTRL_IRQLEVEL(bank)) = 1 << pin;
        else
            __REG_CLR(HW_PINCTRL_IRQLEVEL(bank)) = 1 << pin;
        if(polarity)
            __REG_SET(HW_PINCTRL_IRQPOL(bank)) = 1 << pin;
        else
            __REG_CLR(HW_PINCTRL_IRQPOL(bank)) = 1 << pin;
        __REG_SET(HW_PINCTRL_PIN2IRQ(bank)) = 1 << pin;
        __REG_SET(HW_PINCTRL_IRQEN(bank)) = 1 << pin;
        imx233_icoll_enable_interrupt(INT_SRC_GPIO(bank), true);
    }
}
