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
#include <stdint.h>

struct eic_handler {
    uint8_t gpio_n;
    uint8_t type;       /* EIC_INTTYPE_ */
    uint8_t level;      /* EIC_INTLEVEL_ */
    uint8_t autoflip;
    void (*isr)(struct eic_handler*);
};

void eint_init(void);
void eint_register(struct eic_handler *h);
void eint_unregister(struct eic_handler *h);

void gpio_preinit(void);
void gpio_init(void);
/* get/set configuration for GPIO groups (0..15) */
uint32_t gpio_group_get(int group);
void gpio_group_set(int group, uint32_t mask, uint32_t cfg);

#endif /* __GPIO_S5L8702_H__ */
