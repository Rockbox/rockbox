/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2008 by Michael Sevakis
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
#ifndef GPIO_IMX31_H
#define GPIO_IMX31_H

/* Static registration mechanism for imx31 GPIO interrupts */
#define USE_GPIO1_EVENTS (1 << 0)
#define USE_GPIO2_EVENTS (1 << 1)
#define USE_GPIO3_EVENTS (1 << 2)

/* Module indexes defined by which GPIO modules are used */
enum gpio_module_number
{
    __GPIO_NUM_START = -1,
#if (GPIO_EVENT_MASK & USE_GPIO1_EVENTS)
    GPIO1_NUM,
#endif
#if (GPIO_EVENT_MASK & USE_GPIO2_EVENTS)
    GPIO2_NUM,
#endif
#if (GPIO_EVENT_MASK & USE_GPIO3_EVENTS)
    GPIO3_NUM,
#endif
    GPIO_NUM_GPIO,
};

/* Module corresponding to the event ID is identified by range */
enum gpio_event_bases
{
#if (GPIO_EVENT_MASK & USE_GPIO1_EVENTS)
    GPIO1_EVENT_FIRST = 32*GPIO1_NUM,
#endif
#if (GPIO_EVENT_MASK & USE_GPIO2_EVENTS)
    GPIO2_EVENT_FIRST = 32*GPIO2_NUM,
#endif
#if (GPIO_EVENT_MASK & USE_GPIO3_EVENTS)
    GPIO3_EVENT_FIRST = 32*GPIO3_NUM,
#endif
};

#include "gpio-target.h"

/* Possible values for gpio interrupt line config */
enum gpio_int_sense_enum
{
    GPIO_SENSE_LOW_LEVEL = 0, /* High-level sensitive */
    GPIO_SENSE_HIGH_LEVEL,    /* Low-level sensitive */
    GPIO_SENSE_RISING,        /* Rising-edge sensitive */
    GPIO_SENSE_FALLING,       /* Falling-edge sensitive */
};

#define GPIO_SENSE_CONFIG_MASK 0x3

/* Register map for each module */
struct gpio_map
{
    volatile uint32_t dr;           /* 00h */
    volatile uint32_t gdir;         /* 04h */
    volatile uint32_t psr;          /* 08h */
    union
    {
        struct
        {
            volatile uint32_t icr1; /* 0Ch */
            volatile uint32_t icr2; /* 10h */
        };
        volatile uint32_t icr[2];   /* 0Ch */
    };
    volatile uint32_t imr;          /* 14h */
    volatile uint32_t isr;          /* 18h */
};

/* Pending events will be called in array order which allows easy
 * pioritization */

/* Describes a single event for a pin */
struct gpio_event
{
    uint32_t mask;                  /* mask: 1 << (0...31) */
    enum gpio_int_sense_enum sense; /* Type of sense */
    void (*callback)(void);         /* Callback function */
};

/* Describes the events attached to a port */
struct gpio_event_list
{
    int ints_priority;               /* Interrupt priority for this GPIO */
    unsigned count;                  /* Count of events for the module */
    const struct gpio_event *events; /* List of events */
};

void gpio_init(void);
bool gpio_enable_event(enum gpio_event_ids id);
void gpio_disable_event(enum gpio_event_ids id);

#endif /* GPIO_IMX31_H */
