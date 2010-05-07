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
 * IMX31 GPIO event manager
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
#include "config.h"
#include "system.h"
#include "avic-imx31.h"
#include "gpio-imx31.h"

/* UIE vector found in avic-imx31.c */
extern void UIE_VECTOR(void);

/* Event lists are allocated for the specific target */
#if (GPIO_EVENT_MASK & USE_GPIO1_EVENTS)
static __attribute__((interrupt("IRQ"))) void GPIO1_HANDLER(void);
extern const struct gpio_event gpio1_events[GPIO1_NUM_EVENTS];
#endif

#if (GPIO_EVENT_MASK & USE_GPIO2_EVENTS)
static __attribute__((interrupt("IRQ"))) void GPIO2_HANDLER(void);
extern const struct gpio_event gpio2_events[GPIO2_NUM_EVENTS];
#endif

#if (GPIO_EVENT_MASK & USE_GPIO3_EVENTS)
static __attribute__((interrupt("IRQ"))) void GPIO3_HANDLER(void);
extern const struct gpio_event gpio3_events[GPIO3_NUM_EVENTS];
#endif

#define DR      (0x00 / sizeof (unsigned long)) /* 00h        */
#define GDIR    (0x04 / sizeof (unsigned long)) /* 04h        */
#define PSR     (0x08 / sizeof (unsigned long)) /* 08h        */
#define ICR     (0x0C / sizeof (unsigned long)) /* 0Ch ICR1,2 */
#define IMR     (0x14 / sizeof (unsigned long)) /* 14h        */
#define ISR     (0x18 / sizeof (unsigned long))

static const struct gpio_module_desc
{
    volatile unsigned long * const base;    /* Module base address */
    void (* const handler)(void);           /* Interrupt function */
    const struct gpio_event * const events; /* Event handler list */
    const uint8_t ints;                     /* AVIC int number */
    const uint8_t int_priority;             /* AVIC int priority */
    const uint8_t count;                    /* Number of events */
} gpio_descs[GPIO_NUM_GPIO] =
{
#if (GPIO_EVENT_MASK & USE_GPIO1_EVENTS)
    {
        .base         = (unsigned long *)GPIO1_BASE_ADDR,
        .ints         = INT_GPIO1,
        .handler      = GPIO1_HANDLER,
        .events       = gpio1_events,
        .count        = GPIO1_NUM_EVENTS,
        .int_priority = GPIO1_INT_PRIO
    },
#endif
#if (GPIO_EVENT_MASK & USE_GPIO2_EVENTS)
    {
        .base         = (unsigned long *)GPIO2_BASE_ADDR,
        .ints         = INT_GPIO2,
        .handler      = GPIO2_HANDLER,
        .events       = gpio2_events,
        .count        = GPIO2_NUM_EVENTS,
        .int_priority = GPIO2_INT_PRIO
    },
#endif
#if (GPIO_EVENT_MASK & USE_GPIO3_EVENTS)
    {
        .base         = (unsigned long *)GPIO3_BASE_ADDR,
        .ints         = INT_GPIO3,
        .handler      = GPIO3_HANDLER,
        .events       = gpio3_events,
        .count        = GPIO3_NUM_EVENTS,
        .int_priority = GPIO3_INT_PRIO,
    },
#endif
};

static void gpio_call_events(enum gpio_module_number gpio)
{
    const struct gpio_module_desc * const desc = &gpio_descs[gpio];
    volatile unsigned long * const base = desc->base;
    const struct gpio_event * event, *event_last;

    event = desc->events;
    event_last = event + desc->count;

    /* Intersect pending and unmasked bits */
    unsigned long pnd = base[ISR] & base[IMR];

    /* Call each event handler in order */
    /* .count is surely expected to be > 0 */
    do
    {
        unsigned long mask = event->mask;

        if (pnd & mask)
        {
            event->callback();
            pnd &= ~mask;
        }

        if (pnd == 0)
            break; /* Teminate early if nothing more to service */
    }
    while (++event < event_last);

    if (pnd != 0)
    {
        /* One or more weren't handled */
        UIE_VECTOR();
    }
}

#if (GPIO_EVENT_MASK & USE_GPIO1_EVENTS)
static __attribute__((interrupt("IRQ"))) void GPIO1_HANDLER(void)
{
    gpio_call_events(GPIO1_NUM);
}
#endif

#if (GPIO_EVENT_MASK & USE_GPIO2_EVENTS)
static __attribute__((interrupt("IRQ"))) void GPIO2_HANDLER(void)
{
    gpio_call_events(GPIO2_NUM);
}
#endif

#if (GPIO_EVENT_MASK & USE_GPIO3_EVENTS)
static __attribute__((interrupt("IRQ"))) void GPIO3_HANDLER(void)
{
    gpio_call_events(GPIO3_NUM);
}
#endif

void gpio_init(void)
{
    /* Mask-out GPIO interrupts - enable what's wanted later */
    int i;
    for (i = 0; i < GPIO_NUM_GPIO; i++)
        gpio_descs[i].base[IMR] = 0;
}

bool gpio_enable_event(enum gpio_event_ids id)
{
    const struct gpio_module_desc * const desc = &gpio_descs[id >> 5];
    const struct gpio_event * const event = &desc->events[id & 31];
    volatile unsigned long * const base = desc->base;
    volatile unsigned long *icr;
    unsigned long mask, line;
    unsigned long imr;
    int shift;

    int oldlevel = disable_irq_save();

    imr =  base[IMR];

    if (imr == 0)
    {
        /* First enabled interrupt for this GPIO */
        avic_enable_int(desc->ints, INT_TYPE_IRQ, desc->int_priority,
                        desc->handler);
    }

    /* Set the line sense */
    line = find_first_set_bit(event->mask);
    icr = &base[ICR + (line >> 4)];
    shift = 2*(line & 15);
    mask = GPIO_SENSE_CONFIG_MASK << shift;

    *icr = (*icr & ~mask) | ((event->sense << shift) & mask);

    /* Unmask the line */
    base[IMR] = imr | event->mask;

    restore_irq(oldlevel);

    return true;
}

void gpio_disable_event(enum gpio_event_ids id)
{
    const struct gpio_module_desc * const desc = &gpio_descs[id >> 5];
    const struct gpio_event * const event = &desc->events[id & 31];
    volatile unsigned long * const base = desc->base;
    unsigned long imr;

    int oldlevel = disable_irq_save();

    /* Remove bit from mask */
    imr = base[IMR] & ~event->mask;

    /* Mask the line */
    base[IMR] = imr;

    if (imr == 0)
    {
        /* No events remain enabled */
        avic_disable_int(desc->ints);
    }

    restore_irq(oldlevel);
}
