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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _GPIO_IMX31_H_
#define _GPIO_IMX31_H_

#include "gpio-target.h"

#define USE_GPIO1_EVENTS (1 << 0)
#define USE_GPIO2_EVENTS (1 << 1)
#define USE_GPIO3_EVENTS (1 << 2)

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
    volatile uint32_t dr;        /* 00h */
    volatile uint32_t gdir;      /* 04h */
    volatile uint32_t psr;       /* 08h */
    volatile uint32_t icr[2];    /* 0Ch */
    volatile uint32_t imr;       /* 14h */
    volatile uint32_t isr;       /* 18h */
};

/* Pending events will be called in array order */

/* Describes a single event for a pin */
struct gpio_event
{
    int line;                       /* Line number (0-31) */
    enum gpio_int_sense_enum sense; /* Type of sense */
    int (*callback)(void);          /* Callback function (return nonzero
                                     * to indicate this event was handled) */
};

/* Describes the events attached to a port */
struct gpio_event_list
{
    int priority;                    /* Interrupt priority for this GPIO */
    unsigned count;                  /* Count of events */
    const struct gpio_event *events; /* List of events */
};

void gpio_init(void);
bool gpio_enable_event(enum gpio_module_number gpio, unsigned id);
void gpio_disable_event(enum gpio_module_number gpio, unsigned id);

#endif /* _GPIO_IMX31_H_ */
