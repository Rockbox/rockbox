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
#ifndef __GPIO_STM32_H__
#define __GPIO_STM32_H__

#include "system.h"
#include "gpio-target.h"
#include "stm32h7/gpio.h"
#include <stddef.h>
#include <stdint.h>

/* GPIO port numbers */
#define GPIO_A 0
#define GPIO_B 1
#define GPIO_C 2
#define GPIO_D 3
#define GPIO_E 4
#define GPIO_F 5
#define GPIO_G 6
#define GPIO_H 7
#define GPIO_I 8
#define GPIO_J 9
#define GPIO_K 10

/* GPIO pin numbers */
#define GPION_CREATE(port, pin) ((((port) & 0xf) << 4) | ((pin) & 0xf))
#define GPION_PORT(gpio)        (((gpio) >> 4) & 0xf)
#define GPION_PIN(gpio)         ((gpio) & 0xf)

/* Easy pin number macros */
#define GPIO_PA(x)              GPION_CREATE(GPIO_A, x)
#define GPIO_PB(x)              GPION_CREATE(GPIO_B, x)
#define GPIO_PC(x)              GPION_CREATE(GPIO_C, x)
#define GPIO_PD(x)              GPION_CREATE(GPIO_D, x)
#define GPIO_PE(x)              GPION_CREATE(GPIO_E, x)
#define GPIO_PF(x)              GPION_CREATE(GPIO_F, x)
#define GPIO_PG(x)              GPION_CREATE(GPIO_G, x)
#define GPIO_PH(x)              GPION_CREATE(GPIO_H, x)
#define GPIO_PI(x)              GPION_CREATE(GPIO_I, x)
#define GPIO_PJ(x)              GPION_CREATE(GPIO_J, x)
#define GPIO_PK(x)              GPION_CREATE(GPIO_K, x)

/* GPIO modes */
#define GPIO_MODE_INPUT         0
#define GPIO_MODE_OUTPUT        1
#define GPIO_MODE_FUNCTION      2
#define GPIO_MODE_ANALOG        3

/* GPIO output types */
#define GPIO_TYPE_PUSH_PULL     0
#define GPIO_TYPE_OPEN_DRAIN    1

/* GPIO output speeds */
#define GPIO_SPEED_LOW          0
#define GPIO_SPEED_MEDIUM       1
#define GPIO_SPEED_HIGH         2
#define GPIO_SPEED_VERYHIGH     3

/* GPIO pullup settings */
#define GPIO_PULL_DISABLED      0
#define GPIO_PULL_UP            1
#define GPIO_PULL_DOWN          2

/* Helpers for GPIO library functions */
#define GPIO_SETTING_MASK(n)    (0x3u << GPIO_SETTING_LSB(n))
#define GPIO_SETTING_LSB(n)     ((n)*2)

#define GPIO_AF_MASK(n)         (0xFu << GPIO_AF_LSB(n))
#define GPIO_AF_LSB(n)          ((n)*4)

/* GPIO function attributes */
#define GPIO_F_MODE(n)          (((n) & 0x3) << 0)
#define GPIO_F_TYPE(n)          (((n) & 0x1) << 2)
#define GPIO_F_SPEED(n)         (((n) & 0x3) << 3)
#define GPIO_F_PULL(n)          (((n) & 0x3) << 5)
#define GPIO_F_AFNUM(n)         (((n) & 0xf) << 7)
#define GPIO_F_INITLEVEL(n)     (((n) & 0x1) << 11)

#define GPIO_F_GETMODE(x)       (((x) >> 0) & 0x3)
#define GPIO_F_GETTYPE(x)       (((x) >> 2) & 0x1)
#define GPIO_F_GETSPEED(x)      (((x) >> 3) & 0x3)
#define GPIO_F_GETPULL(x)       (((x) >> 5) & 0x3)
#define GPIO_F_GETAFNUM(x)      (((x) >> 7) & 0xf)
#define GPIO_F_GETINITLEVEL(x)  (((x) >> 11) & 0x1)

#define GPIOF_ANALOG() \
    (GPIO_F_MODE(GPIO_MODE_ANALOG) | \
     GPIO_F_TYPE(GPIO_TYPE_PUSH_PULL) | \
     GPIO_F_SPEED(GPIO_SPEED_LOW) | \
     GPIO_F_PULL(GPIO_PULL_DISABLED))

#define GPIOF_INPUT(pull) \
    (GPIO_F_MODE(GPIO_MODE_INPUT) | \
     GPIO_F_PULL(pull))

#define GPIOF_OUTPUT(initlevel, type, speed, pull) \
    (GPIO_F_MODE(GPIO_MODE_OUTPUT) | \
     GPIO_F_INITLEVEL(initlevel) | \
     GPIO_F_TYPE(type) | \
     GPIO_F_SPEED(speed) | \
     GPIO_F_PULL(pull))

#define GPIOF_FUNCTION(afnum, type, speed, pull) \
    (GPIO_F_MODE(GPIO_MODE_FUNCTION) | \
     GPIO_F_AFNUM(afnum) | \
     GPIO_F_TYPE(type) | \
     GPIO_F_SPEED(speed) | \
     GPIO_F_PULL(pull))

struct gpio_setting
{
    uint16_t gpio;
    uint16_t func;
};

struct pingroup_setting
{
    int port;
    uint16_t pins;
    uint16_t func;
};

#define STM_DEFGPIO(_gpio, _func) \
    {.gpio = (_gpio), .func = (_func)}

#define STM_DEFPINS(_port, _pins, _func) \
    {.port = (_port), .pins = (_pins), .func = (_func)}

/*
 * Configure a single GPIO or multiple GPIOs on one port.
 * 'confbits' describe the configuration to apply: see GPIO_F macros above.
 */
void gpio_configure_single(int gpio, int confbits);
void gpio_configure_port(int port, uint16_t mask, int confbits);

/*
 * Configure multiple GPIO ports according to a list of settings.
 *
 * Identical to calling gpio_configure_single() with each GPIO setting
 * and gpio_configure_port() with each pingroup setting. It is mainly
 * to make the organization of the initial GPIO configuration tidier
 * and intended to be called from gpio_init().
 */
void gpio_configure_all(
    const struct gpio_setting *gpios, size_t ngpios,
    const struct pingroup_setting *pingroups, size_t ngroups) INIT_ATTR;

static inline void gpio_set_mode(int gpio, int mode)
{
    int port = GPION_PORT(gpio);
    int pin = GPION_PIN(gpio);
    uint32_t val = REG_GPIO_MODER(port);
    uint32_t mask = GPIO_SETTING_MASK(pin);

    val &= ~mask;
    val |= (mode << GPIO_SETTING_LSB(pin)) & mask;

    REG_GPIO_MODER(port) = val;
}

static inline void gpio_set_type(int gpio, int type)
{
    int port = GPION_PORT(gpio);
    int pin = GPION_PIN(gpio);
    uint32_t val = REG_GPIO_OTYPER(port);
    uint32_t mask = BIT_N(pin);

    val &= ~mask;
    val |= (type << pin) & mask;

    REG_GPIO_OTYPER(port) = val;
}

static inline void gpio_set_speed(int gpio, int speed)
{
    int port = GPION_PORT(gpio);
    int pin = GPION_PIN(gpio);
    uint32_t val = REG_GPIO_OSPEEDR(port);
    uint32_t mask = GPIO_SETTING_MASK(pin);

    val &= ~mask;
    val |= (speed << GPIO_SETTING_LSB(pin)) & mask;

    REG_GPIO_OSPEEDR(port) = val;
}

static inline void gpio_set_pull(int gpio, int pull)
{
    int port = GPION_PORT(gpio);
    int pin = GPION_PIN(gpio);
    uint32_t val = REG_GPIO_PUPDR(port);
    uint32_t mask = GPIO_SETTING_MASK(pin);

    val &= ~mask;
    val |= (pull << GPIO_SETTING_LSB(pin)) & mask;

    REG_GPIO_PUPDR(port) = val;
}

static inline void gpio_set_function(int gpio, int func)
{
    int port = GPION_PORT(gpio);
    int pin = GPION_PIN(gpio);
    volatile uint32_t *addr = &REG_GPIO_AFRL(port);
    if (pin >= 8) {
        addr += &REG_GPIO_AFRH(0) - &REG_GPIO_AFRL(0);
        pin -= 8;
    }

    uint32_t val = *addr;
    uint32_t mask = GPIO_AF_MASK(pin);

    val &= ~mask;
    val |= (func << GPIO_AF_LSB(pin)) & mask;

    *addr = val;
}

static inline int gpio_get_level(int gpio)
{
    int port = GPION_PORT(gpio);
    int pin = GPION_PIN(gpio);

    return REG_GPIO_IDR(port) & BIT_N(pin);
}

static inline void gpio_set_level(int gpio, int value)
{
    int port = GPION_PORT(gpio);
    int pin = GPION_PIN(gpio);

    if (value)
        REG_GPIO_BSRR(port) = BIT_N(pin);
    else
        REG_GPIO_BSRR(port) = BIT_N(pin + 16);
}

#endif /* __GPIO_STM32_H__ */
