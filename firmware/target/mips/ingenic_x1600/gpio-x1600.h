/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
 *
 * Based on firmware/target/mips/ingenic_x1000/gpio-x1000.h,
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

#ifndef __GPIO_X1600_H__
#define __GPIO_X1600_H__

/* Register definitions first: gpio-ingenic.h is written against these names
 * and checks that they are present. */
#include "x1600/gpio.h"
#include "gpio-ingenic.h"

/* Register offsets differ from X1000 but use the same names in the shared
 * GPIO driver. Pull direction is fixed: PC24-PC26 pull down; other pads
 * pull up (PM 21.4.2.19). */

/* PxEDG supports dual-edge interrupts. Shared code instead uses
 * gpio_flip_edge_irq(), which also works on X1000. */

#endif /* __GPIO_X1600_H__ */
