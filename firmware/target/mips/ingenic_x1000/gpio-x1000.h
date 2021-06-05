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
#define GPIOF_DEVICE(i)     ((i)&3)
#define GPIOF_OUTPUT(i)     (0x4|((i)&1))
#define GPIOF_INPUT         0x16
#define GPIOF_IRQ_LEVEL(i)  (0x1c|((i)&1))
#define GPIOF_IRQ_EDGE(i)   (0x1e|((i)&1))

/* GPIO pin numbers */
#define GPION_CREATE(port, pin) ((((port) & 3) << 5) | ((pin) & 0x1f))
#define GPION_PORT(gpio)        (((gpio) >> 5) & 3)
#define GPION_PIN(gpio)         ((gpio) & 0x1f)
#define GPION_MASK(gpio)        (1u << GPION_PIN(gpio))

/* Easy pin number macros */
#define GPIO_PA(x)  GPION_CREATE(GPIO_A, x)
#define GPIO_PB(x)  GPION_CREATE(GPIO_B, x)
#define GPIO_PC(x)  GPION_CREATE(GPIO_C, x)
#define GPIO_PD(x)  GPION_CREATE(GPIO_D, x)

/* GPIO number to IRQ number (need to include "irq-x1000.h") */
#define GPIO_TO_IRQ(gpio) IRQ_GPIO(GPION_PORT(gpio), GPION_PIN(gpio))

/* Pingroup settings are used for system devices */
struct pingroup_setting {
    int port;
    uint32_t pins;
    int func;
};

/* GPIO settings are used for single pins under software control */
struct gpio_setting {
    int gpio;
    int func;
};

/* Target pins are defined as GPIO_XXX constants usable with the GPIO API */
enum {
#define DEFINE_GPIO(_name, _gpio, _func) GPIO_##_name = _gpio,
#define DEFINE_PINGROUP(...)
#include "gpio-target.h"
#undef DEFINE_GPIO
#undef DEFINE_PINGROUP
    GPIO_NONE = -1,
};

/* These are pin IDs which index gpio_settings */
enum {
#define DEFINE_GPIO(_name, ...) PIN_##_name,
#define DEFINE_PINGROUP(...)
#include "gpio-target.h"
#undef DEFINE_GPIO
#undef DEFINE_PINGROUP
    PIN_COUNT,
};

/* Pingroup IDs which index pingroup_settings */
enum {
#define DEFINE_GPIO(...)
#define DEFINE_PINGROUP(_name, ...) PINGROUP_##_name,
#include "gpio-target.h"
#undef DEFINE_GPIO
#undef DEFINE_PINGROUP
    PINGROUP_COUNT,
};

/* arrays which define the target's GPIO settings */
extern const struct gpio_setting gpio_settings[PIN_COUNT];
extern const struct pingroup_setting pingroup_settings[PINGROUP_COUNT];

/* stringified names for use in debug menus */
extern const char* const gpio_names[PIN_COUNT];
extern const char* const pingroup_names[PINGROUP_COUNT];

/* called at early init to set up GPIOs */
extern void gpio_init(void);

/* Use GPIO Z to reconfigure several pins atomically */
extern void gpioz_configure(int port, uint32_t pins, int func);

static inline void gpio_set_function(int gpio, int func)
{
    gpioz_configure(GPION_PORT(gpio), GPION_MASK(gpio), func);
}

static inline int gpio_get_level(int gpio)
{
    return REG_GPIO_PIN(GPION_PORT(gpio)) & GPION_MASK(gpio) ? 1 : 0;
}

static inline void gpio_set_level(int gpio, int value)
{
    if(value)
        jz_set(GPIO_PAT0(GPION_PORT(gpio)), GPION_MASK(gpio));
    else
        jz_clr(GPIO_PAT0(GPION_PORT(gpio)), GPION_MASK(gpio));
}

static inline void gpio_set_pull(int gpio, int state)
{
    if(state)
        jz_set(GPIO_PULL(GPION_PORT(gpio)), GPION_MASK(gpio));
    else
        jz_clr(GPIO_PULL(GPION_PORT(gpio)), GPION_MASK(gpio));
}

static inline void gpio_mask_irq(int gpio, int mask)
{
    if(mask)
        jz_set(GPIO_MSK(GPION_PORT(gpio)), GPION_MASK(gpio));
    else
        jz_clr(GPIO_MSK(GPION_PORT(gpio)), GPION_MASK(gpio));
}

#define gpio_set_irq_level      gpio_set_level
#define gpio_enable_irq(gpio)   gpio_mask_irq((gpio), 0)
#define gpio_disable_irq(gpio)  gpio_mask_irq((gpio), 1)

/* Helper function for edge-triggered IRQs when you want to get an
 * interrupt on both the rising and falling edges. The hardware can
 * only be set up to interrupt on one edge, so interrupt handlers
 * can call this function to flip the trigger to the other edge.
 *
 * Despite the name, this doesn't depend on the currently set edge,
 * it just reads the GPIO state and sets up an edge trigger to detect
 * a change to the other state -- if some transitions were missed the
 * IRQ trigger may remain unchanged.
 *
 * It can be safely used to initialize the IRQ level.
 */
static inline void gpio_flip_edge_irq(int gpio)
{
    if(gpio_get_level(gpio))
        gpio_set_irq_level(gpio, 0);
    else
        gpio_set_irq_level(gpio, 1);
}

#endif /* __GPIO_X1000_H__ */
