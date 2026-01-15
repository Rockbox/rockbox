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
#ifndef __STM32_SYSTEM_TARGET_H__
#define __STM32_SYSTEM_TARGET_H__

#include "system-arm.h"
#include "cpucache-armv7m.h"
#include <stdbool.h>

/* Enables the SysTick timer -- SysTick interrupt won't be enabled */
void stm32_systick_enable(void);

/* Disables the SysTick timer -- also disables SysTick interrupt */
void stm32_systick_disable(void);

/* Update the CPU frequency to recalibrate SysTick timer */
void stm32_systick_set_cpu_freq(uint32_t freq);

/* Enable/disable debug clock domain during sleep mode. */
void system_debug_enable(bool enable);

/* Implemented by the target -- can be a no-op if not needed */
void gpio_init(void) INIT_ATTR;
void fmc_init(void) INIT_ATTR;

/* Busy loop delay based on systick */
void udelay(uint32_t us);

#endif /* __STM32_SYSTEM_TARGET_H__ */
