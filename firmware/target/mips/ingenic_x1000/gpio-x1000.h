/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
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

#ifndef __GPIO_X1000_H__
#define __GPIO_X1000_H__

/* GPIO API
 * --------
 *
 * To assign a new function to a GPIO, call gpio_config(). This uses the
 * hardware's GPIO Z facility to atomically most GPIO registers at once,
 * so it can be used to make any state transition safely. Since GPIO Z is
 * protected by a mutex, you can't call gpio_config() from interrupt context.
 *
 * If you need to use GPIO Z directly, then use gpio_lock() and gpio_unlock()
 * to acquire the mutex.
 *
 * Depending on the current GPIO state, certain state transitions are safe to
 * perform without locking, as they only change one register:
 *
 * - for pins in GPIO_OUTPUT state:
 *   - use gpio_out_level() to change the output level
 *
 * - for pins in GPIO_IRQ_LEVEL or GPIO_IRQ_EDGE state:
 *   - use gpio_irq_level() to change the trigger level
 *   - use gpio_irq_mask() to mask/unmask the IRQ
 *
 * - for pins in GPIO_DEVICE or GPIO_INPUT state:
 *   - no special transitions allowed
 *
 * - in all states:
 *   - use gpio_set_pull() to change the pull-up/pull-down state
 */

#include "x1000/gpio.h"

/* GPIO port numbers */
#define GPIO_A 0
#define GPIO_B 1
#define GPIO_C 2
#define GPIO_D 3
#define GPIO_Z 7

/* GPIO function bits */
#define GPIO_F_PULL 16
#define GPIO_F_INT  8
#define GPIO_F_MASK 4
#define GPIO_F_PAT1 2
#define GPIO_F_PAT0 1

/* GPIO function numbers */
#define GPIO_DEVICE(i)      ((i)&3)
#define GPIO_OUTPUT(i)      (0x4|((i)&1))
#define GPIO_INPUT          0x16
#define GPIO_IRQ_LEVEL(i)   (0x1c|((i)&1))
#define GPIO_IRQ_EDGE(i)    (0x1e|((i)&1))

extern void gpio_init(void);
extern void gpio_lock(void);
extern void gpio_unlock(void);
extern void gpio_config(int port, unsigned pinmask, int func);

static inline void gpio_out_level(int port, unsigned pinmask, int level)
{
    if(level)
        jz_set(GPIO_PAT0(port), pinmask);
    else
        jz_clr(GPIO_PAT0(port), pinmask);
}

#define gpio_irq_level gpio_out_level

static inline void gpio_irq_mask(int port, unsigned pinmask, int masked)
{
    if(masked)
        jz_set(GPIO_MSK(port), pinmask);
    else
        jz_clr(GPIO_MSK(port), pinmask);
}

#define gpio_enable_irq(port, pinmask)  gpio_irq_mask((port), (pinmask), 0)
#define gpio_disable_irq(port, pinmask) gpio_irq_mask((port), (pinmask), 1)

static inline void gpio_set_pull(int port, unsigned pinmask, int state)
{
    if(state)
        jz_set(GPIO_PULL(port), pinmask);
    else
        jz_clr(GPIO_PULL(port), pinmask);
}

#endif /* __GPIO_X1000_H__ */
