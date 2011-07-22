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
#include "pinctrl-imx233.h"

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
        imx233_enable_interrupt(INT_SRC_GPIO(bank), true);
    }
}
