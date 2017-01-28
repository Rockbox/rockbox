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
#define DEFINE_GPIO_VECTOR_TABLE
#include "gpio-target.h"

/* UIE vector found in avic-imx31.c */
extern void UIE_VECTOR(void);

/* Event lists are allocated for the specific target */
#if (GPIO_EVENT_MASK & USE_GPIO1_EVENTS)
static __attribute__((interrupt("IRQ"))) void GPIO1_HANDLER(void);
#endif

#if (GPIO_EVENT_MASK & USE_GPIO2_EVENTS)
static __attribute__((interrupt("IRQ"))) void GPIO2_HANDLER(void);
#endif

#if (GPIO_EVENT_MASK & USE_GPIO3_EVENTS)
static __attribute__((interrupt("IRQ"))) void GPIO3_HANDLER(void);
#endif

static struct gpio_module_desc
{
    volatile unsigned long * const base;    /* Module base address */
    void (* const handler)(void);           /* Interrupt function */
    const struct gpio_event *events;        /* Event handler list */
    unsigned long enabled;                  /* Enabled event mask */
    const uint8_t ints;                     /* AVIC int number */
    const uint8_t int_priority;             /* AVIC int priority */
    uint8_t       count;                    /* Number of events */
} * const gpio_descs[] =
{
#if (GPIO_EVENT_MASK & USE_GPIO1_EVENTS)
    [GPIO1_NUM] = &(struct gpio_module_desc) {
        .base         = (unsigned long *)GPIO1_BASE_ADDR,
        .ints         = INT_GPIO1,
        .handler      = GPIO1_HANDLER,
        .int_priority = GPIO1_INT_PRIO
    },
#endif
#if (GPIO_EVENT_MASK & USE_GPIO2_EVENTS)
    [GPIO2_NUM] = &(struct gpio_module_desc) {
        .base         = (unsigned long *)GPIO2_BASE_ADDR,
        .ints         = INT_GPIO2,
        .handler      = GPIO2_HANDLER,
        .int_priority = GPIO2_INT_PRIO
    },
#endif
#if (GPIO_EVENT_MASK & USE_GPIO3_EVENTS)
    [GPIO3_NUM] = &(struct gpio_module_desc) {
        .base         = (unsigned long *)GPIO3_BASE_ADDR,
        .ints         = INT_GPIO3,
        .handler      = GPIO3_HANDLER,
        .int_priority = GPIO3_INT_PRIO,
    },
#endif
};

#define GPIO_MODULE_CNT ARRAYLEN(gpio_descs)

static const struct gpio_event * event_from_id(
    const struct gpio_module_desc *desc, enum gpio_id id)
{
    const struct gpio_event *events = desc->events;
    for (unsigned int i = 0; i < desc->count; i++)
    {
        if (events[i].id == id)
            return &events[i];
    }

    return NULL;
}

static void gpio_call_events(enum gpio_module_number gpio)
{
    const struct gpio_module_desc * const desc = gpio_descs[gpio];
    volatile unsigned long * const base = desc->base;

    /* Send only events that are not masked */
    unsigned long pnd = base[GPIO_ISR] & base[GPIO_IMR];

    if (pnd & ~desc->enabled)
        UIE_VECTOR(); /* One or more aren't handled properly */

    const struct gpio_event *event = desc->events;

    while (pnd)
    {
        unsigned long mask = 1ul << (event->id % 32);
        if (pnd & mask)
        {
            event->callback();
            pnd &= ~mask;
        }

        event++;
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

void INIT_ATTR gpio_init(void)
{
    for (unsigned int mod = 0; mod < GPIO_MODULE_CNT; mod++)
    {
        struct gpio_module_desc * const desc = gpio_descs[mod];
        if (!desc)
            continue;

        /* Parse the event table per module. First contiguous run seen for a
         * module is used. */
        const struct gpio_event *event = gpio_event_vector_tbl;
        for (unsigned int i = 0; i < gpio_event_vector_tbl_len; i++, event++)
        {
            if (event->id / 32 == mod)
            {
                desc->events = event;
                while (++desc->count < 32 && (++event)->id / 32 == mod);
                break;
            }
        }

        /* Mask-out GPIO interrupts - enable what's wanted later */
        desc->base[GPIO_IMR] = 0;
    }
}

bool gpio_enable_event(enum gpio_id id, bool enable)
{
    unsigned int mod = id / 32;

    struct gpio_module_desc * const desc = gpio_descs[mod];

    if (mod >= GPIO_MODULE_CNT || desc == NULL)
        return false;

    const struct gpio_event * const event = event_from_id(desc, id);
    if (!event)
        return false;

    volatile unsigned long * const base = desc->base;
    unsigned long num = id % 32;
    unsigned long mask = 1ul << num;

    unsigned long cpsr = disable_irq_save();

    unsigned long imr = base[GPIO_IMR];

    if (enable)
    {
        if (desc->enabled == 0)
        {
            /* First enabled interrupt for this GPIO */
            avic_enable_int(desc->ints, INT_TYPE_IRQ, desc->int_priority,
                            desc->handler);
        }

        /* Set the line sense */
        if (event->sense == GPIO_SENSE_EDGE_SEL)
        {
            /* Interrupt configuration register is ignored */
            base[GPIO_EDGE_SEL] |= mask;
        }
        else
        {
            volatile unsigned long *icrp = &base[GPIO_ICR + num / 16];
            unsigned int shift = 2*(num % 16);
            bitmod32(icrp, event->sense << shift, 0x3 << shift);
            base[GPIO_EDGE_SEL] &= ~mask;
        }

        /* Unmask the line */
        desc->enabled |= mask;
        imr |= mask;
    }
    else
    {
        imr &= ~mask;
        desc->enabled &= ~mask;

        if (desc->enabled == 0)
            avic_disable_int(desc->ints); /* No events remain enabled */
    }

    base[GPIO_IMR] = imr;

    restore_irq(cpsr);

    return true;
}
