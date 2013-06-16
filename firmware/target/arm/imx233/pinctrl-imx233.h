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


#ifndef __PINCTRL_IMX233_H__
#define __PINCTRL_IMX233_H__

#include "config.h"
#include "system.h"
#include "regs/regs-pinctrl.h"

// set to debug pinctrl use
#define IMX233_PINCTRL_DEBUG

#define PINCTRL_FUNCTION_MAIN   0
#define PINCTRL_FUNCTION_ALT1   1
#define PINCTRL_FUNCTION_ALT2   2
#define PINCTRL_FUNCTION_GPIO   3

#define PINCTRL_DRIVE_4mA       0
#define PINCTRL_DRIVE_8mA       1
#define PINCTRL_DRIVE_12mA      2
#define PINCTRL_DRIVE_16mA      3 /* not available on all pins */

#ifdef IMX233_PINCTRL_DEBUG
void imx233_pinctrl_acquire(unsigned bank, unsigned pin, const char *name);
void imx233_pinctrl_acquire_mask(unsigned bank, uint32_t mask, const char *name);
void imx233_pinctrl_release(unsigned bank, unsigned pin, const char *name);
void imx233_pinctrl_release_mask(unsigned bank, uint32_t mask, const char *name);
const char *imx233_pinctrl_blame(unsigned bank, unsigned pin);
#else
#define imx233_pinctrl_acquire(...)
#define imx233_pinctrl_acquire_mask(...)
#define imx233_pinctrl_release(...)
#define imx233_pinctrl_release_mask(...)
#define imx233_pinctrl_blame(...) NULL
#endif

typedef void (*pin_irq_cb_t)(int bank, int pin);

static inline void imx233_pinctrl_init(void)
{
    HW_PINCTRL_CTRL_CLR = BM_OR2(PINCTRL_CTRL, CLKGATE, SFTRST);
}

static inline void imx233_pinctrl_set_drive(unsigned bank, unsigned pin, unsigned strength)
{
    HW_PINCTRL_DRIVEn_CLR(4 * bank + pin / 8) = 3 << (4 * (pin % 8));
    HW_PINCTRL_DRIVEn_SET(4 * bank + pin / 8) = strength << (4 * (pin % 8));
}

static inline void imx233_pinctrl_enable_gpio(unsigned bank, unsigned pin, bool enable)
{
   if(enable)
        HW_PINCTRL_DOEn_SET(bank) = 1 << pin;
    else
        HW_PINCTRL_DOEn_CLR(bank) = 1 << pin;
}

static inline void imx233_pinctrl_enable_gpio_mask(unsigned bank, uint32_t pin_mask, bool enable)
{
    if(enable)
        HW_PINCTRL_DOEn_SET(bank) = pin_mask;
    else
        HW_PINCTRL_DOEn_CLR(bank) = pin_mask;
}

static inline void imx233_pinctrl_set_gpio(unsigned bank, unsigned pin, bool value)
{
    if(value)
        HW_PINCTRL_DOUTn_SET(bank) = 1 << pin;
    else
        HW_PINCTRL_DOUTn_CLR(bank) = 1 << pin;
}

static inline void imx233_pinctrl_set_gpio_mask(unsigned bank, uint32_t pin_mask, bool value)
{
    if(value)
        HW_PINCTRL_DOUTn_SET(bank) = pin_mask;
    else
        HW_PINCTRL_DOUTn_CLR(bank) = pin_mask;
}

static inline uint32_t imx233_pinctrl_get_gpio_mask(unsigned bank, uint32_t pin_mask)
{
    return HW_PINCTRL_DINn(bank) & pin_mask;
}

static inline void imx233_pinctrl_set_function(unsigned bank, unsigned pin, unsigned function)
{
    HW_PINCTRL_MUXSELn_CLR(2 * bank + pin / 16) = 3 << (2 * (pin % 16));
    HW_PINCTRL_MUXSELn_SET(2 * bank + pin / 16) = function << (2 * (pin % 16));
}

static inline void imx233_pinctrl_enable_pullup(unsigned bank, unsigned pin, bool enable)
{
    if(enable)
        HW_PINCTRL_PULLn_SET(bank) = 1 << pin;
    else
        HW_PINCTRL_PULLn_CLR(bank) = 1 << pin;
}

static inline void imx233_pinctrl_enable_pullup_mask(unsigned bank, uint32_t pin_msk, bool enable)
{
    if(enable)
        HW_PINCTRL_PULLn_SET(bank) = pin_msk;
    else
        HW_PINCTRL_PULLn_CLR(bank) = pin_msk;
}

/** On irq, the pin irq interrupt is disable and then cb is called;
 * the setup_pin_irq function needs to be called again to enable it again */
void imx233_pinctrl_setup_irq(int bank, int pin, bool enable_int,
    bool level, bool polarity, pin_irq_cb_t cb);

#endif /* __PINCTRL_IMX233_H__ */
