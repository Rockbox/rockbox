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

enum stm_clock
{
    STM_CLOCK_SPI1_KER,
    STM_CLOCK_SPI2_KER,
    STM_CLOCK_SPI3_KER,
    STM_CLOCK_SPI4_KER,
    STM_CLOCK_SPI5_KER,
    STM_CLOCK_SPI6_KER,
    STM_NUM_CLOCKS,
};

/*
 * Implemented by the target to initialize all oscillators,
 * system, CPU, and bus clocks that need to be enabled from
 * early boot.
 */
void stm_target_clock_init(void) INIT_ATTR;

/*
 * Callback to be implemented by the target when the hardware
 * clock needs to be turned on or off. Clocks are internally
 * reference counted so only the first / last user will change
 * the hardware state.
 *
 * Only clocks that are actually used need to be implemented,
 * and unless otherwise noted it is allowed for enable/disable
 * to be a no-op if the clock is always enabled.
 */
void stm_target_clock_enable(enum stm_clock clock, bool enable);

/*
 * Called from system_init(). Sets up internal book-keeping
 * and then calls stm_target_clock_init().
 */
void stm_clock_init(void) INIT_ATTR;

/*
 * Enable or disable a clock. Not safe to call from an IRQ handler.
 */
void stm_clock_enable(enum stm_clock clock);
void stm_clock_disable(enum stm_clock clock);

#endif /* __CLOCK_STM32H7_H__ */
