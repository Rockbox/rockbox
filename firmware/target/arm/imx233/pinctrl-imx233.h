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

// set to debug pinctrl use
#define IMX233_PINCTRL_DEBUG

#define HW_PINCTRL_BASE         0x80018000

#define HW_PINCTRL_CTRL         (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0x0))
#define HW_PINCTRL_MUXSEL(i)    (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0x100 + (i) * 0x10))
#define HW_PINCTRL_DRIVE(i)     (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0x200 + (i) * 0x10))
#define HW_PINCTRL_PULL(i)      (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0x400 + (i) * 0x10))
#define HW_PINCTRL_DOUT(i)      (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0x500 + (i) * 0x10))
#define HW_PINCTRL_DIN(i)       (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0x600 + (i) * 0x10))
#define HW_PINCTRL_DOE(i)       (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0x700 + (i) * 0x10))
#define HW_PINCTRL_PIN2IRQ(i)   (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0x800 + (i) * 0x10))
#define HW_PINCTRL_IRQEN(i)     (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0x900 + (i) * 0x10))
#define HW_PINCTRL_IRQEN(i)     (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0x900 + (i) * 0x10))
#define HW_PINCTRL_IRQLEVEL(i)  (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0xa00 + (i) * 0x10))
#define HW_PINCTRL_IRQPOL(i)    (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0xb00 + (i) * 0x10))
#define HW_PINCTRL_IRQSTAT(i)   (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0xc00 + (i) * 0x10))

#define PINCTRL_FUNCTION_MAIN   0
#define PINCTRL_FUNCTION_ALT1   1
#define PINCTRL_FUNCTION_ALT2   2
#define PINCTRL_FUNCTION_GPIO   3

#define PINCTRL_DRIVE_4mA       0
#define PINCTRL_DRIVE_8mA       1
#define PINCTRL_DRIVE_12mA      2
#define PINCTRL_DRIVE_16mA      3 /* not available on all pins */

#ifdef IMX233_PINCTRL_DEBUG
void imx233_pinctrl_acquire_pin(unsigned bank, unsigned pin, const char *name);
void imx233_pinctrl_acquire_pin_mask(unsigned bank, uint32_t mask, const char *name);
void imx233_pinctrl_release_pin(unsigned bank, unsigned pin, const char *name);
void imx233_pinctrl_release_pin_mask(unsigned bank, uint32_t mask, const char *name);
const char *imx233_pinctrl_get_pin_use(unsigned bank, unsigned pin);
#else
#define imx233_pinctrl_acquire_pin(...)
#define imx233_pinctrl_acquire_pin_mask(...)
#define imx233_pinctrl_release_pin(...)
#define imx233_pinctrl_release_pin_mask(...)
#define imx233_pinctrl_get_pin_use(...) NULL
#endif

typedef void (*pin_irq_cb_t)(int bank, int pin);

static inline void imx233_pinctrl_init(void)
{
    __REG_CLR(HW_PINCTRL_CTRL) = __BLOCK_CLKGATE | __BLOCK_SFTRST;
}

static inline void imx233_set_pin_drive_strength(unsigned bank, unsigned pin, unsigned strength)
{
    __REG_CLR(HW_PINCTRL_DRIVE(4 * bank + pin / 8)) = 3 << (4 * (pin % 8));
    __REG_SET(HW_PINCTRL_DRIVE(4 * bank + pin / 8)) = strength << (4 * (pin % 8));
}

static inline void imx233_enable_gpio_output(unsigned bank, unsigned pin, bool enable)
{
    if(enable)
        __REG_SET(HW_PINCTRL_DOE(bank)) = 1 << pin;
    else
        __REG_CLR(HW_PINCTRL_DOE(bank)) = 1 << pin;
}

static inline void imx233_enable_gpio_output_mask(unsigned bank, uint32_t pin_mask, bool enable)
{
    if(enable)
        __REG_SET(HW_PINCTRL_DOE(bank)) = pin_mask;
    else
        __REG_CLR(HW_PINCTRL_DOE(bank)) = pin_mask;
}

static inline void imx233_set_gpio_output(unsigned bank, unsigned pin, bool value)
{
    if(value)
        __REG_SET(HW_PINCTRL_DOUT(bank)) = 1 << pin;
    else
        __REG_CLR(HW_PINCTRL_DOUT(bank)) = 1 << pin;
}

static inline void imx233_set_gpio_output_mask(unsigned bank, uint32_t pin_mask, bool value)
{
    if(value)
        __REG_SET(HW_PINCTRL_DOUT(bank)) = pin_mask;
    else
        __REG_CLR(HW_PINCTRL_DOUT(bank)) = pin_mask;
}

static inline uint32_t imx233_get_gpio_input_mask(unsigned bank, uint32_t pin_mask)
{
    return HW_PINCTRL_DIN(bank) & pin_mask;
}

static inline void imx233_set_pin_function(unsigned bank, unsigned pin, unsigned function)
{
    __REG_CLR(HW_PINCTRL_MUXSEL(2 * bank + pin / 16)) = 3 << (2 * (pin % 16));
    __REG_SET(HW_PINCTRL_MUXSEL(2 * bank + pin / 16)) = function << (2 * (pin % 16));
}

static inline void imx233_enable_pin_pullup(unsigned bank, unsigned pin, bool enable)
{
    if(enable)
        __REG_SET(HW_PINCTRL_PULL(bank)) = 1 << pin;
    else
        __REG_CLR(HW_PINCTRL_PULL(bank)) = 1 << pin;
}

static inline void imx233_enable_pin_pullup_mask(unsigned bank, uint32_t pin_msk, bool enable)
{
    if(enable)
        __REG_SET(HW_PINCTRL_PULL(bank)) = pin_msk;
    else
        __REG_CLR(HW_PINCTRL_PULL(bank)) = pin_msk;
}

/** On irq, the pin irq interrupt is disable and then cb is called;
 * the setup_pin_irq function needs to be called again to enable it again */
void imx233_setup_pin_irq(int bank, int pin, bool enable_int,
    bool level, bool polarity, pin_irq_cb_t cb);

#endif /* __PINCTRL_IMX233_H__ */
