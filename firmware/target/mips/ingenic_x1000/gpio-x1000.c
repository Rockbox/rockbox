/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include "gpio-x1000.h"

const struct gpio_setting gpio_settings[PIN_COUNT] = {
#define DEFINE_GPIO(_name, _gpio, _func) \
    {.gpio = _gpio, .func = _func},
#define DEFINE_PINGROUP(...)
#include "gpio-target.h"
#undef DEFINE_GPIO
#undef DEFINE_PINGROUP
};

const struct pingroup_setting pingroup_settings[PINGROUP_COUNT] = {
#define DEFINE_GPIO(...)
#define DEFINE_PINGROUP(_name, _port, _pins, _func) \
    {.port = _port, .pins = _pins, .func = _func},
#include "gpio-target.h"
#undef DEFINE_GPIO
#undef DEFINE_PINGROUP
};

const char* const gpio_names[PIN_COUNT] = {
#define DEFINE_GPIO(_name, ...) #_name,
#define DEFINE_PINGROUP(...)
#include "gpio-target.h"
#undef DEFINE_GPIO
#undef DEFINE_PINGROUP
};

const char* const pingroup_names[PINGROUP_COUNT] = {
#define DEFINE_GPIO(...)
#define DEFINE_PINGROUP(_name, ...) #_name,
#include "gpio-target.h"
#undef DEFINE_GPIO
#undef DEFINE_PINGROUP
};

void gpio_init(void)
{
    /* Apply all initial GPIO settings */
    for(int i = 0; i < PINGROUP_COUNT; ++i) {
        const struct pingroup_setting* d = &pingroup_settings[i];
        if(d->pins != 0)
            gpioz_configure(d->port, d->pins, d->func);
    }

    for(int i = 0; i < PIN_COUNT; ++i) {
        const struct gpio_setting* d = &gpio_settings[i];
        if(d->gpio != GPIO_NONE)
            gpioz_configure(GPION_PORT(d->gpio), GPION_MASK(d->gpio), d->func);
    }

    /* Any GPIO pins left in an IRQ trigger state need to be switched off,
     * because the drivers won't be ready to handle the interrupts until they
     * get initialized later in the boot. */
    for(int i = 0; i < 4; ++i) {
        uint32_t intbits = REG_GPIO_INT(i);
        if(intbits) {
            gpioz_configure(i, intbits, GPIOF_INPUT);
            jz_clr(GPIO_FLAG(i), intbits);
        }
    }
}

void gpioz_configure(int port, uint32_t pins, int func)
{
    uint32_t intr = REG_GPIO_INT(port);
    uint32_t mask = REG_GPIO_MSK(port);
    uint32_t pat1 = REG_GPIO_PAT1(port);
    uint32_t pat0 = REG_GPIO_PAT0(port);

    /* Note: GPIO Z has _only_ set and clear registers, which are used to
     * atomically manipulate the selected GPIO port when we write GID2LD.
     * So there's not really any direct setting or clearing going on here...
     */
    if(func & GPIO_F_INT)  jz_set(GPIO_INT(GPIO_Z), (intr & pins) ^ pins);
    else                   jz_clr(GPIO_INT(GPIO_Z), (~intr & pins) ^ pins);
    if(func & GPIO_F_MASK) jz_set(GPIO_MSK(GPIO_Z), (mask & pins) ^ pins);
    else                   jz_clr(GPIO_MSK(GPIO_Z), (~mask & pins) ^ pins);
    if(func & GPIO_F_PAT1) jz_set(GPIO_PAT1(GPIO_Z), (pat1 & pins) ^ pins);
    else                   jz_clr(GPIO_PAT1(GPIO_Z), (~pat1 & pins) ^ pins);
    if(func & GPIO_F_PAT0) jz_set(GPIO_PAT0(GPIO_Z), (pat0 & pins) ^ pins);
    else                   jz_clr(GPIO_PAT0(GPIO_Z), (~pat0 & pins) ^ pins);
    REG_GPIO_Z_GID2LD = port;

    if(func & GPIO_F_PULL)
        jz_set(GPIO_PULL(port), pins);
    else
        jz_clr(GPIO_PULL(port), pins);
}
