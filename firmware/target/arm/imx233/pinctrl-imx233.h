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
#include "regs/pinctrl.h"

// set to debug pinctrl use
#define IMX233_PINCTRL_DEBUG

#define PINCTRL_FUNCTION_MAIN   0
#define PINCTRL_FUNCTION_ALT1   1
#define PINCTRL_FUNCTION_ALT2   2
#define PINCTRL_FUNCTION_GPIO   3

#if IMX233_SUBTARGET >= 3700
#define PINCTRL_DRIVE_4mA       0
#define PINCTRL_DRIVE_8mA       1
#define PINCTRL_DRIVE_12mA      2
#define PINCTRL_DRIVE_16mA      3 /* not available on all pins */
#else
#define PINCTRL_DRIVE_4mA       0
#define PINCTRL_DRIVE_8mA       1
#endif

#ifdef IMX233_PINCTRL_DEBUG
void imx233_pinctrl_acquire(unsigned bank, unsigned pin, const char *name);
void imx233_pinctrl_release(unsigned bank, unsigned pin, const char *name);
const char *imx233_pinctrl_blame(unsigned bank, unsigned pin);
#else
#define imx233_pinctrl_acquire(...)
#define imx233_pinctrl_release(...)
#define imx233_pinctrl_get_pin_use(...) NULL
#endif

typedef void (*pin_irq_cb_t)(int bank, int pin, intptr_t user);

static inline void imx233_pinctrl_init(void)
{
    BF_CLR(PINCTRL_CTRL, CLKGATE, SFTRST);
}

#if IMX233_SUBTARGET >= 3700
static inline void imx233_pinctrl_set_drive(unsigned bank, unsigned pin, unsigned strength)
{
    HW_PINCTRL_DRIVEn_CLR(4 * bank + pin / 8) = 3 << (4 * (pin % 8));
    HW_PINCTRL_DRIVEn_SET(4 * bank + pin / 8) = strength << (4 * (pin % 8));
}
#else
static inline void imx233_pinctrl_set_drive(unsigned bank, unsigned pin, unsigned strength)
{
   HW_PINCTRL_DRIVEn_CLR(bank) = 1 << pin;
   HW_PINCTRL_DRIVEn_SET(bank) = strength << pin;
}
#endif

static inline void imx233_pinctrl_enable_gpio(unsigned bank, unsigned pin, bool enable)
{
    if(enable)
        HW_PINCTRL_DOEn_SET(bank) = 1 << pin;
    else
        HW_PINCTRL_DOEn_CLR(bank) = 1 << pin;
}

static inline void imx233_pinctrl_set_gpio(unsigned bank, unsigned pin, bool value)
{
    if(value)
        HW_PINCTRL_DOUTn_SET(bank) = 1 << pin;
    else
        HW_PINCTRL_DOUTn_CLR(bank) = 1 << pin;
}

static inline bool imx233_pinctrl_get_gpio(unsigned bank, unsigned pin)
{
    return (HW_PINCTRL_DINn(bank) >> pin) & 1;
}

static inline void imx233_pinctrl_set_function(unsigned bank, unsigned pin, unsigned function)
{
#if IMX233_SUBTARGET >= 3700
    HW_PINCTRL_MUXSELn_CLR(2 * bank + pin / 16) = 3 << (2 * (pin % 16));
    HW_PINCTRL_MUXSELn_SET(2 * bank + pin / 16) = function << (2 * (pin % 16));
#else
    if(pin < 16)
    {
        HW_PINCTRL_MUXSELLn_CLR(bank) = 3 << (2 * pin);
        HW_PINCTRL_MUXSELLn_SET(bank) = function << (2 * pin);
    }
    else
    {
        pin -= 16;
        HW_PINCTRL_MUXSELHn_CLR(bank) = 3 << (2 * pin);
        HW_PINCTRL_MUXSELHn_SET(bank) = function << (2 * pin);
    }
#endif
}

#if IMX233_SUBTARGET >= 3700
static inline void imx233_pinctrl_enable_pullup(unsigned bank, unsigned pin, bool enable)
{
    if(enable)
        HW_PINCTRL_PULLn_SET(bank) = 1 << pin;
    else
        HW_PINCTRL_PULLn_CLR(bank) = 1 << pin;
}
#else
static inline void imx233_pinctrl_enable_pullup(unsigned bank, unsigned pin, bool enable)
{
    (void) bank;
    (void) pin;
    (void) enable;
}
#endif

/** On irq, the pin irq interrupt is disable and then cb is called;
 * the setup_pin_irq function needs to be called again to enable it again */
void imx233_pinctrl_setup_irq(unsigned bank, unsigned pin, bool enable_int,
    bool level, bool polarity, pin_irq_cb_t cb, intptr_t user);

/**
 * Virtual pin interface
 *
 * This interface provides an easy way to configure standard pins for
 * devices like SSP, LCD, etc
 * The point here is that these pins can or cannot exist depending on the
 * chip and the package and the drivers don't want to mess with that.
 *
 * A virtual pin is described by a bank, a pin and the function.
 */
typedef unsigned vpin_t;
#define VPIN_PACK(bank, pin, mux)  \
    ((vpin_t)((bank) << 5 | (pin) | PINCTRL_FUNCTION_##mux << 7))
#define VPIN_UNPACK_BANK(vpin) (((vpin) >> 5) & 3)
#define VPIN_UNPACK_PIN(vpin)  (((vpin) >> 0) & 0x1f)
#define VPIN_UNPACK_MUX(vpin)  (((vpin) >> 7) & 3)

static inline void imx233_pinctrl_setup_vpin(vpin_t vpin, const char *name,
    unsigned drive, bool pullup)
{
    unsigned bank = VPIN_UNPACK_BANK(vpin), pin = VPIN_UNPACK_PIN(vpin);
    imx233_pinctrl_acquire(bank, pin, name);
    imx233_pinctrl_set_function(bank, pin, VPIN_UNPACK_MUX(vpin));
    imx233_pinctrl_set_drive(bank, pin, drive);
    imx233_pinctrl_enable_pullup(bank, pin, pullup);
}

#if IMX233_SUBTARGET < 3700
#include "pins/pins-stmp3600.h"
#elif IMX233_SUBTARGET < 3770
#include "pins/pins-stmp3700.h"
#elif IMX233_SUBTARGET < 3780
#error implement this
#else
#include "pins/pins-imx233.h"
#endif

#endif /* __PINCTRL_IMX233_H__ */
