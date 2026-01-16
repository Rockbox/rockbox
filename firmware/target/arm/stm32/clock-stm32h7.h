/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 Aidan MacDonald
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
#ifndef __CLOCK_STM32H7_H__
#define __CLOCK_STM32H7_H__

#include "system.h"
#include <stdbool.h>
#include <stddef.h>

struct stm32_clock
{
    uint32_t frequency;

    uint32_t en_reg;
    uint32_t en_bit;

    uint32_t lpen_reg;
    uint32_t lpen_bit;
};

/*
 * Implemented by the target to initialize all oscillators,
 * system, CPU, and bus clocks that need to be enabled from
 * early boot.
 */
void stm_target_clock_init(void) INIT_ATTR;

/*
 * Called by system_init() to setup target clocks.
 */
static inline void stm_clock_init(void)
{
    stm_target_clock_init();
}

/*
 * Enables a clock by setting its enable bits in the RCC.
 */
static inline void stm32_clock_enable(const struct stm32_clock *clk)
{
    if (clk->en_reg)
        *(volatile uint32_t *)clk->en_reg |= clk->en_bit;

    if (clk->lpen_reg)
        *(volatile uint32_t *)clk->lpen_reg |= clk->lpen_bit;
}

/*
 * Disables a clock in the RCC.
 */
static inline void stm32_clock_disable(const struct stm32_clock *clk)
{
    if (clk->en_reg)
        *(volatile uint32_t *)clk->en_reg &= ~clk->en_bit;

    if (clk->lpen_reg)
        *(volatile uint32_t *)clk->lpen_reg &= ~clk->lpen_bit;
}

/*
 * Get a clock's frequency in Hz.
 */
static inline uint32_t stm32_clock_get_frequency(const struct stm32_clock *clk)
{
    return clk->frequency;
}

#endif /* __CLOCK_STM32H7_H__ */
