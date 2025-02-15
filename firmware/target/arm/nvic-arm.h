/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 by Aidan MacDonald
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
#ifndef __NVIC_ARM_H__
#define __NVIC_ARM_H__

#include "system.h"
#include "cortex-m/nvic.h"

#define NVIC_MAX_PRIO 0xFF

static inline void nvic_enable_irq(int nr)
{
    int reg = nr / 32;
    int bit = nr % 32;

    cm_write(NVIC_ISER(reg), BIT_N(bit));
}

static inline void nvic_disable_irq(int nr)
{
    int reg = nr / 32;
    int bit = nr % 32;

    cm_write(NVIC_ICER(reg), BIT_N(bit));
}

static inline void nvic_set_pending_irq(int nr)
{
    int reg = nr / 32;
    int bit = nr % 32;

    cm_write(NVIC_ISPR(reg), BIT_N(bit));
}

static inline void nvic_clear_pending_irq(int nr)
{
    int reg = nr / 32;
    int bit = nr % 32;

    cm_write(NVIC_ICPR(reg), BIT_N(bit));
}

static inline bool nvic_is_active_irq(int nr)
{
    int reg = nr / 32;
    int bit = nr % 32;

    return cm_read(NVIC_IABR(reg)) & BIT_N(bit);
}

static inline bool nvic_is_enabled_irq(int nr)
{
    int reg = nr / 32;
    int bit = nr % 32;

    return cm_read(NVIC_ISER(reg)) & BIT_N(bit);
}

static inline void nvic_set_irq_priority(int nr, int prio)
{
    int reg = nr / 4;
    int shift = (nr % 4) * 8;

    uint32_t val = cm_read(NVIC_IPR(reg));

    val &= NVIC_MAX_PRIO << shift;
    val |= (prio & NVIC_MAX_PRIO) << shift;

    cm_write(NVIC_IPR(reg), val);
}

#endif /* __NVIC_ARM_H__ */
