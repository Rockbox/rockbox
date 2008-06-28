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
extern const struct gpio_event_list gpio1_event_list;
#endif

#if (GPIO_EVENT_MASK & USE_GPIO2_EVENTS)
static __attribute__((interrupt("IRQ"))) void GPIO2_HANDLER(void);
extern const struct gpio_event_list gpio2_event_list;
#endif

#if (GPIO_EVENT_MASK & USE_GPIO3_EVENTS)
static __attribute__((interrupt("IRQ"))) void GPIO3_HANDLER(void);
extern const struct gpio_event_list gpio3_event_list;
#endif

static struct gpio_module_descriptor
{
    struct gpio_map * const base;       /* Module base address */
    enum IMX31_INT_LIST ints;           /* AVIC int number */
    void (*handler)(void);              /* Interrupt function */
    const struct gpio_event_list *list; /* Event handler list */
} gpio_descs[GPIO_NUM_GPIO] =
{
#if (GPIO_EVENT_MASK & USE_GPIO1_EVENTS)
    {
        .base    = (struct gpio_map *)GPIO1_BASE_ADDR,
        .ints    = GPIO1,
        .handler = GPIO1_HANDLER,
    },
#endif
#if (GPIO_EVENT_MASK & USE_GPIO2_EVENTS)
    {
        .base    = (struct gpio_map *)GPIO2_BASE_ADDR,
        .ints    = GPIO2,
        .handler = GPIO2_HANDLER,
    },
#endif
#if (GPIO_EVENT_MASK & USE_GPIO3_EVENTS)
    {
        .base    = (struct gpio_map *)GPIO3_BASE_ADDR,
        .ints    = GPIO3,
        .handler = GPIO3_HANDLER,
    },
#endif
};

static void gpio_call_events(const struct gpio_module_descriptor * const desc)
{
    const struct gpio_event_list * const list = desc->list;
    struct gpio_map * const base = desc->base;
    const struct gpio_event * event, *event_last;

    /* Intersect pending and unmasked bits */
    uint32_t pnd = base->isr & base->imr;

    event = list->events;
    event_last = event + list->count;

    /* Call each event handler in order */
    /* .count is surely expected to be > 0 */
    do
    {
        uint32_t mask = event->mask;

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
    gpio_call_events(&gpio_descs[GPIO1_NUM]);
}
#endif

#if (GPIO_EVENT_MASK & USE_GPIO2_EVENTS)
static __attribute__((interrupt("IRQ"))) void GPIO2_HANDLER(void)
{
    gpio_call_events(&gpio_descs[GPIO2_NUM]);
}
#endif

#if (GPIO_EVENT_MASK & USE_GPIO3_EVENTS)
static __attribute__((interrupt("IRQ"))) void GPIO3_HANDLER(void)
{
    gpio_call_events(&gpio_descs[GPIO3_NUM]);
}
#endif

void gpio_init(void)
{
    /* Mask-out GPIO interrupts - enable what's wanted later */
    GPIO1_IMR = 0;
    GPIO2_IMR = 0;
    GPIO3_IMR = 0;

    /* Init the externally-defined event lists for each port */
#if (GPIO_EVENT_MASK & USE_GPIO1_EVENTS)
    gpio_descs[GPIO1_NUM].list = &gpio1_event_list;
#endif
#if (GPIO_EVENT_MASK & USE_GPIO2_EVENTS)
    gpio_descs[GPIO2_NUM].list = &gpio2_event_list;
#endif
#if (GPIO_EVENT_MASK & USE_GPIO3_EVENTS)
    gpio_descs[GPIO3_NUM].list = &gpio3_event_list;
#endif
}

bool gpio_enable_event(enum gpio_event_ids id)
{
    const struct gpio_module_descriptor * const desc = &gpio_descs[id >> 5];
    const struct gpio_event * const event = &desc->list->events[id & 31];
    struct gpio_map * const base = desc->base;
    volatile uint32_t *icr;
    uint32_t mask, line;
    uint32_t imr;
    int shift;

    int oldlevel = disable_irq_save();

    imr =  base->imr;

    if (imr == 0)
    {
        /* First enabled interrupt for this GPIO */
        avic_enable_int(desc->ints, IRQ, desc->list->ints_priority,
                        desc->handler);
    }

    /* Set the line sense */
    line = find_first_set_bit(event->mask);
    icr = &base->icr[line >> 4];
    shift = (line & 15) << 1;
    mask = GPIO_SENSE_CONFIG_MASK << shift;

    *icr = (*icr & ~mask) | ((event->sense << shift) & mask);

    /* Unmask the line */
    base->imr = imr | event->mask;

    restore_irq(oldlevel);

    return true;
}

void gpio_disable_event(enum gpio_event_ids id)
{
    const struct gpio_module_descriptor * const desc = &gpio_descs[id >> 5];
    const struct gpio_event * const event = &desc->list->events[id & 31];
    struct gpio_map * const base = desc->base;
    uint32_t imr;

    int oldlevel = disable_irq_save();

    /* Remove bit from mask */
    imr = base->imr & ~event->mask;

    /* Mask the line */
    base->imr = imr;

    if (imr == 0)
    {
        /* No events remain enabled */
        avic_disable_int(desc->ints);
    }

    restore_irq(oldlevel);
}
