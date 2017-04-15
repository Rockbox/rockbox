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
#include "panic.h"

#include "gpio-s5l8702.h"


int rec_hw_ver;

static uint32_t gpio_data[] =
{
    0x5322222F, 0xEEEEEE00, 0x2332EEEE, 0x3333E222,
    0x33333333, 0x33333333, 0x3F000E33, 0xEEEEEEEE,
    0xEEEEEEEE, 0xEEEEEEEE, 0xE0EEEEEE, 0xEE00EE0E,
    0xEEEE0EEE, 0xEEEEEEEE, 0xEE2222EE, 0xEEEE0EEE
};

void INIT_ATTR gpio_preinit(void)
{
    for (int i = 0; i < 16; i++) {
        PCON(i) = gpio_data[i];
        PUNB(i) = 0;
        PUNC(i) = 0;
    }
}

void INIT_ATTR gpio_init(void)
{
    /* Capture hardware versions:
     *
     * HW version 1 includes an amplifier for the jack plug
     * microphone, it is activated configuring GPIO E7 as output
     * high. It is posible to detect capture HW version (even
     * when HP are not plugged) reading GPIO E7:
     *
     *   Ver  GPIO E7  models       capture support
     *   ---  -------  ------       ---------------
     *   0    1        80/160fat    dock line-in
     *   1    0        120/160slim  dock line-in + jack mic
     */
    GPIOCMD = 0xe0700;
    udelay(10000);
    rec_hw_ver = (PDAT(14) & (1 << 7)) ? 0 : 1;
    GPIOCMD = 0xe070e;  /* restore default configuration */

    if (rec_hw_ver == 0) {
        GPIOCMD = 0xe060e;
    }
    else {
        /* GPIO E6 is connected to mikey IRQ line (active low),
           configure it as pull-up input */
        GPIOCMD = 0xe0600;
        PUNB(14) |= (1 << 6);
    }
}

#if 0
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
#endif


/*
 * eINT API
 */
#ifndef EINT_MAX_HANDLERS
#define EINT_MAX_HANDLERS 1
#endif
static struct eint_handler* l_handlers[EINT_MAX_HANDLERS] IDATA_ATTR;

void INIT_ATTR eint_init(void)
{
    /* disable external interrupts */
    for (int i = 0; i < EIC_N_GROUPS; i++)
        EIC_INTEN(i) = 0;
}

void eint_register(struct eint_handler *h)
{
    int i;
    int flags = disable_irq_save();

    for (i = 0; i < EINT_MAX_HANDLERS; i++) {
        if (!l_handlers[i]) {
            int group = EIC_GROUP(h->gpio_n);
            int index = EIC_INDEX(h->gpio_n);

            EIC_INTTYPE(group) |= (h->type << index);
            EIC_INTLEVEL(group) |= (h->level << index);
            EIC_INTSTAT(group) = (1 << index); /* clear */
            EIC_INTEN(group) |= (1 << index); /* enable */

            /* XXX: valid only for gpio_n = 0..127 (IRQ_EXT0..IRQ_EXT3) */
            VIC0INTENABLE = 1 << (IRQ_EXT0 + (h->gpio_n >> 5));

            l_handlers[i] = h;
            break;
        }
    }

    restore_irq(flags);

    if (i == EINT_MAX_HANDLERS)
        panicf("%s(): too many handlers!", __func__);
}

void eint_unregister(struct eint_handler *h)
{
    int flags = disable_irq_save();

    for (int i = 0; i < EINT_MAX_HANDLERS; i++) {
        if (l_handlers[i] == h) {
            int group = EIC_GROUP(h->gpio_n);
            int index = EIC_INDEX(h->gpio_n);

            EIC_INTEN(group) &= ~(1 << index); /* disable */

            /* XXX: valid only for gpio_n = 0..127 (IRQ_EXT0..IRQ_EXT3) */
            if (EIC_INTEN(group) == 0)
                VIC0INTENCLEAR = 1 << (IRQ_EXT0 + (h->gpio_n >> 5));

            l_handlers[i] = NULL;
            break;
        }
    }

    restore_irq(flags);
}

/* ISR */
static void ICODE_ATTR eint_handler(int group)
{
    int i;
    uint32_t ints;

    ints = EIC_INTSTAT(group) & EIC_INTEN(group);

    for (i = 0; i < EINT_MAX_HANDLERS; i++) {
        struct eint_handler *h = l_handlers[i];

        if (!h || (EIC_GROUP(h->gpio_n) != group))
            continue;

        uint32_t bit = 1 << EIC_INDEX(h->gpio_n);

        if (ints & bit) {
            /* Clear INTTYPE_EDGE interrupt, to clear INTTYPE_LEVEL
               interrupts the source (line level) must be "cleared" */
            if (h->type == EIC_INTTYPE_EDGE)
                EIC_INTSTAT(group) = bit; /* clear */

            if (h->autoflip)
                EIC_INTLEVEL(group) ^= bit; /* swap level */

            if (h->isr)
                h->isr(h); /* exec app handler */
        }
    }
}

void ICODE_ATTR INT_EXT0(void)
{
    eint_handler(6);
}

void ICODE_ATTR INT_EXT1(void)
{
    eint_handler(5);
}

void ICODE_ATTR INT_EXT2(void)
{
    eint_handler(4);
}

void ICODE_ATTR INT_EXT3(void)
{
    eint_handler(3);
}

void ICODE_ATTR INT_EXT6(void)
{
    eint_handler(0);
}
