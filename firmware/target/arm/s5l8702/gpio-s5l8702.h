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
#ifndef __GPIO_S5L8702_H__
#define __GPIO_S5L8702_H__

/* s5l8702 GPIO Interrupt Controller */

#define REG32_PTR_T volatile uint32_t *

#define GPIOIC_BASE 0x39a00000 /* probably a part of the system controller */

#define GPIOIC_INTLEVEL(g)  (*((REG32_PTR_T)(GPIOIC_BASE + 0x80 + 4*(g))))
#define GPIOIC_INTSTAT(g)   (*((REG32_PTR_T)(GPIOIC_BASE + 0xA0 + 4*(g))))
#define GPIOIC_INTEN(g)     (*((REG32_PTR_T)(GPIOIC_BASE + 0xC0 + 4*(g))))
#define GPIOIC_INTTYPE(g)   (*((REG32_PTR_T)(GPIOIC_BASE + 0xE0 + 4*(g))))

#define GPIOIC_INTLEVEL_LOW     0
#define GPIOIC_INTLEVEL_HIGH    1

#define GPIOIC_INTTYPE_EDGE     0
#define GPIOIC_INTTYPE_LEVEL    1

/* 7 groups of 32 interrupts, GPIO pins are seen as 'wired' to
 * groups 6..3 in reverse order. Last four bits on group 3 are
 * always masked (GPIO 124..127). All bits in groups 1 and 2 are
 * also masked (not used). On group 0 all bits are masked except
 * bits 0 and 2, where are these two lines connected is unknown.
 */

/* get GPIOIC group and bit for a given GPIO port */
#define IC_GROUP(n)  (6 - (n >> 5))
#define IC_IDX(n)    ((0x18 - (n & 0x18)) | (n & 0x7))

void gpio_init(void);
void gpio_int_register(int gpio_n, void *isr,
                        int type, int level, int autoflip);
void gpio_int_enable(int gpio_n);
void gpio_int_disable(int gpio_n);

#endif /* __GPIO_S5L8702_H__ */
