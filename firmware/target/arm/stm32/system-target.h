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

/* Implemented by the target -- can be a no-op if not needed */
void gpio_init(void) INIT_ATTR;
void fmc_init(void) INIT_ATTR;

void udelay(uint32_t us);
void mdelay(uint32_t ms);

#endif /* __STM32_SYSTEM_TARGET_H__ */
