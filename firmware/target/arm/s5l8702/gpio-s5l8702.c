/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 Cástor Muñoz
 * Code based on openiBoot project
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
#include <stdint.h>

#include "config.h"
#include "system.h"
#include "gpio-s5l8702.h"
#include "panic.h"


/*
 * XXX: disabled, not used and never tested!
 */
#if 0
/* handlers list */
#define GPIOIC_MAX_HANDLERS  2

#define FLAG_TYPE_LEVEL  (1 << 0)
#define FLAG_AUTOFLIP    (1 << 1)

struct gpio_handler {
    int gpio_n;
    void (*isr)(void);
    int flags;
};
static struct gpio_handler l_handlers[GPIOIC_MAX_HANDLERS] IDATA_ATTR;
static int n_handlers = 0;


/* API */
void INIT_ATTR gpio_init(void)
{
    /* TODO: initialize GPIO ports for minimum power consumption */
}

uint32_t gpio_group_get(int group)
{
    uint32_t pcon = PCON(group);
    uint32_t pdat = PDAT(group);
    uint32_t group_cfg = 0;
    int i;

    for (i = 0; i < 8; i++) {
        int pin_cfg = pcon & 0xF;
        if (pin_cfg == 1) /* output */
            pin_cfg = 0xE | ((pdat >> i) & 1);
        group_cfg = (group_cfg >> 4) | (pin_cfg << 28);
        pcon >>= 4;
    }

    return group_cfg;
}

void gpio_group_set(int group, uint32_t mask, uint32_t cfg)
{
    PCON(group) = (PCON(group) & ~mask) | (cfg & mask);
}

void gpio_int_register(int gpio_n, void *isr, int type, int level, int autoflip)
{
    if (n_handlers >= GPIOIC_MAX_HANDLERS)
        panicf("gpio_int_register(): too many handlers!");

    int group = IC_GROUP(gpio_n);
    int index = IC_IDX(gpio_n);

    int flags = disable_irq_save();

    /* fill handler struct */
    struct gpio_handler *handler = &l_handlers[n_handlers++];

    handler->gpio_n = gpio_n;
    handler->isr = isr;
    handler->flags = (type ? FLAG_TYPE_LEVEL : 0)
                   | (autoflip ? FLAG_AUTOFLIP : 0);

    /* configure */
    GPIOIC_INTTYPE(group) |=
            (type ? GPIOIC_INTTYPE_LEVEL : GPIOIC_INTTYPE_EDGE) << index;
    GPIOIC_INTLEVEL(group) |=
            (level ? GPIOIC_INTLEVEL_HIGH : GPIOIC_INTLEVEL_LOW) << index;

    restore_irq(flags);

    /* XXX: valid only for gpio_n = 0..127 (IRQ_EXT0..IRQ_EXT3) */
    VIC0INTENABLE = 1 << (IRQ_EXT0 + (gpio_n >> 5));
}

void gpio_int_enable(int gpio_n)
{
    int group = IC_GROUP(gpio_n);
    uint32_t bit_mask = 1 << IC_IDX(gpio_n);

    int flags = disable_irq_save();
    GPIOIC_INTSTAT(group) = bit_mask; /* clear */
    GPIOIC_INTEN(group) |= bit_mask; /* enable */
    restore_irq(flags);
}

void gpio_int_disable(int gpio_n)
{
    int group = IC_GROUP(gpio_n);
    uint32_t bit_mask = 1 << IC_IDX(gpio_n);

    int flags = disable_irq_save();
    GPIOIC_INTEN(group) &= ~bit_mask; /* disable */
    GPIOIC_INTSTAT(group) = bit_mask; /* clear */
    restore_irq(flags);
}


/* ISR */
void ICODE_ATTR gpio_handler(int group)
{
    int i;
    struct gpio_handler *handler;
    uint32_t ints, bit_mask;

    ints = GPIOIC_INTSTAT(group) & GPIOIC_INTEN(group);

    for (i = 0; i < n_handlers; i++) {
        handler = &l_handlers[i];

        if (IC_GROUP(handler->gpio_n) != group)
            continue;

        bit_mask = 1 << IC_IDX(handler->gpio_n);

        if (ints & bit_mask) {
            if ((handler->flags & FLAG_TYPE_LEVEL) == 0)
                GPIOIC_INTSTAT(group) = bit_mask; /* clear */

            if (handler->flags & FLAG_AUTOFLIP)
                GPIOIC_INTLEVEL(group) ^= bit_mask; /* swap level */

            if (handler->isr)
                handler->isr(); /* exec GPIO handler */

            GPIOIC_INTSTAT(group) = bit_mask; /* clear */
        }
    }
}

void ICODE_ATTR INT_EXT0(void)
{
    gpio_handler(6);
}

void ICODE_ATTR INT_EXT1(void)
{
    gpio_handler(5);
}

void ICODE_ATTR INT_EXT2(void)
{
    gpio_handler(4);
}

void ICODE_ATTR INT_EXT3(void)
{
    gpio_handler(3);
}

void ICODE_ATTR INT_EXT6(void)
{
    gpio_handler(0);
}
#endif
